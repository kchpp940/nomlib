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
#ifndef NOMLIB_SYSTEM_CACHED_RESOURCE_LOADER_HPP
#define NOMLIB_SYSTEM_CACHED_RESOURCE_LOADER_HPP

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

#include "nomlib/config.hpp"
#include "nomlib/system/File.hpp"
#include "nomlib/system/Path.hpp"
#include "nomlib/system/ResourceFile.hpp"
#include "nomlib/system/ResourceManifest.hpp"
#include "nomlib/system/SearchPath.hpp"
#include "nomlib/system/IResourceTypeLoader.hpp"

namespace nom {

/// \brief Unified resource loader with caching, lifecycle management, and
///        preloading support.
///
/// This class is the **central coordinator** of the refactored resource
/// system. It composes three specialized collaborators:
///
///   1. **ResourceManifest** — describes *what* resources exist (logical
///      names → relative paths + type tags).
///   2. **SearchPath**       — resolves *where* files live on disk
///      (search prefixes + base directory → absolute paths).
///   3. **IResourceTypeLoader** adapters — know *how* to load each
///      resource type (Texture, SoundBuffer, Font, etc.).
///
/// CachedResourceLoader itself is responsible for:
///   - Caching loaded resources and managing their lifetimes
///   - Looking up resource names in the manifest
///   - Resolving relative paths via SearchPath
///   - Validating file existence before passing to type loaders
///   - Matching resource types to the correct IResourceTypeLoader
///   - Preloading batches of resources
///   - Providing explicit release / eviction policies
///
/// It is **NOT** responsible for:
///   - Parsing manifest JSON → ResourceManifest does that
///   - Walking search prefixes → SearchPath does that
///   - Type-specific loading logic → IResourceTypeLoader subclasses do that
///
/// ### Usage example:
/// \code
///   // Setup
///   CachedResourceLoader loader;
///   loader.load_config("resources.json");            // SearchPath
///   loader.load_manifest("resources.json");          // ResourceManifest
///   loader.register_type_loader(
///     std::make_unique<TextureLoader>() );
///
///   // Load by logical name (path resolution + caching is automatic)
///   Texture* tex = loader.load<Texture>("player_sprite");
///
///   // Preload a batch
///   loader.preload( {"bg_music", "title_font", "player_sprite"} );
///
///   // Release specific or all resources
///   loader.release("player_sprite");
///   loader.clear();
/// \endcode
class CachedResourceLoader
{
  public:
    typedef CachedResourceLoader self_type;
    typedef self_type* raw_ptr;
    typedef std::unique_ptr<self_type> unique_ptr;
    typedef std::shared_ptr<self_type> shared_ptr;

    /// \brief Release strategy hints for cache eviction.
    enum class ReleasePolicy
    {
      /// \brief Release only if the resource is not currently referenced by
      ///        any external shared_ptr.
      UnusedOnly,

      /// \brief Release regardless of external references (use with caution).
      Force
    };

    /// \brief Default constructor.
    CachedResourceLoader( void );

    /// \brief Destructor — releases all cached resources.
    virtual ~CachedResourceLoader( void );

    // ------------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------------

    /// \brief Load the search-path configuration from a JSON file.
    ///
    /// Delegates to SearchPath::load_file.
    bool load_search_paths( const std::string& config_file,
                            const std::string& node = "resources" );

    /// \brief Load the resource manifest from a JSON file.
    ///
    /// Delegates to ResourceManifest::load_file.
    bool load_manifest( const std::string& manifest_file,
                        const std::string& node = "manifest" );

    /// \brief Access the underlying SearchPath for manual configuration.
    SearchPath& search_path( void );
    const SearchPath& search_path( void ) const;

    /// \brief Access the underlying ResourceManifest for manual entry.
    ResourceManifest& manifest( void );
    const ResourceManifest& manifest( void ) const;

    /// \brief Register a type loader for a specific resource type.
    ///
    /// \remarks If a loader for the same type already exists, it is
    ///          replaced.
    void register_type_loader( std::unique_ptr<IResourceTypeLoader> loader );

    /// \brief Register a type loader for a specific resource type (by enum).
    ///
    /// This overload lets you specify an explicit ResourceFile::Type, which
    /// is useful for loaders that handle multiple types (e.g. FontLoader
    /// handles both TrueTypeFont and BitmapFont).
    void register_type_loader( ResourceFile::Type type,
                               std::unique_ptr<IResourceTypeLoader> loader );

    // ------------------------------------------------------------------
    // Resource loading / retrieval
    // ------------------------------------------------------------------

    /// \brief Load a resource by its logical name.
    ///
    /// This is the **primary API** for end users. Under the hood it:
    ///   1. Looks up `name` in the ResourceManifest
    ///   2. Resolves the manifest's relative path via SearchPath
    ///   3. Checks if the resolved file exists
    ///   4. Returns a cached copy if already loaded
    ///   5. Otherwise: finds the matching IResourceTypeLoader, calls
    ///      load(), caches the result, returns it
    ///
    /// \tparam T  The concrete resource type (Texture, Font, Image, etc.)
    ///
    /// \returns A non-null pointer to the resource on success, or nullptr
    ///          on failure (with a diagnostic log message emitted).
    template <typename T>
    T* load( const std::string& name );

    /// \brief Check whether a resource with the given logical name exists
    ///        in the manifest.
    bool exists( const std::string& name ) const;

    /// \brief Check whether a resource is currently loaded in the cache.
    bool is_loaded( const std::string& name ) const;

    // ------------------------------------------------------------------
    // Batch operations (preloading / releasing)
    // ------------------------------------------------------------------

    /// \brief Preload a list of resources by logical name.
    ///
    /// \returns The number of resources successfully loaded.
    nom::size_type preload( const std::vector<std::string>& names );

    /// \brief Preload all resources listed in the manifest.
    ///
    /// \returns The number of resources successfully loaded.
    nom::size_type preload_all( void );

    /// \brief Release a specific resource from the cache.
    ///
    /// \param policy If UnusedOnly (default), the resource is only
    ///               released when there are no external shared_ptr
    ///               references to it.
    ///
    /// \returns TRUE if the resource was actually released.
    bool release( const std::string& name,
                  ReleasePolicy policy = ReleasePolicy::UnusedOnly );

    /// \brief Release all resources from the cache.
    ///
    /// \see release for the policy parameter semantics.
    ///
    /// \returns The number of resources that were actually released.
    nom::size_type clear( ReleasePolicy policy = ReleasePolicy::UnusedOnly );

    /// \brief Get the number of resources currently cached.
    nom::size_type size( void ) const;

    // ------------------------------------------------------------------
    // Debugging aids
    // ------------------------------------------------------------------

    /// \brief Print the contents of the cache to the log.
    void dump( void ) const;

  private:
    /// \brief Internal cache entry — holds the loaded resource pointer,
    ///        its type loader (for unload), and metadata.
    struct CacheEntry
    {
      CacheEntry() :
        resource( nullptr ),
        loader( nullptr ),
        loaded( false )
      {}

      void* resource;
      IResourceTypeLoader* loader;  ///< Non-owning; kept for ::unload()
      bool loaded;
      ResourceDescriptor descriptor;
    };

    /// \brief Non-templated implementation of load() so logic isn't
    ///        duplicated in every template instantiation.
    void* load_internal( const std::string& name,
                         ResourceFile::Type expected_type,
                         bool& out_loaded_from_cache );

    /// \brief Resolve the full absolute path for a resource descriptor.
    std::string resolve_path( const ResourceDescriptor& desc ) const;

    /// \brief Find a suitable type loader for the given resource type.
    IResourceTypeLoader* find_loader( ResourceFile::Type type ) const;

    /// \brief Search path resolver (owned).
    SearchPath search_path_;

    /// \brief Resource manifest / catalog (owned).
    ResourceManifest manifest_;

    /// \brief Registered type loaders, keyed by ResourceFile::Type.
    /// Multiple types can share the same loader pointer.
    std::map<ResourceFile::Type, std::shared_ptr<IResourceTypeLoader>> loaders_;

    /// \brief Loaded resource cache, keyed by logical resource name.
    std::map<std::string, CacheEntry> cache_;

    /// \brief Mutex for thread-safe cache access.
    mutable std::mutex mutex_;
};

// ============================================================================
// Template implementation
// ============================================================================

template <typename T>
T* CachedResourceLoader::load( const std::string& name )
{
  // We don't try to deduce the type from T (that would require RTTI or
  // additional registration). Instead, we rely on the manifest to tell us
  // the type, and the type loader to produce a compatible T*. The caller is
  // responsible for requesting a T that matches the manifest entry's type.
  bool loaded_from_cache = false;
  void* raw = this->load_internal( name, ResourceFile::Type::Invalid,
                                    loaded_from_cache );
  return static_cast<T*>( raw );
}

} // namespace nom

#endif // include guard defined

/// \class nom::CachedResourceLoader
/// \ingroup system
///
/// ### Design rationale
///
/// Previously, resource-related concerns were scattered across the codebase:
///
/// | Concern               | Old location(s)                       |
/// |-----------------------|---------------------------------------|
/// | Manifest parsing      | (missing, partially in SearchPath)    |
/// | Path resolution       | SearchPath (serializers module)       |
/// | Caching               | ResourceCache<Font> etc.              |
/// | Type-specific loading | Texture::load, audio::create_buffer…  |
/// | Preload / release     | (missing)                             |
///
/// This scattered design caused duplicated path-concatenation code
/// (`res.path() + filename`) across every example and test, and made it
/// impossible to implement cross-cutting concerns like preloading or LRU
/// eviction.
///
/// CachedResourceLoader centralizes the orchestration while delegating
/// each specialized concern to a dedicated collaborator, following the
/// *Single Responsibility Principle*.
///
