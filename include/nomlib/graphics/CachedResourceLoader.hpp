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
#ifndef NOMLIB_GRAPHICS_CACHED_RESOURCE_LOADER_HPP
#define NOMLIB_GRAPHICS_CACHED_RESOURCE_LOADER_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "nomlib/config.hpp"
#include "nomlib/serializers/ResourceManifest.hpp"
#include "nomlib/graphics/Texture.hpp"
#include "nomlib/graphics/Image.hpp"
#include "nomlib/graphics/fonts/Font.hpp"
#include "nomlib/graphics/sprite/SpriteSheet.hpp"
#include "nomlib/graphics/ResourceLoader.hpp"

namespace nom {

/// \brief Manifest-aware resource loader with per-id caching, eager preload,
///        lazy load-on-first-access, and tag-based bulk load/release.
///
/// \remarks Texture preloading requires a current GL context (i.e. a
///          nom::RenderWindow made current).  Fonts, Images, and SpriteSheets
///          may be preloaded without a GL context.
class CachedResourceLoader
{
  public:
    typedef CachedResourceLoader self_type;

    CachedResourceLoader( void );
    explicit CachedResourceLoader( const ResourceManifest& manifest );

    ~CachedResourceLoader( void );

    CachedResourceLoader( const self_type& ) = delete;
    self_type& operator =( const self_type& ) = delete;

    CachedResourceLoader( self_type&& other ) noexcept;
    self_type& operator =( self_type&& other ) noexcept;

    /// \brief Attach a resource manifest (replaces any existing manifest and
    ///        clears all caches).
    void set_manifest( const ResourceManifest& manifest );

    /// \brief Access the attached manifest.
    const ResourceManifest& manifest( void ) const;

    // -----------------------------------------------------------------------
    // Lazy cached access
    // -----------------------------------------------------------------------

    /// \brief Get a cached Texture, loading it on first access.
    ///
    /// \returns shared_ptr to the Texture, or nullptr on load failure.
    std::shared_ptr<Texture>
    get_texture( const std::string& id,
                 bool use_cache = false,
                 Texture::Access type = Texture::Access::Static );

    /// \brief Get a cached Image, loading it on first access.
    std::shared_ptr<Image>
    get_image( const std::string& id,
               uint32 pixel_format = SDL_PIXELFORMAT_ARGB8888 );

    /// \brief Get a cached Font, loading it on first access.
    std::shared_ptr<Font>
    get_font( const std::string& id );

    /// \brief Get a cached SpriteSheet descriptor, loading it on first access.
    std::shared_ptr<SpriteSheet>
    get_spritesheet( const std::string& id );

    // -----------------------------------------------------------------------
    // Preload
    // -----------------------------------------------------------------------

    /// \brief Preload all manifest entries marked preload=Eager.
    ///
    /// \returns true if ALL eager resources loaded successfully; false if any
    ///          failed (individual errors are logged).
    bool preload_eager( void );

    /// \brief Preload all manifest entries carrying a given tag.
    ///
    /// \returns Count of resources successfully loaded (duplicates already in
    ///          cache are skipped and still counted).
    nom::size_type
    preload_by_tag( const std::string& tag );

    // -----------------------------------------------------------------------
    // Release
    // -----------------------------------------------------------------------

    /// \brief Release a single cached resource by id.
    ///
    /// \returns true if the resource was found and released.
    bool release( const std::string& id );

    /// \brief Release all cached resources whose manifest entry carries tag.
    ///
    /// \returns Number of resources released.
    nom::size_type
    release_by_tag( const std::string& tag );

    /// \brief Release ALL cached resources (manifests remain attached).
    void clear( void );

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    /// \brief Whether a resource id is present in any cache.
    bool is_loaded( const std::string& id ) const;

    /// \brief Total number of resources currently cached.
    nom::size_type loaded_count( void ) const;

    /// \brief Debug dump of all cached resources.
    void dump( void ) const;

  private:
    /// \brief Helper: true if id is present in any cache map.
    bool id_in_any_cache( const std::string& id ) const;

    /// \brief Helper: get all entry ids with tag, across all types.
    std::vector<std::string> ids_by_tag( const std::string& tag ) const;

    ResourceManifest manifest_;

    std::map<std::string, std::shared_ptr<Texture>>     cache_textures_;
    std::map<std::string, std::shared_ptr<Image>>       cache_images_;
    std::map<std::string, std::shared_ptr<Font>>        cache_fonts_;
    std::map<std::string, std::shared_ptr<SpriteSheet>> cache_spritesheets_;
};

} // namespace nom

#endif // include guard defined

// Inline implementation ------------------------------------------------------

namespace nom {

inline CachedResourceLoader::CachedResourceLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

inline CachedResourceLoader::CachedResourceLoader( const ResourceManifest& manifest ) :
  manifest_( manifest )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

inline CachedResourceLoader::~CachedResourceLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
  this->clear();
}

inline CachedResourceLoader::CachedResourceLoader( self_type&& other ) noexcept :
  manifest_( std::move( other.manifest_ ) ),
  cache_textures_( std::move( other.cache_textures_ ) ),
  cache_images_( std::move( other.cache_images_ ) ),
  cache_fonts_( std::move( other.cache_fonts_ ) ),
  cache_spritesheets_( std::move( other.cache_spritesheets_ ) )
{
}

inline CachedResourceLoader::self_type&
CachedResourceLoader::operator =( self_type&& other ) noexcept
{
  if( this != &other )
  {
    this->clear();
    this->manifest_ = std::move( other.manifest_ );
    this->cache_textures_ = std::move( other.cache_textures_ );
    this->cache_images_ = std::move( other.cache_images_ );
    this->cache_fonts_ = std::move( other.cache_fonts_ );
    this->cache_spritesheets_ = std::move( other.cache_spritesheets_ );
  }
  return *this;
}

inline void
CachedResourceLoader::set_manifest( const ResourceManifest& manifest )
{
  this->clear();
  this->manifest_ = manifest;
}

inline const ResourceManifest&
CachedResourceLoader::manifest( void ) const
{
  return this->manifest_;
}

inline std::shared_ptr<Texture>
CachedResourceLoader::get_texture( const std::string& id,
                                   bool use_cache,
                                   Texture::Access type )
{
  auto itr = this->cache_textures_.find( id );
  if( itr != this->cache_textures_.end() )
  {
    return itr->second;
  }

  auto tex = std::make_shared<Texture>();
  if( nom::load_texture( this->manifest_, id, *tex, use_cache, type ) == false )
  {
    return nullptr;
  }
  this->cache_textures_[id] = tex;
  return tex;
}

inline std::shared_ptr<Image>
CachedResourceLoader::get_image( const std::string& id, uint32 pixel_format )
{
  auto itr = this->cache_images_.find( id );
  if( itr != this->cache_images_.end() )
  {
    return itr->second;
  }

  auto img = std::make_shared<Image>();
  if( nom::load_image( this->manifest_, id, *img, pixel_format ) == false )
  {
    return nullptr;
  }
  this->cache_images_[id] = img;
  return img;
}

inline std::shared_ptr<Font>
CachedResourceLoader::get_font( const std::string& id )
{
  auto itr = this->cache_fonts_.find( id );
  if( itr != this->cache_fonts_.end() )
  {
    return itr->second;
  }

  auto font = std::make_shared<Font>();
  if( nom::load_font( this->manifest_, id, *font ) == false )
  {
    return nullptr;
  }
  this->cache_fonts_[id] = font;
  return font;
}

inline std::shared_ptr<SpriteSheet>
CachedResourceLoader::get_spritesheet( const std::string& id )
{
  auto itr = this->cache_spritesheets_.find( id );
  if( itr != this->cache_spritesheets_.end() )
  {
    return itr->second;
  }

  auto sheet = std::make_shared<SpriteSheet>();
  if( nom::load_spritesheet( this->manifest_, id, *sheet ) == false )
  {
    return nullptr;
  }
  this->cache_spritesheets_[id] = sheet;
  return sheet;
}

inline bool
CachedResourceLoader::preload_eager( void )
{
  bool all_ok = true;

  auto textures  = this->manifest_.find_by_type( ResourceType::Texture );
  auto images    = this->manifest_.find_by_type( ResourceType::Image );
  auto fonts     = this->manifest_.find_by_type( ResourceType::Font );
  auto sheets    = this->manifest_.find_by_type( ResourceType::SpriteSheet );

  auto try_load = [&]( const ResourceManifestEntry& e )
  {
    if( e.preload() != PreloadStrategy::Eager ) return;
    if( this->is_loaded( e.id() ) ) return;

    switch( e.type() )
    {
      case ResourceType::Texture:
        if( this->get_texture( e.id() ) == nullptr ) all_ok = false;
        break;
      case ResourceType::Image:
        if( this->get_image( e.id() ) == nullptr ) all_ok = false;
        break;
      case ResourceType::Font:
        if( this->get_font( e.id() ) == nullptr ) all_ok = false;
        break;
      case ResourceType::SpriteSheet:
        if( this->get_spritesheet( e.id() ) == nullptr ) all_ok = false;
        break;
      default: break;
    }
  };

  for( const auto& e : textures )  try_load( e );
  for( const auto& e : images )    try_load( e );
  for( const auto& e : fonts )     try_load( e );
  for( const auto& e : sheets )    try_load( e );

  return all_ok;
}

inline nom::size_type
CachedResourceLoader::preload_by_tag( const std::string& tag )
{
  nom::size_type count = 0;
  auto entries = this->manifest_.find_by_tag( tag );

  for( const auto& e : entries )
  {
    if( this->is_loaded( e.id() ) ) { ++count; continue; }

    bool loaded = false;
    switch( e.type() )
    {
      case ResourceType::Texture:
        loaded = ( this->get_texture( e.id() ) != nullptr );
        break;
      case ResourceType::Image:
        loaded = ( this->get_image( e.id() ) != nullptr );
        break;
      case ResourceType::Font:
        loaded = ( this->get_font( e.id() ) != nullptr );
        break;
      case ResourceType::SpriteSheet:
        loaded = ( this->get_spritesheet( e.id() ) != nullptr );
        break;
      default: break;
    }
    if( loaded ) ++count;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "CachedResourceLoader::preload_by_tag('", tag, "') loaded ",
                count, "/", entries.size(), " resources." );
  return count;
}

inline bool
CachedResourceLoader::release( const std::string& id )
{
  if( this->cache_textures_.erase( id ) > 0 )     return true;
  if( this->cache_images_.erase( id ) > 0 )       return true;
  if( this->cache_fonts_.erase( id ) > 0 )        return true;
  if( this->cache_spritesheets_.erase( id ) > 0 ) return true;
  return false;
}

inline nom::size_type
CachedResourceLoader::release_by_tag( const std::string& tag )
{
  nom::size_type count = 0;
  auto ids = this->ids_by_tag( tag );
  for( const auto& id : ids )
  {
    if( this->release( id ) ) ++count;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "CachedResourceLoader::release_by_tag('", tag,
                "') released ", count, " resources." );
  return count;
}

inline void
CachedResourceLoader::clear( void )
{
  this->cache_textures_.clear();
  this->cache_images_.clear();
  this->cache_fonts_.clear();
  this->cache_spritesheets_.clear();
}

inline bool
CachedResourceLoader::is_loaded( const std::string& id ) const
{
  return this->id_in_any_cache( id );
}

inline nom::size_type
CachedResourceLoader::loaded_count( void ) const
{
  return this->cache_textures_.size() +
         this->cache_images_.size() +
         this->cache_fonts_.size() +
         this->cache_spritesheets_.size();
}

inline void
CachedResourceLoader::dump( void ) const
{
  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "CachedResourceLoader (", this->loaded_count(), " cached):" );
  for( const auto& kv : this->cache_textures_ ) {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM, "  [texture] ", kv.first );
  }
  for( const auto& kv : this->cache_images_ ) {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM, "  [image]   ", kv.first );
  }
  for( const auto& kv : this->cache_fonts_ ) {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM, "  [font]    ", kv.first );
  }
  for( const auto& kv : this->cache_spritesheets_ ) {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM, "  [sheet]   ", kv.first );
  }
}

inline bool
CachedResourceLoader::id_in_any_cache( const std::string& id ) const
{
  if( this->cache_textures_.count( id ) > 0 )     return true;
  if( this->cache_images_.count( id ) > 0 )       return true;
  if( this->cache_fonts_.count( id ) > 0 )        return true;
  if( this->cache_spritesheets_.count( id ) > 0 ) return true;
  return false;
}

inline std::vector<std::string>
CachedResourceLoader::ids_by_tag( const std::string& tag ) const
{
  std::vector<std::string> result;
  auto entries = this->manifest_.find_by_tag( tag );
  for( const auto& e : entries ) result.push_back( e.id() );
  return result;
}

} // namespace nom
