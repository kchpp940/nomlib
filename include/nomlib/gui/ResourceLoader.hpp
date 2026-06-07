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
#ifndef NOMLIB_GUI_RESOURCE_LOADER_HPP
#define NOMLIB_GUI_RESOURCE_LOADER_HPP

#include <string>

#include "nomlib/config.hpp"
#include "nomlib/serializers/ResourceManifest.hpp"
#include "nomlib/gui/UIContext.hpp"

namespace nom {

/// \brief Load a font into a UIContext from a ResourceManifest entry.
///
/// \param manifest  The manifest to look up the id from.
/// \param id        The manifest resource id (type must be Font).
/// \param context   UIContext to register the font with.
///
/// \returns true on success, false on failure (id missing, type mismatch,
///          or UIContext::load_font failure).
inline bool
load_ui_font( const ResourceManifest& manifest,
              const std::string& id,
              UIContext& context )
{
  const ResourceManifestEntry& entry = manifest.find( id );
  if( ! entry.valid() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_ui_font: unknown manifest id:", id );
    return false;
  }
  if( entry.type() != ResourceType::Font )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_ui_font: id '", id, "' has type '",
                 ResourceManifest::type_to_string( entry.type() ),
                 "', expected 'font'." );
    return false;
  }

  std::string path = manifest.resolve_path( id );
  if( context.load_font( path ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_ui_font: failed to load '", path, "'" );
    return false;
  }
  return true;
}

/// \brief Load a UI (RML) document into a UIContext from a ResourceManifest entry.
///
/// \param manifest  The manifest to look up the id from.
/// \param id        The manifest resource id (type must be UI).
/// \param context   UIContext to load the document into.
///
/// \remarks Uses entry.path() directly rather than resolve_path(), because
///          UI document filenames are resolved by the UI context's own
///          RocketFileInterface using its configured base directory.
///
/// \returns Pointer to the loaded ElementDocument, or nullptr on failure.
inline void*
load_ui_document( const ResourceManifest& manifest,
                  const std::string& id,
                  UIContext& context )
{
  const ResourceManifestEntry& entry = manifest.find( id );
  if( ! entry.valid() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_ui_document: unknown manifest id:", id );
    return nullptr;
  }
  if( entry.type() != ResourceType::UI )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_ui_document: id '", id, "' has type '",
                 ResourceManifest::type_to_string( entry.type() ),
                 "', expected 'ui'." );
    return nullptr;
  }

  std::string filename = entry.path();
  auto doc = context.load_document_file( filename );
  if( doc == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_ui_document: failed to load '", filename, "'" );
  }
  return doc;
}

} // namespace nom

#endif // include guard defined
