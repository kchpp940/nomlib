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
#include "nomlib/gui/ResourceLoaders.hpp"

#include <fstream>
#include <sstream>

#include "nomlib/system/File.hpp"
#include "nomlib/system/CachedResourceLoader.hpp"
#include "nomlib/gui/UIContext.hpp"
#include "nomlib/gui/UIWidget.hpp"

namespace nom {

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

GuiDocumentLoader::GuiDocumentLoader()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_GUI,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

GuiDocumentLoader::~GuiDocumentLoader()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_GUI,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

// ---------------------------------------------------------------------------
// IResourceTypeLoader interface
// ---------------------------------------------------------------------------

ResourceFile::Type GuiDocumentLoader::type() const
{
  return ResourceFile::Type::GuiDocument;
}

void* GuiDocumentLoader::load( const std::string& absolute_path )
{
  File fp;
  if( ! fp.exists( absolute_path ) )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GUI,
                 "GuiDocumentLoader: file does not exist:",
                 absolute_path );
    return nullptr;
  }

  std::ifstream in( absolute_path, std::ios::in | std::ios::binary );
  if( ! in.is_open() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GUI,
                 "GuiDocumentLoader: could not open file:",
                 absolute_path );
    return nullptr;
  }

  std::ostringstream ss;
  ss << in.rdbuf();

  return new std::string( ss.str() );
}

void GuiDocumentLoader::unload( void* resource )
{
  if( resource == nullptr )
  {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_GUI,
                  "GuiDocumentLoader: refusing to unload null resource" );
    return;
  }

  std::string* str = static_cast<std::string*>( resource );
  delete str;
}

// ===========================================================================
// Convenience free functions — adapt libRocket path-only APIs.
//
// IMPORTANT: These helpers never bypass manifest type validation. They call
// CachedResourceLoader::resolve_path(expected_type, id), which first verifies
// the manifest entry's type tag matches the adapter's expectations, then
// returns the absolute file path. No "FilePath" pseudo-types, no caching of
// raw std::string objects as if they were resources — the manifest's type
// tag is the single source of truth.
// ===========================================================================

bool load_font_from_resource( UIContext& context,
                              CachedResourceLoader& loader,
                              const std::string& resource_id )
{
  const std::string path = loader.resolve_path( ResourceFile::TrueTypeFont,
                                                resource_id );
  if( path.empty() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GUI,
                 "load_font_from_resource: failed to resolve manifest ID:",
                 resource_id );
    return false;
  }
  return context.load_font( path );
}

bool load_document_from_resource( UIWidget& widget,
                                  CachedResourceLoader& loader,
                                  const std::string& resource_id )
{
  const std::string path = loader.resolve_path( ResourceFile::GuiDocument,
                                                resource_id );
  if( path.empty() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_GUI,
                 "load_document_from_resource: failed to resolve manifest ID:",
                 resource_id );
    return false;
  }
  return widget.load_document_file( path );
}

} // namespace nom
