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
#include "nomlib/serializers/ResourceLoaderHelpers.hpp"

#include "nomlib/ptree/Value.hpp"
#include "nomlib/serializers/JsonCppDeserializer.hpp"
#include "nomlib/system/CachedResourceLoader.hpp"
#include "nomlib/system/ResourceManifest.hpp"
#include "nomlib/system/SearchPath.hpp"

namespace nom {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

std::unique_ptr<IValueDeserializer> default_deserializer(
  IValueDeserializer* user_deserializer )
{
  if( user_deserializer != nullptr )
  {
    // Caller provided their own; we return a no-op unique_ptr that won't
    // delete the caller-owned pointer.
    return std::unique_ptr<IValueDeserializer>( user_deserializer,
                                                []( IValueDeserializer* ) {} );
  }
  return std::unique_ptr<IValueDeserializer>( new JsonCppDeserializer() );
}

bool load_value_tree( const std::string& filename,
                      IValueDeserializer* deserializer,
                      Value& out_root )
{
  auto deser = default_deserializer( deserializer );
  if( deser == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SERIALIZERS,
                 "ResourceLoaderHelpers: null deserializer" );
    return false;
  }

  if( deser->load( filename, out_root ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SERIALIZERS,
                 "ResourceLoaderHelpers: failed to parse JSON file:",
                 filename );
    return false;
  }

  return true;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool load_resource_config( CachedResourceLoader& loader,
                           const std::string& filename,
                           const std::string& resources_key,
                           const std::string& manifest_key,
                           IValueDeserializer* deserializer )
{
  Value root;
  if( ! load_value_tree( filename, deserializer, root ) )
  {
    return false;
  }

  bool ok_sp = true;
  bool ok_manifest = true;

  if( root[resources_key].null_type() == false )
  {
    ok_sp = loader.parse_search_paths_from_value( root[resources_key] );
  }
  else
  {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_SERIALIZERS,
                  "ResourceLoaderHelpers: no '", resources_key,
                  "' node in:", filename );
    ok_sp = false;
  }

  if( root[manifest_key].null_type() == false )
  {
    ok_manifest = loader.parse_manifest_from_value( root[manifest_key] );
  }
  else
  {
    // Manifest is optional; the caller may populate it programmatically.
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SERIALIZERS,
                  "ResourceLoaderHelpers: no '", manifest_key,
                  "' node in:", filename,
                  "(manifest entries must be added manually)" );
  }

  return ( ok_sp || ok_manifest );
}

bool load_search_path_file( SearchPath& sp,
                            const std::string& filename,
                            const std::string& resources_key,
                            IValueDeserializer* deserializer )
{
  Value root;
  if( ! load_value_tree( filename, deserializer, root ) )
  {
    return false;
  }

  if( root[resources_key].null_type() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SERIALIZERS,
                 "ResourceLoaderHelpers: no '", resources_key,
                 "' node in:", filename );
    return false;
  }

  return sp.parse_from_value( root[resources_key] );
}

bool load_manifest_file( ResourceManifest& manifest,
                         const std::string& filename,
                         const std::string& manifest_key,
                         IValueDeserializer* deserializer )
{
  Value root;
  if( ! load_value_tree( filename, deserializer, root ) )
  {
    return false;
  }

  if( root[manifest_key].null_type() )
  {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_SERIALIZERS,
                  "ResourceLoaderHelpers: no '", manifest_key,
                  "' node in:", filename );
    return false;
  }

  return manifest.parse_from_value( root[manifest_key] );
}

} // namespace nom
