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
#ifndef NOMLIB_GUI_RESOURCE_LOADERS_HPP
#define NOMLIB_GUI_RESOURCE_LOADERS_HPP

#include "nomlib/config.hpp"
#include "nomlib/system/IResourceTypeLoader.hpp"

namespace nom {

// Forward declarations
class UIContext;
class UIWidget;
class CachedResourceLoader;

/// \brief Resource type loader adapter for GUI / libRocket document files
///        (RML / RCSS).
///
/// \remarks In practice, libRocket loads most of its own assets through its
///          own Rocket::Core::FileInterface. This adapter exists primarily
///          for symmetry with the graphics and audio modules, so that
///          CachedResourceLoader can serve as a single, unified entry point
///          for resource discovery. It also enables preloading and caching
///          of raw document strings when desired.
///
///          For most GUI use cases, you will want to register a
///          nom::RocketFileInterface (which wraps a SearchPath or
///          CachedResourceLoader) with libRocket instead.
///
///          ### Type tag in manifest:
///          - `"type": "GuiDocument"` — loads a raw file as a string buffer.
class GuiDocumentLoader : public IResourceTypeLoader
{
  public:
    GuiDocumentLoader();
    virtual ~GuiDocumentLoader();

    /// \brief Returns ResourceFile::Type::GuiDocument.
    ResourceFile::Type type() const override;

    /// \brief Load a file from disk as a raw std::string buffer.
    ///
    /// \returns A pointer to a heap-allocated std::string containing the
    ///          file contents, or nullptr on failure.
    ///
    /// \remarks The caller is responsible for `delete`ing the returned
    ///          pointer, or letting CachedResourceLoader manage its lifetime.
    void* load( const std::string& absolute_path ) override;

    /// \brief Delete a previously-loaded std::string buffer.
    void unload( void* resource ) override;
};

// ============================================================================
// Convenience helpers — wrap libRocket path-string-only APIs.
//
// These free functions look up a resource by manifest ID through the
// CachedResourceLoader, resolve the absolute path, and hand it off to the
// underlying libRocket API (UIContext::load_font, UIWidget::load_document_file).
// They exist so that application-level code (examples, games) never needs to
// call CachedResourceLoader::resolve_path() directly.
// ============================================================================

/// \brief Load a font into a UIContext (libRocket desktop) from a resource
///        manifest ID.
///
/// \returns TRUE if the resource was found, resolved, and successfully
///          loaded by libRocket.
bool load_font_from_resource( UIContext& context,
                              CachedResourceLoader& loader,
                              const std::string& resource_id );

/// \brief Load an RML document file into a UIWidget (e.g. UIMessageBox,
///        UIQuestionDialogBox) from a resource manifest ID.
///
/// \returns TRUE if the resource was found, resolved, and successfully
///          loaded into the widget.
bool load_document_from_resource( UIWidget& widget,
                                  CachedResourceLoader& loader,
                                  const std::string& resource_id );

} // namespace nom

#endif // include guard defined
