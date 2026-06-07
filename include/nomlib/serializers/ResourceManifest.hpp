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
#ifndef NOMLIB_SERIALIZERS_RESOURCE_MANIFEST_HPP
#define NOMLIB_SERIALIZERS_RESOURCE_MANIFEST_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "nomlib/config.hpp"

namespace nom {

// Forward declarations
class IValueDeserializer;

/// \brief Resource type enumeration for manifest entries
enum class ResourceType
{
  Invalid = 0,
  Texture,
  Image,
  Audio,
  Font,
  UI,
  SpriteSheet,
  Config
};

/// \brief Preload strategy for manifest resources
enum class PreloadStrategy
{
  None = 0,
  Eager,
  Lazy
};

/// \brief Descriptor of a resource entry within the manifest
class ResourceManifestEntry
{
  public:
    typedef ResourceManifestEntry self_type;

    ResourceManifestEntry( void );
    ~ResourceManifestEntry( void );

    ResourceManifestEntry(
      const std::string& id,
      ResourceType type,
      const std::string& path,
      const std::vector<std::string>& tags = {},
      PreloadStrategy preload = PreloadStrategy::Lazy
    );

    bool valid( void ) const;

    const std::string& id( void ) const;
    ResourceType type( void ) const;
    const std::string& path( void ) const;
    const std::vector<std::string>& tags( void ) const;
    PreloadStrategy preload( void ) const;

    void set_id( const std::string& id );
    void set_type( ResourceType type );
    void set_path( const std::string& path );
    void set_tags( const std::vector<std::string>& tags );
    void set_preload( PreloadStrategy strategy );

    bool has_tag( const std::string& tag ) const;

  private:
    std::string id_;
    ResourceType type_;
    std::string path_;
    std::vector<std::string> tags_;
    PreloadStrategy preload_;
};

/// \brief Manifest-based resource lookup and path resolution.
///
/// \remarks Loads a JSON manifest describing all game resources and provides
///          id-based lookup, tag/type queries, and base-path-relative file
///          resolution.  Type-specific loading (textures, audio, fonts, UI)
///          is intentionally left to the caller -- combine ::resolve_path
///          with each type's own ::load method, e.g.:
///          \code
///            ResourceManifest m;
///            m.load_file("resources.json");
///            Texture tex;
///            tex.load(m.resolve_path("player"));
///          \endcode
class ResourceManifest
{
  public:
    typedef ResourceManifest self_type;

    typedef self_type* raw_ptr;
    typedef std::unique_ptr<self_type> unique_ptr;
    typedef std::shared_ptr<self_type> shared_ptr;

    ResourceManifest( void );
    ~ResourceManifest( void );

    ResourceManifest( const self_type& other );
    self_type& operator =( const self_type& other );

    ResourceManifest( self_type&& other ) noexcept;
    self_type& operator =( self_type&& other ) noexcept;

    /// \brief Load a manifest from a JSON file
    ///
    /// \param manifest_path  Absolute or relative path to the manifest JSON
    /// \param manifest_node  Top-level JSON key for the manifest array;
    ///                       defaults to "resources"
    bool load_file(
      const std::string& manifest_path,
      const std::string& manifest_node = "resources"
    );

    /// \brief Set a custom JSON deserializer
    void set_deserializer( std::unique_ptr<IValueDeserializer> fp );

    /// \brief Get the resolved base resource path
    const std::string& base_path( void ) const;

    /// \brief Query whether a resource id exists in the manifest
    bool exists( const std::string& id ) const;

    /// \brief Get a manifest entry by id
    ///
    /// \returns ResourceManifestEntry, or a default-constructed invalid entry
    ///          if the id is not found.
    const ResourceManifestEntry& find( const std::string& id ) const;

    /// \brief Get all entries matching a given resource type
    std::vector<ResourceManifestEntry> find_by_type( ResourceType type ) const;

    /// \brief Get all entries tagged with a given label
    std::vector<ResourceManifestEntry> find_by_tag( const std::string& tag ) const;

    /// \brief Resolve a manifest entry to its absolute file path
    std::string resolve_path( const std::string& id ) const;

    /// \brief Total number of registered resource entries
    nom::size_type size( void ) const;

    /// \brief Remove all entries from the manifest
    void clear( void );

    /// \brief Dump all registered entries (debug aid)
    void dump( void ) const;

    /// \brief Convert a ResourceType to its string representation
    static std::string type_to_string( ResourceType type );

    /// \brief Parse a ResourceType from a string
    static ResourceType string_to_type( const std::string& str );

    /// \brief Parse a PreloadStrategy from a string
    static PreloadStrategy string_to_preload( const std::string& str );

  private:
    /// \brief Resolve base path using search prefixes, mimicking SearchPath
    bool resolve_base_path(
      const std::vector<std::string>& search_prefix,
      const std::string& path
    );

    /// \brief Internal entry storage keyed by resource id
    std::map<std::string, ResourceManifestEntry> entries_;

    /// \brief The resolved absolute base path for resources
    std::string base_path_;

    /// \brief JSON deserializer
    std::unique_ptr<IValueDeserializer> fp_;

    /// \brief Sentinel invalid entry used by ::find
    static const ResourceManifestEntry null_entry;
};

} // namespace nom

#endif // include guard defined
