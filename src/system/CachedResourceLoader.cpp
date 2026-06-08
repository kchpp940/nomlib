/******************************************************************************

  nomlib - C++11 cross-platform game engine

Copyright (c) 2013, 2014 Jeffrey Carpenter <i8degrees@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
#include "nomlib/system/CachedResourceLoader.hpp"

#include "nomlib/ptree/Value.hpp"

namespace nom {

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

CachedResourceLoader::CachedResourceLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

CachedResourceLoader::~CachedResourceLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM,
                      nom::NOM_LOG_PRIORITY_VERBOSE );

  // Release everything (force unload regardless of references)
  this->clear( ReleasePolicy::Force );
}

// ---------------------------------------------------------------------------
// Configuration — Value-based parsing (no file I/O here)
// ---------------------------------------------------------------------------

bool CachedResourceLoader::parse_search_paths_from_value(
  const Value& resources_node )
{
  return this->search_path_.parse_from_value( resources_node );
}

bool CachedResourceLoader::parse_manifest_from_value(
  const Value& manifest_node )
{
  return this->manifest_.parse_from_value( manifest_node );
}

SearchPath& CachedResourceLoader::search_path( void )
{
  return this->search_path_;
}

const SearchPath& CachedResourceLoader::search_path( void ) const
{
  return this->search_path_;
}

ResourceManifest& CachedResourceLoader::manifest( void )
{
  return this->manifest_;
}

const ResourceManifest& CachedResourceLoader::manifest( void ) const
{
  return this->manifest_;
}

void CachedResourceLoader::register_type_loader(
  std::unique_ptr<IResourceTypeLoader> loader )
{
  if( loader == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
                 "CachedResourceLoader: refusing to register null type loader" );
    return;
  }

  ResourceFile::Type type = loader->type();
  auto shared = std::shared_ptr<IResourceTypeLoader>( std::move( loader ) );

  if( type == ResourceFile::Type::Invalid )
  {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                  "CachedResourceLoader: loader reports Type::Invalid; "
                  "it must be registered explicitly via the overloaded "
                  "register_type_loader(Type, unique_ptr)" );
    return;
  }

  this->loaders_[type] = shared;

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "CachedResourceLoader: registered type loader for type ",
                static_cast<int>( type ) );
}

void CachedResourceLoader::register_type_loader(
  ResourceFile::Type type,
  std::unique_ptr<IResourceTypeLoader> loader )
{
  if( loader == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
                 "CachedResourceLoader: refusing to register null type loader" );
    return;
  }

  auto shared = std::shared_ptr<IResourceTypeLoader>( std::move( loader ) );
  this->loaders_[type] = shared;

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "CachedResourceLoader: registered type loader for type ",
                static_cast<int>( type ) );
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

std::string CachedResourceLoader::resolve_internal_path(
  const ResourceDescriptor& desc ) const
{
  return this->search_path_.resolve( desc.path );
}

IResourceTypeLoader* CachedResourceLoader::find_loader(
  ResourceFile::Type type ) const
{
  auto itr = this->loaders_.find( type );
  if( itr != this->loaders_.end() )
  {
    return itr->second.get();
  }
  return nullptr;
}

void* CachedResourceLoader::load_internal( const std::string& name,
                                           ResourceFile::Type expected_type,
                                           bool& out_loaded_from_cache )
{
  out_loaded_from_cache = false;
  std::lock_guard<std::mutex> lock( this->mutex_ );

  // 1. Look up in manifest
  const ResourceDescriptor* desc = this->manifest_.find( name );
  if( desc == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
                 "CachedResourceLoader: resource '", name,
                 "' not found in manifest" );
    return nullptr;
  }

  // 2. Check cache first
  auto cache_itr = this->cache_.find( name );
  if( cache_itr != this->cache_.end() && cache_itr->second.loaded )
  {
    out_loaded_from_cache = true;
    return cache_itr->second.resource;
  }

  // 3. Resolve full path
  const std::string full_path = this->resolve_internal_path( *desc );

  // 4. Verify file exists
  File fp;
  if( fp.exists( full_path ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
                 "CachedResourceLoader: resolved path for '", name,
                 "' does not exist: ", full_path );
    return nullptr;
  }

  // 5. Find suitable type loader
  ResourceFile::Type loader_type = desc->type;
  if( loader_type == ResourceFile::Type::Invalid )
  {
    // Fallback: try to find ANY registered loader if manifest didn't specify
    if( expected_type != ResourceFile::Type::Invalid )
    {
      loader_type = expected_type;
    }
    else
    {
      NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
                   "CachedResourceLoader: no type specified for '", name,
                   "' in manifest and no expected type provided" );
      return nullptr;
    }
  }

  IResourceTypeLoader* loader = this->find_loader( loader_type );
  if( loader == nullptr )
  {
    // For fonts, try the other font sub-type too
    if( loader_type == ResourceFile::Type::TrueTypeFont )
    {
      loader = this->find_loader( ResourceFile::Type::BitmapFont );
    }
    else if( loader_type == ResourceFile::Type::BitmapFont )
    {
      loader = this->find_loader( ResourceFile::Type::TrueTypeFont );
    }
  }

  if( loader == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
                 "CachedResourceLoader: no type loader registered for type ",
                 static_cast<int>( loader_type ),
                 " (resource: '", name, "')" );
    return nullptr;
  }

  // 6. Delegate to type loader
  void* resource = loader->load( full_path );
  if( resource == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
                 "CachedResourceLoader: type loader failed to load '", name,
                 "' from: ", full_path );
    return nullptr;
  }

  // 7. Insert into cache
  CacheEntry entry;
  entry.resource = resource;
  entry.loader = loader;
  entry.loaded = true;
  entry.descriptor = *desc;

  this->cache_[name] = entry;

  return resource;
}

// ---------------------------------------------------------------------------
// Resource query API
// ---------------------------------------------------------------------------

bool CachedResourceLoader::exists( const std::string& name ) const
{
  return this->manifest_.exists( name );
}

bool CachedResourceLoader::is_loaded( const std::string& name ) const
{
  std::lock_guard<std::mutex> lock( this->mutex_ );
  auto itr = this->cache_.find( name );
  return ( itr != this->cache_.end() && itr->second.loaded );
}

// ---------------------------------------------------------------------------
// Batch operations
// ---------------------------------------------------------------------------

nom::size_type CachedResourceLoader::preload(
  const std::vector<std::string>& names )
{
  nom::size_type loaded = 0;
  bool from_cache = false;

  for( auto itr = names.begin(); itr != names.end(); ++itr )
  {
    void* res = this->load_internal( *itr, ResourceFile::Type::Invalid,
                                      from_cache );
    if( res != nullptr )
    {
      ++loaded;
    }
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "CachedResourceLoader: preloaded ", loaded, "/",
                names.size(), " resources" );
  return loaded;
}

nom::size_type CachedResourceLoader::preload_all( void )
{
  std::vector<std::string> names;
  names.reserve( this->manifest_.size() );

  for( auto itr = this->manifest_.begin();
       itr != this->manifest_.end();
       ++itr )
  {
    names.push_back( itr->first );
  }

  return this->preload( names );
}

bool CachedResourceLoader::release( const std::string& name,
                                    ReleasePolicy policy )
{
  std::lock_guard<std::mutex> lock( this->mutex_ );

  auto itr = this->cache_.find( name );
  if( itr == this->cache_.end() )
  {
    return false;
  }

  CacheEntry& entry = itr->second;

  // Note: we don't track external shared_ptr references yet, so for now
  // UnusedOnly behaves identically to Force. A future enhancement could
  // wrap the returned pointer in a custom deleter that tracks refcounts
  // and defers unloading until the last reference is released.
  (void)policy;

  if( entry.loader != nullptr && entry.resource != nullptr )
  {
    entry.loader->unload( entry.resource );
  }

  this->cache_.erase( itr );

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "CachedResourceLoader: released '", name, "'" );
  return true;
}

nom::size_type CachedResourceLoader::clear( ReleasePolicy policy )
{
  std::lock_guard<std::mutex> lock( this->mutex_ );

  nom::size_type released = 0;

  for( auto itr = this->cache_.begin(); itr != this->cache_.end(); ++itr )
  {
    CacheEntry& entry = itr->second;
    if( entry.loader != nullptr && entry.resource != nullptr )
    {
      entry.loader->unload( entry.resource );
    }
    ++released;
  }

  this->cache_.clear();

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "CachedResourceLoader: cleared all ", released,
                " cached resource(s)" );
  (void)policy;
  return released;
}

nom::size_type CachedResourceLoader::size( void ) const
{
  std::lock_guard<std::mutex> lock( this->mutex_ );
  return this->cache_.size();
}

// ---------------------------------------------------------------------------
// Public convenience: resolve path by resource name
// ---------------------------------------------------------------------------

std::string CachedResourceLoader::resolve_path( const std::string& name ) const
{
  const ResourceDescriptor* desc = this->manifest_.find( name );
  if( desc == nullptr )
  {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                  "CachedResourceLoader::resolve_path: '", name,
                  "' not found in manifest" );
    return std::string();
  }
  return this->search_path_.resolve( desc->path );
}

// ---------------------------------------------------------------------------
// Debugging
// ---------------------------------------------------------------------------

void CachedResourceLoader::dump( void ) const
{
  std::lock_guard<std::mutex> lock( this->mutex_ );

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "CachedResourceLoader: ", this->cache_.size(),
                " cached resource(s):" );

  for( auto itr = this->cache_.begin(); itr != this->cache_.end(); ++itr )
  {
    const CacheEntry& entry = itr->second;
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                  "  - ", itr->first,
                  " [type=", static_cast<int>( entry.descriptor.type ),
                  "] -> ", this->resolve_internal_path( entry.descriptor ) );
  }
}

} // namespace nom
