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
#ifndef NOMLIB_GRAPHICS_RESOURCE_LOADER_HPP
#define NOMLIB_GRAPHICS_RESOURCE_LOADER_HPP

#include <string>

#include "nomlib/config.hpp"
#include "nomlib/serializers/ResourceManifest.hpp"
#include "nomlib/graphics/Texture.hpp"
#include "nomlib/graphics/Image.hpp"
#include "nomlib/graphics/fonts/Font.hpp"
#include "nomlib/graphics/sprite/SpriteSheet.hpp"

namespace nom {

/// \brief Load a Texture from a ResourceManifest entry.
///
/// \param manifest  The manifest to look up the id from.
/// \param id        The manifest resource id (type must be Texture or Image).
/// \param tex       Output Texture to populate.
/// \param use_cache Whether to use internal texture caching.
/// \param type      Texture access mode.
///
/// \returns true on success, false on failure (id missing, type mismatch, or
///          underlying Texture::load failure).
inline bool
load_texture(  const ResourceManifest& manifest,
               const std::string& id,
               Texture& tex,
               bool use_cache = false,
               Texture::Access type = Texture::Access::Static )
{
  const ResourceManifestEntry& entry = manifest.find( id );
  if( ! entry.valid() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_texture: unknown manifest id:", id );
    return false;
  }
  if( entry.type() != ResourceType::Texture &&
      entry.type() != ResourceType::Image )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_texture: id '", id, "' has type '",
                 ResourceManifest::type_to_string( entry.type() ),
                 "', expected 'texture' or 'image'." );
    return false;
  }

  std::string path = manifest.resolve_path( id );
  if( tex.load( path, use_cache, type ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_texture: failed to load '", path, "'" );
    return false;
  }
  return true;
}

/// \brief Load an Image from a ResourceManifest entry.
inline bool
load_image( const ResourceManifest& manifest,
            const std::string& id,
            Image& img,
            uint32 pixel_format = SDL_PIXELFORMAT_ARGB8888 )
{
  const ResourceManifestEntry& entry = manifest.find( id );
  if( ! entry.valid() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_image: unknown manifest id:", id );
    return false;
  }
  if( entry.type() != ResourceType::Image &&
      entry.type() != ResourceType::Texture )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_image: id '", id, "' has type '",
                 ResourceManifest::type_to_string( entry.type() ),
                 "', expected 'image' or 'texture'." );
    return false;
  }

  std::string path = manifest.resolve_path( id );
  if( img.load( path, pixel_format ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_image: failed to load '", path, "'" );
    return false;
  }
  return true;
}

/// \brief Load a Font from a ResourceManifest entry.
inline bool
load_font(  const ResourceManifest& manifest,
            const std::string& id,
            Font& font )
{
  const ResourceManifestEntry& entry = manifest.find( id );
  if( ! entry.valid() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_font: unknown manifest id:", id );
    return false;
  }
  if( entry.type() != ResourceType::Font )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_font: id '", id, "' has type '",
                 ResourceManifest::type_to_string( entry.type() ),
                 "', expected 'font'." );
    return false;
  }

  std::string path = manifest.resolve_path( id );
  if( font.load( path ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_font: failed to load '", path, "'" );
    return false;
  }
  return true;
}

/// \brief Load a SpriteSheet descriptor from a ResourceManifest entry.
inline bool
load_spritesheet( const ResourceManifest& manifest,
                  const std::string& id,
                  SpriteSheet& sheet )
{
  const ResourceManifestEntry& entry = manifest.find( id );
  if( ! entry.valid() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_spritesheet: unknown manifest id:", id );
    return false;
  }
  if( entry.type() != ResourceType::SpriteSheet )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_spritesheet: id '", id, "' has type '",
                 ResourceManifest::type_to_string( entry.type() ),
                 "', expected 'spritesheet'." );
    return false;
  }

  std::string path = manifest.resolve_path( id );
  if( sheet.load_file( path ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_spritesheet: failed to load '", path, "'" );
    return false;
  }
  return true;
}

} // namespace nom

#endif // include guard defined
