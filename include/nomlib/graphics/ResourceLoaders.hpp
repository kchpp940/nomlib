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
#ifndef NOMLIB_GRAPHICS_RESOURCE_LOADERS_HPP
#define NOMLIB_GRAPHICS_RESOURCE_LOADERS_HPP

#include <string>
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/system/IResourceTypeLoader.hpp"
#include "nomlib/system/ResourceFile.hpp"

namespace nom {

// Forward declarations
class Texture;
class Image;
class Font;
class SpriteSheet;
class RenderWindow;
class CachedResourceLoader;

/// \brief Resource type loader for nom::Texture (graphic images).
///
/// \remarks This adapter handles **only** the type-specific loading logic for
///          Texture objects. It does not deal with path resolution, caching,
///          or resource name lookup.
class TextureLoader : public IResourceTypeLoader
{
  public:
    typedef TextureLoader self_type;

    virtual ~TextureLoader( void );

    virtual ResourceFile::Type type( void ) const override;

    /// \brief Load a Texture from an absolute file path.
    ///
    /// \returns A raw pointer to a new nom::Texture on success, or nullptr
    ///          on failure. The caller takes ownership.
    virtual void* load( const std::string& absolute_path ) override;

    /// \brief Destroy a Texture previously loaded by this loader.
    virtual void unload( void* resource ) override;
};

/// \brief Resource type loader for nom::Image (bitmap surfaces).
///
/// \remarks Use this when you need raw pixel data (CPU-side) rather than a
///          GPU-uploaded texture.
class ImageLoader : public IResourceTypeLoader
{
  public:
    typedef ImageLoader self_type;

    virtual ~ImageLoader( void );

    virtual ResourceFile::Type type( void ) const override;

    virtual void* load( const std::string& absolute_path ) override;

    virtual void unload( void* resource ) override;
};

/// \brief Resource type loader for nom::Font (both bitmap and TrueType).
///
/// \remarks The loader internally uses Font::load() which auto-detects the
///          font format from the file extension.
class FontLoader : public IResourceTypeLoader
{
  public:
    typedef FontLoader self_type;

    virtual ~FontLoader( void );

    virtual ResourceFile::Type type( void ) const override;

    virtual void* load( const std::string& absolute_path ) override;

    virtual void unload( void* resource ) override;
};

/// \brief Resource type loader for nom::SpriteSheet JSON descriptors.
///
/// \remarks Loads a SpriteSheet from its JSON descriptor file via
///          SpriteSheet::load_file(). The resource type tag is
///          ResourceFile::SpriteSheet.
class SpriteSheetLoader : public IResourceTypeLoader
{
  public:
    typedef SpriteSheetLoader self_type;

    virtual ~SpriteSheetLoader( void );

    virtual ResourceFile::Type type( void ) const override;

    virtual void* load( const std::string& absolute_path ) override;

    virtual void unload( void* resource ) override;
};

// ============================================================================
// Convenience helpers — wrap "legacy" APIs that only consume raw file paths.
//
// These free functions look up a resource by manifest ID through the
// CachedResourceLoader, resolve the absolute path, and hand it off to the
// underlying API (RenderWindow, SpriteSheet, etc.). They exist so that
// application-level code (examples, games) never needs to call
// CachedResourceLoader::resolve_path() directly.
// ============================================================================

/// \brief Set a RenderWindow icon from a resource manifest ID.
///
/// \returns TRUE if the icon was found in the manifest, resolved, and
///          successfully applied to the window.
bool set_window_icon_from_resource( RenderWindow& window,
                                    CachedResourceLoader& loader,
                                    const std::string& resource_id );

/// \brief Load a SpriteSheet from its JSON descriptor, looked up by manifest
///        ID.
///
/// \returns TRUE if the resource was found, resolved, and loaded into the
///          SpriteSheet instance.
bool load_sprite_sheet_from_resource( SpriteSheet& sheet,
                                      CachedResourceLoader& loader,
                                      const std::string& resource_id );

} // namespace nom

#endif // include guard defined
