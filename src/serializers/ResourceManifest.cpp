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
#include "nomlib/serializers/ResourceManifest.hpp"

#include <algorithm>

#include "nomlib/system/File.hpp"
#include "nomlib/ptree/Value.hpp"
#include "nomlib/serializers/JsonCppDeserializer.hpp"
#include "nomlib/serializers/IValueDeserializer.hpp"

namespace nom {

// ---------------------------------------------------------------------------
// ResourceManifestEntry
// ---------------------------------------------------------------------------

const ResourceManifestEntry ResourceManifest::null_entry;

ResourceManifestEntry::ResourceManifestEntry( void ) :
  id_( "\0" ),
  type_( ResourceType::Invalid ),
  path_( "\0" ),
  preload_( PreloadStrategy::Lazy )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

ResourceManifestEntry::~ResourceManifestEntry( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

ResourceManifestEntry::ResourceManifestEntry(
  const std::string& id,
  ResourceType type,
  const std::string& path,
  const std::vector<std::string>& tags,
  PreloadStrategy preload
) :
  id_( id ),
  type_( type ),
  path_( path ),
  tags_( tags ),
  preload_( preload )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

bool ResourceManifestEntry::valid( void ) const
{
  return (
    ! this->id_.empty() &&
    this->type_ != ResourceType::Invalid &&
    ! this->path_.empty()
  );
}

const std::string& ResourceManifestEntry::id( void ) const
{
  return this->id_;
}

ResourceType ResourceManifestEntry::type( void ) const
{
  return this->type_;
}

const std::string& ResourceManifestEntry::path( void ) const
{
  return this->path_;
}

const std::vector<std::string>& ResourceManifestEntry::tags( void ) const
{
  return this->tags_;
}

PreloadStrategy ResourceManifestEntry::preload( void ) const
{
  return this->preload_;
}

void ResourceManifestEntry::set_id( const std::string& id )
{
  this->id_ = id;
}

void ResourceManifestEntry::set_type( ResourceType type )
{
  this->type_ = type;
}

void ResourceManifestEntry::set_path( const std::string& path )
{
  this->path_ = path;
}

void ResourceManifestEntry::set_tags( const std::vector<std::string>& tags )
{
  this->tags_ = tags;
}

void ResourceManifestEntry::set_preload( PreloadStrategy strategy )
{
  this->preload_ = strategy;
}

bool ResourceManifestEntry::has_tag( const std::string& tag ) const
{
  return std::find( this->tags_.begin(), this->tags_.end(), tag ) !=
         this->tags_.end();
}

// ---------------------------------------------------------------------------
// ResourceManifest
// ---------------------------------------------------------------------------

ResourceManifest::ResourceManifest( void ) :
  fp_( nullptr )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

ResourceManifest::~ResourceManifest( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

ResourceManifest::ResourceManifest( const self_type& other ) :
  entries_( other.entries_ ),
  base_path_( other.base_path_ ),
  fp_( nullptr )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

ResourceManifest::self_type&
ResourceManifest::operator =( const self_type& other )
{
  if( this != &other )
  {
    this->entries_ = other.entries_;
    this->base_path_ = other.base_path_;
    this->fp_.reset( nullptr );
  }
  return *this;
}

ResourceManifest::ResourceManifest( self_type&& other ) noexcept :
  entries_( std::move( other.entries_ ) ),
  base_path_( std::move( other.base_path_ ) ),
  fp_( std::move( other.fp_ ) )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

ResourceManifest::self_type&
ResourceManifest::operator =( self_type&& other ) noexcept
{
  if( this != &other )
  {
    this->entries_ = std::move( other.entries_ );
    this->base_path_ = std::move( other.base_path_ );
    this->fp_ = std::move( other.fp_ );
  }
  return *this;
}

std::string ResourceManifest::type_to_string( ResourceType type )
{
  switch( type )
  {
    default:
    case ResourceType::Invalid:     return "invalid";
    case ResourceType::Texture:     return "texture";
    case ResourceType::Image:       return "image";
    case ResourceType::Audio:       return "audio";
    case ResourceType::Font:        return "font";
    case ResourceType::UI:          return "ui";
    case ResourceType::SpriteSheet: return "spritesheet";
    case ResourceType::Config:      return "config";
  }
}

ResourceType ResourceManifest::string_to_type( const std::string& str )
{
  if( str == "texture" )     return ResourceType::Texture;
  if( str == "image" )       return ResourceType::Image;
  if( str == "audio" )       return ResourceType::Audio;
  if( str == "font" )        return ResourceType::Font;
  if( str == "ui" )          return ResourceType::UI;
  if( str == "spritesheet" ) return ResourceType::SpriteSheet;
  if( str == "config" )      return ResourceType::Config;
  return ResourceType::Invalid;
}

PreloadStrategy ResourceManifest::string_to_preload( const std::string& str )
{
  if( str == "eager" ) return PreloadStrategy::Eager;
  if( str == "lazy" )  return PreloadStrategy::Lazy;
  if( str == "none" )  return PreloadStrategy::None;
  return PreloadStrategy::Lazy;
}

void ResourceManifest::set_deserializer( std::unique_ptr<IValueDeserializer> fp )
{
  this->fp_ = std::move( fp );
}

const std::string& ResourceManifest::base_path( void ) const
{
  return this->base_path_;
}

bool ResourceManifest::resolve_base_path(
  const std::vector<std::string>& search_prefix,
  const std::string& path
)
{
  File dir;
  std::vector<std::string> candidates;

  for( const auto& prefix : search_prefix )
  {
    candidates.push_back( prefix + path );
  }

  for( const auto& candidate : candidates )
  {
    if( dir.exists( candidate ) )
    {
      this->base_path_ = candidate;
      NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                    "ResourceManifest using base path:", this->base_path_ );
      return true;
    }
    else
    {
      NOM_LOG_DEBUG( NOM_LOG_CATEGORY_SYSTEM,
                     "ResourceManifest skipping non-existent path:",
                     candidate );
    }
  }

  NOM_LOG_CRIT( NOM_LOG_CATEGORY_APPLICATION,
                "ResourceManifest could not resolve base path; tried:" );
  for( const auto& c : candidates )
  {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM, "  ", c );
  }
  return false;
}

bool ResourceManifest::load_file(
  const std::string& manifest_path,
  const std::string& manifest_node
)
{
  Value root;

  if( this->fp_ == nullptr )
  {
    this->fp_.reset( new JsonCppDeserializer() );
  }

  if( this->fp_->load( manifest_path, root ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "ResourceManifest could not load file:", manifest_path );
    return false;
  }

  // Resolve base path (using manifest_path's directory or explicit node)
  {
    std::vector<std::string> search_prefix = { "./", "../", "../../", "../../../" };
    std::string base = "";

    if( ! root["search_prefix"].null_type() && root["search_prefix"].array_type() )
    {
      search_prefix.clear();
      for( auto itr = root["search_prefix"].begin();
           itr != root["search_prefix"].end(); ++itr )
      {
        search_prefix.push_back( (*itr).get_string() );
      }
    }

    if( ! root["base_path"].null_type() && root["base_path"].string_type() )
    {
      base = root["base_path"].get_string();
    }

    if( this->resolve_base_path( search_prefix, base ) == false )
    {
      NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                    "ResourceManifest base path unresolved; entries must use "
                    "absolute paths." );
    }
  }

  // Sanity check the manifest node
  if( root[manifest_node].null_type() )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_APPLICATION,
                  "ResourceManifest top-level node '", manifest_node,
                  "' is not defined." );
    return false;
  }

  if( ! root[manifest_node].array_type() )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_APPLICATION,
                  "ResourceManifest node '", manifest_node,
                  "' must be an array." );
    return false;
  }

  const Value& items = root[manifest_node];
  nom::size_type count = 0;

  for( auto itr = items.begin(); itr != items.end(); ++itr )
  {
    const Value& item = *itr;

    if( ! item.object_type() )
    {
      NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                    "ResourceManifest skipping non-object entry at index",
                    count );
      continue;
    }

    if( item["id"].null_type() || ! item["id"].string_type() )
    {
      NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                    "ResourceManifest skipping entry without string 'id'." );
      continue;
    }

    if( item["type"].null_type() || ! item["type"].string_type() )
    {
      NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                    "ResourceManifest skipping entry '",
                    item["id"].get_string(),
                    "' without string 'type'." );
      continue;
    }

    if( item["path"].null_type() || ! item["path"].string_type() )
    {
      NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                    "ResourceManifest skipping entry '",
                    item["id"].get_string(),
                    "' without string 'path'." );
      continue;
    }

    std::string id = item["id"].get_string();
    ResourceType type = string_to_type( item["type"].get_string() );
    std::string path = item["path"].get_string();

    if( type == ResourceType::Invalid )
    {
      NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                    "ResourceManifest entry '", id,
                    "' has unknown type '", item["type"].get_string(),
                    "'; skipping." );
      continue;
    }

    std::vector<std::string> tags;
    if( ! item["tags"].null_type() && item["tags"].array_type() )
    {
      for( auto t = item["tags"].begin(); t != item["tags"].end(); ++t )
      {
        tags.push_back( (*t).get_string() );
      }
    }

    PreloadStrategy preload = PreloadStrategy::Lazy;
    if( ! item["preload"].null_type() && item["preload"].string_type() )
    {
      preload = string_to_preload( item["preload"].get_string() );
    }
    else if( ! item["preload"].null_type() && item["preload"].bool_type() )
    {
      preload = item["preload"].get_bool() ?
        PreloadStrategy::Eager : PreloadStrategy::Lazy;
    }

    ResourceManifestEntry entry( id, type, path, tags, preload );

    if( this->entries_.count( id ) > 0 )
    {
      NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                    "ResourceManifest overwriting duplicate id '", id, "'." );
      this->entries_[id] = entry;
    }
    else
    {
      this->entries_.emplace( id, entry );
    }

    ++count;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "ResourceManifest loaded", count,
                "entries from:", manifest_path );
  return true;
}

bool ResourceManifest::exists( const std::string& id ) const
{
  return this->entries_.count( id ) > 0;
}

const ResourceManifestEntry& ResourceManifest::find( const std::string& id ) const
{
  auto itr = this->entries_.find( id );
  if( itr != this->entries_.end() )
  {
    return itr->second;
  }
  return ResourceManifest::null_entry;
}

std::vector<ResourceManifestEntry>
ResourceManifest::find_by_type( ResourceType type ) const
{
  std::vector<ResourceManifestEntry> result;
  for( const auto& pair : this->entries_ )
  {
    if( pair.second.type() == type )
    {
      result.push_back( pair.second );
    }
  }
  return result;
}

std::vector<ResourceManifestEntry>
ResourceManifest::find_by_tag( const std::string& tag ) const
{
  std::vector<ResourceManifestEntry> result;
  for( const auto& pair : this->entries_ )
  {
    if( pair.second.has_tag( tag ) )
    {
      result.push_back( pair.second );
    }
  }
  return result;
}

std::string ResourceManifest::resolve_path( const std::string& id ) const
{
  const ResourceManifestEntry& entry = this->find( id );
  if( ! entry.valid() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "ResourceManifest::resolve_path - unknown id:", id );
    return "\0";
  }

  File fp;
  std::string candidate = entry.path();

  if( fp.is_file( candidate ) )
  {
    return candidate;
  }

  if( ! this->base_path_.empty() )
  {
    candidate = this->base_path_ + entry.path();
    if( fp.is_file( candidate ) )
    {
      return candidate;
    }
  }

  NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                "ResourceManifest could not resolve file for id '",
                id, "' at '", entry.path(), "'" );
  return entry.path();
}

nom::size_type ResourceManifest::size( void ) const
{
  return this->entries_.size();
}

void ResourceManifest::clear( void )
{
  this->entries_.clear();
  this->base_path_.clear();
}

void ResourceManifest::dump( void ) const
{
  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "ResourceManifest (", this->entries_.size(),
                " entries, base_path='", this->base_path_, "'):" );
  for( const auto& pair : this->entries_ )
  {
    const auto& e = pair.second;
    std::string tags;
    for( const auto& t : e.tags() )
    {
      if( ! tags.empty() ) tags += ", ";
      tags += t;
    }
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                  "  [", type_to_string( e.type() ), "] ",
                  e.id(), " -> ", e.path(),
                  ( tags.empty() ? "" : " (" + tags + ")" ) );
  }
}

} // namespace nom
