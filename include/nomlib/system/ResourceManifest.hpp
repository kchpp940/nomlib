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
#include <map>
#include <vector>
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/system/ResourceFile.hpp"

namespace nom {

// Forward declarations
class Value;

/// \brief Plain description of a single resource entry in the manifest.
///
/// \remarks This is a pure data structure — no file I/O, no parsing, no
///          caching. It only describes *what* a resource is.
struct ResourceDescriptor
{
  /// \brief Logical resource name used as the lookup key (e.g. "icon").
  std::string name;

  /// \brief Relative file path to the resource on disk (e.g. "app/icon.png").
  std::string path;

  /// \brief Resource type tag used to select the right IResourceTypeLoader.
  ResourceFile::Type type = ResourceFile::Type::Invalid;

  /// \brief Optional free-form key/value metadata.
  std::map<std::string, std::string> metadata;
};

/// \brief A pure-data catalog of resource descriptors.
///
/// \remarks This class is part of the **data layer** of the resource system.
///          It does **not** deal with:
///            - File I/O or JSON parsing (that belongs in serializers module)
///            - Path resolution (see SearchPath)
///            - Resource loading or caching (see CachedResourceLoader)
///
///          It only holds a `name -> ResourceDescriptor` map and provides
///          lookup + iteration over it. The caller (typically a JSON parser
///          in the serializers module, or CachedResourceLoader via a
///          pre-parsed ptree::Value) is responsible for populating it.
///
///          ### Populating from ptree::Value:
///          \code
///            Value root = ...; // parsed from JSON elsewhere
///            ResourceManifest manifest;
///            manifest.parse_from_value( root["manifest"] );
///          \endcode
class ResourceManifest
{
  public:
    typedef ResourceManifest self_type;
    typedef self_type* raw_ptr;
    typedef std::unique_ptr<self_type> unique_ptr;
    typedef std::shared_ptr<self_type> shared_ptr;

    typedef std::map<std::string, ResourceDescriptor>::const_iterator
      const_iterator;

    /// \brief Default constructor — creates an empty manifest.
    ResourceManifest( void );

    /// \brief Destructor.
    ~ResourceManifest( void );

    /// \brief Parse resource descriptors from an already-parsed ptree::Value
    ///        node and insert them into the manifest.
    ///
    /// \param manifest_node A ptree::Value object node whose keys are
    ///                      logical resource names and whose values are
    ///                      objects with at least a "path" field.
    ///
    /// \returns TRUE if at least one valid descriptor was parsed.
    ///
    /// \remarks This method depends **only** on nomlib-ptree (the generic
    ///          value container), NOT on nomlib-serializers (JSON/XML parsers).
    ///          File I/O and JSON deserialization is the caller's job.
    bool parse_from_value( const Value& manifest_node );

    /// \brief Look up a descriptor by logical resource name.
    ///
    /// \returns A pointer to the descriptor if found, nullptr otherwise.
    const ResourceDescriptor* find( const std::string& name ) const;

    /// \brief Check whether a resource with the given name exists.
    bool exists( const std::string& name ) const;

    /// \brief Insert a descriptor into the manifest.
    ///
    /// \returns TRUE if the entry was inserted; FALSE if a descriptor with
    ///          the same name already existed.
    bool insert( const ResourceDescriptor& descriptor );

    /// \brief Number of resource descriptors in the manifest.
    nom::size_type size( void ) const;

    /// \brief Remove all entries from the manifest.
    void clear( void );

    /// \brief Iteration (const only — use insert() for mutation).
    const_iterator begin( void ) const;
    const_iterator end( void ) const;

  private:
    /// \brief Helper: convert a type string ("Graphic", "Audio", etc.) to a
    ///        ResourceFile::Type enumeration value.
    static ResourceFile::Type type_from_string( const std::string& type_str );

    /// \brief Backing storage: logical name → descriptor.
    std::map<std::string, ResourceDescriptor> entries_;
};

} // namespace nom

#endif // include guard defined

/// \class nom::ResourceManifest
/// \ingroup system
///
/// ### Design rationale
///
/// Previously, ResourceManifest (and SearchPath) were entangled with JSON
/// parsing (the serializers module). This created an undesirable dependency
/// from the system module *upwards* into serializers, when the correct
/// layering is:
///
///   core → ptree → system → serializers → graphics/audio/gui → examples
///
/// By restricting ResourceManifest to pure data + a ptree::Value parser,
/// system now depends only on ptree (a lower layer), and the actual JSON
/// file loading lives in the serializers module where it belongs.
///
