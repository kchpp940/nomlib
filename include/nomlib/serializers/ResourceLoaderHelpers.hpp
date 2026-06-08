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
#ifndef NOMLIB_SERIALIZERS_RESOURCE_LOADER_HELPERS_HPP
#define NOMLIB_SERIALIZERS_RESOURCE_LOADER_HELPERS_HPP

#include <string>

#include "nomlib/config.hpp"

// Forward declarations
namespace nom {
  class CachedResourceLoader;
  class SearchPath;
  class ResourceManifest;
  class IValueDeserializer;
}

namespace nom {

/// \brief Load a JSON configuration file and parse the search-path and
///        manifest sections into a CachedResourceLoader.
///
/// This is the **recommended entry point** for setting up a
/// CachedResourceLoader from a JSON config file. It handles all JSON parsing
/// so that the system module (where CachedResourceLoader lives) doesn't need
/// to depend on serializers.
///
/// ### Expected JSON structure:
/// \code
///   {
///     "resources": {
///       "search_prefix": ["./", "../../", "../../../"],
///       "path": "Resources/examples/app/"
///     },
///     "manifest": {
///       "icon": { "path": "icon.png", "type": "Graphic" },
///       "board": { "path": "boardoutline.png", "type": "Graphic" }
///     }
///   }
/// \endcode
///
/// \param loader     The CachedResourceLoader instance to populate.
/// \param filename   Path to the JSON configuration file.
/// \param resources_key Name of the JSON node containing search-path config
///                     (default: "resources").
/// \param manifest_key  Name of the JSON node containing the manifest
///                      (default: "manifest").
/// \param deserializer Optional custom deserializer (defaults to
///                     JsonCppDeserializer).
///
/// \returns TRUE if both the search path and manifest were loaded
///          successfully.
bool load_resource_config( CachedResourceLoader& loader,
                           const std::string& filename,
                           const std::string& resources_key = "resources",
                           const std::string& manifest_key = "manifest",
                           IValueDeserializer* deserializer = nullptr );

/// \brief Load search-path configuration from a JSON file into a SearchPath.
///
/// \param sp           The SearchPath instance to populate.
/// \param filename     Path to the JSON configuration file.
/// \param resources_key Name of the JSON node (default: "resources").
/// \param deserializer Optional custom deserializer.
///
/// \returns TRUE on success.
bool load_search_path_file( SearchPath& sp,
                            const std::string& filename,
                            const std::string& resources_key = "resources",
                            IValueDeserializer* deserializer = nullptr );

/// \brief Load a resource manifest from a JSON file into a ResourceManifest.
///
/// \param manifest     The ResourceManifest instance to populate.
/// \param filename     Path to the JSON configuration file.
/// \param manifest_key Name of the JSON node (default: "manifest").
/// \param deserializer Optional custom deserializer.
///
/// \returns TRUE on success.
bool load_manifest_file( ResourceManifest& manifest,
                         const std::string& filename,
                         const std::string& manifest_key = "manifest",
                         IValueDeserializer* deserializer = nullptr );

} // namespace nom

#endif // include guard defined

/// \class nom::ResourceLoaderHelpers (free functions)
/// \ingroup serializers
///
/// These free functions live in the serializers module because JSON parsing
/// is a serialization-layer concern. By keeping the parsing here, the
/// lower-level system module (ResourceManifest, SearchPath,
/// CachedResourceLoader) stays free of any upward dependency on serializers.
///
/// The dependency graph remains correct:
///
///   core → ptree → system → serializers → graphics/audio/gui → examples
///
