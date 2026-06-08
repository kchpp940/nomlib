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
#ifndef NOMLIB_SYSTEM_IRESOURCE_TYPE_LOADER_HPP
#define NOMLIB_SYSTEM_IRESOURCE_TYPE_LOADER_HPP

#include <string>
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/system/ResourceFile.hpp"

namespace nom {

/// \brief Abstract interface for type-specific resource loading adapters.
///
/// \remarks Each module (graphics, audio, gui) implements this interface to
///          provide the **type-specific loading logic** for its resource
///          types. The loader is responsible ONLY for:
///
///          1. Taking an absolute file path and loading the raw resource
///             from disk into memory.
///          2. Reporting which ResourceFile::Type enumeration it handles.
///
///          It is **NOT** responsible for:
///          - Path resolution (handled by SearchPath)
///          - Resource name lookup (handled by ResourceManifest)
///          - Caching or lifecycle management (handled by CachedResourceLoader)
///          - File existence checks (handled by CachedResourceLoader)
///
/// This design enforces a clean separation of concerns:
///   ResourceManifest → describes what exists
///   SearchPath       → resolves where files are
///   IResourceTypeLoader → knows how to load a specific type
///   CachedResourceLoader → orchestrates everything and manages lifetimes
class IResourceTypeLoader
{
  public:
    typedef IResourceTypeLoader self_type;
    typedef self_type* raw_ptr;
    typedef std::unique_ptr<self_type> unique_ptr;
    typedef std::shared_ptr<self_type> shared_ptr;

    virtual ~IResourceTypeLoader( void )
    {
      NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM,
                          nom::NOM_LOG_PRIORITY_VERBOSE );
    }

    /// \brief Get the resource type that this loader handles.
    ///
    /// \returns One of the ResourceFile::Type enumeration values, e.g.
    ///          ResourceFile::Graphic, ResourceFile::Audio, etc.
    virtual ResourceFile::Type type( void ) const = 0;

    /// \brief Load a resource from an absolute file path.
    ///
    /// \param absolute_path The fully-resolved absolute path to the file.
    ///                      The file is guaranteed to exist before this
    ///                      method is called (CachedResourceLoader checks).
    ///
    /// \returns A raw pointer to the loaded resource on success, or nullptr
    ///          on failure. The caller (CachedResourceLoader) takes ownership
    ///          of the pointer and will manage its lifetime.
    ///
    /// \note  The returned pointer must be castable to the actual resource
    ///        type (e.g. Texture*, Image*, SoundBuffer*, Font*).
    virtual void* load( const std::string& absolute_path ) = 0;

    /// \brief Unload (destroy) a resource that was previously loaded by this
    ///        loader.
    ///
    /// \param resource A pointer previously returned by ::load.
    ///
    /// \remarks The default implementation performs a `delete` on the
    ///          pointer. Override this if your resource requires special
    ///          deallocation logic (e.g. audio buffers that need to be
    ///          released through an audio device API).
    virtual void unload( void* resource )
    {
      if( resource != nullptr )
      {
        // Default: assume the type can be safely deleted via its base
        // destructor. Subclasses should override for non-trivial cleanup.
        // We intentionally do NOT delete here because we don't know the
        // concrete type — subclasses MUST override.
        NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                      "IResourceTypeLoader::unload was not overridden for "
                      "type loader. Possible resource leak." );
      }
    }
};

} // namespace nom

#endif // include guard defined
