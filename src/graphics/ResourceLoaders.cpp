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
#include "nomlib/graphics/ResourceLoaders.hpp"

#include "nomlib/graphics/Texture.hpp"
#include "nomlib/graphics/Image.hpp"
#include "nomlib/graphics/fonts/Font.hpp"
#include "nomlib/graphics/sprite/SpriteSheet.hpp"
#include "nomlib/graphics/RenderWindow.hpp"
#include "nomlib/system/CachedResourceLoader.hpp"

namespace nom {

// ---------------------------------------------------------------------------
// TextureLoader
// ---------------------------------------------------------------------------

TextureLoader::~TextureLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_SYSTEM,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

ResourceFile::Type TextureLoader::type( void ) const
{
  return ResourceFile::Type::Graphic;
}

void* TextureLoader::load( const std::string& absolute_path )
{
  Texture* tex = new ( std::nothrow ) Texture();
  if( tex == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GRAPHICS,
                 "Failed to allocate Texture for:", absolute_path );
    return nullptr;
  }

  if( tex->load( absolute_path ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GRAPHICS,
                 "TextureLoader: failed to load:", absolute_path );
    delete tex;
    return nullptr;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_GRAPHICS,
                "TextureLoader: loaded:", absolute_path );
  return tex;
}

void TextureLoader::unload( void* resource )
{
  if( resource != nullptr )
  {
    Texture* tex = static_cast<Texture*>( resource );
    delete tex;
  }
}

// ---------------------------------------------------------------------------
// ImageLoader
// ---------------------------------------------------------------------------

ImageLoader::~ImageLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_SYSTEM,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

ResourceFile::Type ImageLoader::type( void ) const
{
  return ResourceFile::Type::Graphic;
}

void* ImageLoader::load( const std::string& absolute_path )
{
  Image* img = new ( std::nothrow ) Image();
  if( img == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GRAPHICS,
                 "Failed to allocate Image for:", absolute_path );
    return nullptr;
  }

  if( img->load( absolute_path ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GRAPHICS,
                 "ImageLoader: failed to load:", absolute_path );
    delete img;
    return nullptr;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_GRAPHICS,
                "ImageLoader: loaded:", absolute_path );
  return img;
}

void ImageLoader::unload( void* resource )
{
  if( resource != nullptr )
  {
    Image* img = static_cast<Image*>( resource );
    delete img;
  }
}

// ---------------------------------------------------------------------------
// FontLoader
// ---------------------------------------------------------------------------

FontLoader::~FontLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_SYSTEM,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

ResourceFile::Type FontLoader::type( void ) const
{
  return ResourceFile::Type::Invalid; // Handles both TrueType and Bitmap
}

void* FontLoader::load( const std::string& absolute_path )
{
  Font* font = new ( std::nothrow ) Font();
  if( font == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GRAPHICS,
                 "Failed to allocate Font for:", absolute_path );
    return nullptr;
  }

  if( font->load( absolute_path ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GRAPHICS,
                 "FontLoader: failed to load:", absolute_path );
    delete font;
    return nullptr;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_GRAPHICS,
                "FontLoader: loaded:", absolute_path );
  return font;
}

void FontLoader::unload( void* resource )
{
  if( resource != nullptr )
  {
    Font* font = static_cast<Font*>( resource );
    delete font;
  }
}

// ---------------------------------------------------------------------------
// SpriteSheetLoader
// ---------------------------------------------------------------------------

SpriteSheetLoader::~SpriteSheetLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_SYSTEM,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

ResourceFile::Type SpriteSheetLoader::type( void ) const
{
  return ResourceFile::Type::SpriteSheet;
}

void* SpriteSheetLoader::load( const std::string& absolute_path )
{
  SpriteSheet* sheet = new ( std::nothrow ) SpriteSheet();
  if( sheet == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GRAPHICS,
                 "Failed to allocate SpriteSheet for:", absolute_path );
    return nullptr;
  }

  if( sheet->load_file( absolute_path ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GRAPHICS,
                 "SpriteSheetLoader: failed to load:", absolute_path );
    delete sheet;
    return nullptr;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_GRAPHICS,
                "SpriteSheetLoader: loaded:", absolute_path );
  return sheet;
}

void SpriteSheetLoader::unload( void* resource )
{
  if( resource != nullptr )
  {
    SpriteSheet* sheet = static_cast<SpriteSheet*>( resource );
    delete sheet;
  }
}

// ===========================================================================
// Convenience free functions — adapt legacy path-only APIs
//
// IMPORTANT: These helpers must **never** bypass type validation. They call
// CachedResourceLoader::resolve_path(expected_type, id), which first checks
// that the manifest entry's type tag matches what the adapter expects,
// then returns the absolute file path. No "FilePath" pseudo-types, no
// std::string masquerading as a resource — the manifest type tag is the
// single source of truth.
// ===========================================================================

bool set_window_icon_from_resource( RenderWindow& window,
                                    CachedResourceLoader& loader,
                                    const std::string& resource_id )
{
  const std::string path = loader.resolve_path( ResourceFile::Graphic,
                                                resource_id );
  if( path.empty() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GRAPHICS,
                 "set_window_icon_from_resource: failed to resolve manifest ID:",
                 resource_id );
    return false;
  }
  return window.set_window_icon( path );
}

bool load_sprite_sheet_from_resource( SpriteSheet& sheet,
                                      CachedResourceLoader& loader,
                                      const std::string& resource_id )
{
  // Option 1 — use the SpriteSheetLoader directly, which gives us full
  // caching and lifecycle management through the CachedResourceLoader.
  SpriteSheet* cached = loader.load<SpriteSheet>( resource_id );
  if( cached != nullptr )
  {
    // Copy loaded state into the caller's instance.
    // NOTE: SpriteSheet doesn't have a deep copy; this covers the common
    // case where the caller just wants to load the file once.
    if( cached->is_valid() && ! sheet.is_valid() )
    {
      sheet = *cached;
    }
    return true;
  }

  // Option 2 — fall back to resolve_path with explicit SpriteSheet type tag.
  // Still goes through CachedResourceLoader's manifest lookup and type
  // validation — no bypass, no std::string pseudo-resources.
  const std::string path = loader.resolve_path( ResourceFile::SpriteSheet,
                                                resource_id );
  if( path.empty() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GRAPHICS,
                 "load_sprite_sheet_from_resource: failed to resolve manifest ID:",
                 resource_id );
    return false;
  }
  return sheet.load_file( path );
}

} // namespace nom
