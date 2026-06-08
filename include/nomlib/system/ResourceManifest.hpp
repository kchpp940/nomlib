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
#ifndef NOMLIB_SYSTEM_RESOURCE_MANIFEST_HPP
#define NOMLIB_SYSTEM_RESOURCE_MANIFEST_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/system/ResourceFile.hpp"

namespace nom {

// Forward declarations
class IValueDeserializer;

/// \brief Resource descriptor entry within a manifest
struct ResourceDescriptor
{
  ResourceDescriptor() :
    type( ResourceFile::Type::Invalid )
  {}

  ResourceDescriptor( const std::string& res_name,
                      const std::string& res_path,
                      ResourceFile::Type res_type ) :
    name( res_name ),
    path( res_path ),
    type( res_type )
  {}

  /// \brief Logical resource name (key used for lookup)
  std::string name;

  /// \brief Relative file path (relative to the resolved resources directory)
  std::string path;

  /// \brief Resource type classification
  ResourceFile::Type type;

  /// \brief Optional metadata
  std::map<std::string, std::string> metadata;
};

/// \brief Manifest describing all available resources.
///
/// \remarks This class **only** handles description and parsing of the
/// resource manifest. It does not perform path resolution, caching, loading,
/// or lifecycle management -- those concerns belong to CachedResourceLoader
/// and SearchPath.
///
/// The manifest JSON format:
/// \code
/// {
///   "resources": {
///     "search_prefix": ["./", "../../"],
///     "path": "Resources/assets/"
///   },
///   "manifest": {
///     "icon": {
///       "path": "icon.png",
///       "type": "Graphic"
///     },
///     "title_music": {
///       "path": "audio/title.ogg",
///       "type": "Audio"
///     },
///     "menu_font": {
///       "path": "fonts/menu.ttf",
///       "type": "TrueTypeFont"
///     }
///   }
/// }
/// \endcode
class ResourceManifest
{
  public:
    typedef ResourceManifest self_type;
    typedef self_type* raw_ptr;
    typedef std::unique_ptr<self_type> unique_ptr;
    typedef std::shared_ptr<self_type> shared_ptr;

    typedef std::map<std::string, ResourceDescriptor>::const_iterator const_iterator;
    typedef std::map<std::string, ResourceDescriptor>::iterator iterator;

    /// \brief Default constructor.
    ResourceManifest( void );

    /// \brief Destructor.
    virtual ~ResourceManifest( void );

    /// \brief Load and parse a manifest file.
    ///
    /// \param manifest_file The absolute file path to a JSON manifest file.
    /// \param manifest_node  The top-level object key for the manifest
    ///                       section; defaults to "manifest".
    ///
    /// \returns TRUE if the file was successfully parsed and the manifest
    ///          contains at least one valid resource descriptor entry.
    ///
    /// \remarks The default parser is JSON. Use ::set_deserializer to change
    ///          the parser.
    bool load_file( const std::string& manifest_file,
                    const std::string& manifest_node = "manifest" );

    /// \brief Set a custom deserializer.
    ///
    /// \remarks The default parser is JSON.
    void set_deserializer( std::unique_ptr<IValueDeserializer> fp );

    /// \brief Look up a resource descriptor by its logical name.
    ///
    /// \returns A const pointer to the ResourceDescriptor on success, or
    ///          nullptr if no such resource is registered.
    const ResourceDescriptor* find( const std::string& name ) const;

    /// \brief Check whether a resource with the given logical name exists.
    bool exists( const std::string& name ) const;

    /// \brief Manually register a resource descriptor.
    ///
    /// \note  This is useful for unit tests or for building a manifest
    ///        programmatically without a file.
    bool insert( const ResourceDescriptor& descriptor );

    /// \brief Get the total number of resource descriptors.
    nom::size_type size( void ) const;

    /// \brief Remove all resource descriptors.
    void clear( void );

    /// \brief Iterator access (begin).
    const_iterator begin( void ) const;

    /// \brief Iterator access (end).
    const_iterator end( void ) const;

  private:
    /// \brief Convert a resource type string (e.g. "Graphic", "Audio") to the
    ///        corresponding ResourceFile::Type enumeration value.
    static ResourceFile::Type type_from_string( const std::string& type_str );

    /// \brief Parser object to use for deserializing the manifest file.
    std::unique_ptr<IValueDeserializer> fp_;

    /// \brief Resource descriptor entries, keyed by logical resource name.
    std::map<std::string, ResourceDescriptor> entries_;
};

} // namespace nom

#endif // include guard defined
