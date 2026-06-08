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
#ifndef NOMLIB_SYSTEM_SEARCH_PATH_HPP
#define NOMLIB_SYSTEM_SEARCH_PATH_HPP

#include <memory>
#include <string>
#include <vector>

#include "nomlib/config.hpp"

namespace nom {

// Forward declarations
class IValueDeserializer;

/// \brief Resolve a directory location by scanning through search path
///        prefixes.
///
/// \remarks This class **only** handles path resolution: it parses search
/// prefixes and a relative base path from a config file, scans for the first
/// existing directory, and provides a method to resolve a relative resource
/// path to an absolute file path.
///
/// It does NOT handle:
/// - Resource manifest parsing (see ResourceManifest)
/// - Resource caching / lifecycle management (see CachedResourceLoader)
/// - Resource loading / type-specific logic (see IResourceTypeLoader)
///
/// JSON config format (shared with the manifest file):
/// \code
/// {
///   "resources": {
///     "search_prefix": ["./", "../../", "../../../"],
///     "path": "Resources/examples/app/"
///   }
/// }
/// \endcode
class SearchPath
{
  public:
    typedef SearchPath self_type;
    typedef self_type* raw_ptr;
    typedef std::unique_ptr<self_type> unique_ptr;
    typedef std::shared_ptr<self_type> shared_ptr;

    /// \brief Default constructor.
    SearchPath();

    /// \brief Destructor.
    ~SearchPath();

    /// \brief Get the resolved base path.
    ///
    /// \returns The absolute (or resolved relative) base path to the
    ///          resources directory, or an empty string if not yet loaded.
    const std::string& path() const;

    /// \brief Resolve a relative resource path to a full file path.
    ///
    /// \param relative_path The path relative to the resolved base path.
    ///
    /// \returns The concatenated full path. Does NOT check for file
    ///          existence — use File::exists if you need that.
    std::string resolve( const std::string& relative_path ) const;

    /// \brief Set a custom deserializer.
    ///
    /// \remarks The default parser is JSON.
    void set_deserializer( std::unique_ptr<IValueDeserializer> fp );

    /// \brief Parse a config file and resolve the base path.
    ///
    /// \param filename The absolute file path to a JSON config file.
    /// \param node     The top-level object key that contains
    ///                 "search_prefix" and "path" fields.
    ///                 Defaults to "resources".
    ///
    /// \returns TRUE if a valid existing directory was resolved.
    bool load_file( const std::string& filename,
                    const std::string& node = "resources" );

    /// \brief Manually set the search prefixes and base path, then resolve.
    ///
    /// \param search_prefixes List of directory prefixes to scan.
    /// \param base_path       The relative path suffix appended to each
    ///                        prefix.
    ///
    /// \returns TRUE if a valid existing directory was resolved.
    bool resolve_from( const std::vector<std::string>& search_prefixes,
                       const std::string& base_path );

    /// \brief Get the list of search prefixes that were scanned.
    const std::vector<std::string>& search_prefixes() const;

    /// \brief Check whether a valid path has been resolved.
    bool is_resolved() const;

  private:
    /// \brief Parser object to use.
    std::unique_ptr<IValueDeserializer> fp_;

    /// \brief List of path prefixes that were scanned.
    std::vector<std::string> search_prefix_;

    /// \brief The resolved base path (first existing directory found).
    std::string path_;
};

} // namespace nom

#endif // include guard defined
