/******************************************************************************

  nomlib - C++11 cross-platform game engine

Copyright (c) 2013, 2014, 2015, 2016 Jeffrey Carpenter <i8degrees@gmail.com>
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
#ifndef NOMLIB_SYSTEM_RESOURCE_LOADERS_HPP
#define NOMLIB_SYSTEM_RESOURCE_LOADERS_HPP

#include <string>
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/system/IResourceTypeLoader.hpp"
#include "nomlib/system/ResourceFile.hpp"

namespace nom {

/// \brief A catch-all resource type loader that returns the resolved absolute
///        file path as a heap-allocated std::string.
///
/// \remarks This loader exists as a **last-resort adapter** for legacy APIs
///          that consume raw file paths instead of loaded resource objects.
///          Whenever a higher-level adapter (TextureLoader, AudioBufferLoader,
///          etc.) is available, prefer it over this one.
///
///          Unlike the other type loaders, this one does not actually "load"
///          the file contents — it simply caches the resolved absolute path
///          string so that repeated lookups of the same resource ID do not
///          re-run the manifest search + SearchPath resolution.
///
///          ### Type tag in manifest:
///          Any type may be matched by FilePathLoader because it is a fall-
///          through adapter. The explicit tag is `"type": "FilePath"`.
///
///          ### Usage:
///          \code
///            loader.register_type_loader(std::make_unique<ResourceFilePathLoader>());
///            std::string* path = loader.load<std::string>("my_icon");
///            window.set_window_icon(*path); // legacy API needs raw path
///          \endcode
class ResourceFilePathLoader : public IResourceTypeLoader
{
  public:
    typedef ResourceFilePathLoader self_type;

    virtual ~ResourceFilePathLoader( void );

    /// \brief Returns ResourceFile::Type::FilePath.
    virtual ResourceFile::Type type( void ) const override;

    /// \brief "Load" a resource by returning its absolute file path as a
    ///        heap-allocated std::string.
    ///
    /// \returns A pointer to a new std::string containing the absolute path
    ///          (same as the input parameter), or nullptr if the path is
    ///          empty.
    ///
    /// \remarks The file itself is not opened or read. This loader is purely
    ///          a path caching mechanism for legacy APIs.
    virtual void* load( const std::string& absolute_path ) override;

    /// \brief Destroy a previously-loaded path string.
    virtual void unload( void* resource ) override;
};

} // namespace nom

#endif // include guard defined
