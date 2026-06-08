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
#ifndef NOMLIB_AUDIO_CACHED_RESOURCE_LOADER_HPP
#define NOMLIB_AUDIO_CACHED_RESOURCE_LOADER_HPP

#include <string>
#include <map>
#include <vector>

#include "nomlib/config.hpp"
#include "nomlib/serializers/ResourceManifest.hpp"
#include "nomlib/audio/IAudioDevice.hpp"
#include "nomlib/audio/IOAudioEngine.hpp"
#include "nomlib/audio/ResourceLoader.hpp"

#if defined(NOM_USE_OPENAL)
  #include "nomlib/audio/AL/SoundSource.hpp"
#endif

namespace nom {
namespace audio {

/// \brief Manifest-aware audio buffer loader with per-id caching, eager
///        preload, lazy load-on-first-access, and tag-based bulk load/release.
///
/// \remarks This class is a non-owning view of an IOAudioEngine; the engine
///          must outlive the loader.  All cached SoundBuffers are freed via
///          IOAudioEngine::free_buffer on release/destruction.
///
///          When built without NOM_USE_OPENAL, all load operations return
///          nullptr with an error log.
class CachedResourceLoader
{
  public:
    typedef CachedResourceLoader self_type;

    CachedResourceLoader( void );
    CachedResourceLoader( const ResourceManifest& manifest,
                          IOAudioEngine* engine );

    ~CachedResourceLoader( void );

    CachedResourceLoader( const self_type& ) = delete;
    self_type& operator =( const self_type& ) = delete;

    CachedResourceLoader( self_type&& other ) noexcept;
    self_type& operator =( self_type&& other ) noexcept;

    /// \brief Attach a resource manifest (clears all caches).
    void set_manifest( const ResourceManifest& manifest );

    /// \brief Attach an audio engine (clears all caches, since buffers are
    ///        engine-owned).
    void set_engine( IOAudioEngine* engine );

    const ResourceManifest& manifest( void ) const;
    IOAudioEngine* engine( void ) const;

    // -----------------------------------------------------------------------
    // Lazy cached access
    // -----------------------------------------------------------------------

    /// \brief Get a cached SoundBuffer, loading it on first access.
    ///
    /// \returns Non-owning pointer to the cached SoundBuffer, or nullptr on
    ///          failure.  The pointer remains valid until release()/clear()
    ///          or loader destruction.
    SoundBuffer* get_audio( const std::string& id );

    // -----------------------------------------------------------------------
    // Preload
    // -----------------------------------------------------------------------

    /// \brief Preload all Audio entries with PreloadStrategy::Eager.
    bool preload_eager( void );

    /// \brief Preload all Audio entries carrying the given tag.
    ///
    /// \returns Count of successfully (pre)loaded buffers.
    nom::size_type preload_by_tag( const std::string& tag );

    // -----------------------------------------------------------------------
    // Release
    // -----------------------------------------------------------------------

    /// \brief Release a single cached buffer by id, freeing it from the engine.
    bool release( const std::string& id );

    /// \brief Release all cached buffers whose manifest entry carries tag.
    nom::size_type release_by_tag( const std::string& tag );

    /// \brief Release ALL cached audio buffers.
    void clear( void );

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    bool is_loaded( const std::string& id ) const;
    nom::size_type loaded_count( void ) const;
    void dump( void ) const;

  private:
    ResourceManifest manifest_;
    IOAudioEngine* engine_;
    std::map<std::string, SoundBuffer*> cache_audio_;
};

} // namespace audio
} // namespace nom

#endif // include guard defined

// Inline implementation ------------------------------------------------------

namespace nom {
namespace audio {

inline CachedResourceLoader::CachedResourceLoader( void ) :
  engine_( nullptr )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

inline CachedResourceLoader::CachedResourceLoader( const ResourceManifest& manifest,
                                                   IOAudioEngine* engine ) :
  manifest_( manifest ),
  engine_( engine )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

inline CachedResourceLoader::~CachedResourceLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
  this->clear();
}

inline CachedResourceLoader::CachedResourceLoader( self_type&& other ) noexcept :
  manifest_( std::move( other.manifest_ ) ),
  engine_( other.engine_ ),
  cache_audio_( std::move( other.cache_audio_ ) )
{
  other.engine_ = nullptr;
  other.cache_audio_.clear();
}

inline CachedResourceLoader::self_type&
CachedResourceLoader::operator =( self_type&& other ) noexcept
{
  if( this != &other )
  {
    this->clear();
    this->manifest_ = std::move( other.manifest_ );
    this->engine_ = other.engine_;
    this->cache_audio_ = std::move( other.cache_audio_ );
    other.engine_ = nullptr;
    other.cache_audio_.clear();
  }
  return *this;
}

inline void
CachedResourceLoader::set_manifest( const ResourceManifest& manifest )
{
  this->clear();
  this->manifest_ = manifest;
}

inline void
CachedResourceLoader::set_engine( IOAudioEngine* engine )
{
  this->clear();
  this->engine_ = engine;
}

inline const ResourceManifest&
CachedResourceLoader::manifest( void ) const
{
  return this->manifest_;
}

inline IOAudioEngine*
CachedResourceLoader::engine( void ) const
{
  return this->engine_;
}

inline SoundBuffer*
CachedResourceLoader::get_audio( const std::string& id )
{
  auto itr = this->cache_audio_.find( id );
  if( itr != this->cache_audio_.end() )
  {
    return itr->second;
  }

  if( this->engine_ == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "CachedResourceLoader::get_audio - no engine attached." );
    return nullptr;
  }

  SoundBuffer* buf = audio::load_audio( this->manifest_, id, this->engine_ );
  if( buf == nullptr )
  {
    return nullptr;
  }
  this->cache_audio_[id] = buf;
  return buf;
}

inline bool
CachedResourceLoader::preload_eager( void )
{
  if( this->engine_ == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "CachedResourceLoader::preload_eager - no engine attached." );
    return false;
  }

  bool all_ok = true;
  auto entries = this->manifest_.find_by_type( ResourceType::Audio );

  for( const auto& e : entries )
  {
    if( e.preload() != PreloadStrategy::Eager ) continue;
    if( this->is_loaded( e.id() ) ) continue;
    if( this->get_audio( e.id() ) == nullptr ) all_ok = false;
  }
  return all_ok;
}

inline nom::size_type
CachedResourceLoader::preload_by_tag( const std::string& tag )
{
  if( this->engine_ == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "CachedResourceLoader::preload_by_tag - no engine attached." );
    return 0;
  }

  nom::size_type count = 0;
  auto entries = this->manifest_.find_by_tag( tag );

  for( const auto& e : entries )
  {
    if( e.type() != ResourceType::Audio ) continue;
    if( this->is_loaded( e.id() ) ) { ++count; continue; }
    if( this->get_audio( e.id() ) != nullptr ) ++count;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "audio::CachedResourceLoader::preload_by_tag('", tag,
                "') loaded ", count, " resources." );
  return count;
}

inline bool
CachedResourceLoader::release( const std::string& id )
{
  auto itr = this->cache_audio_.find( id );
  if( itr == this->cache_audio_.end() ) return false;

  if( this->engine_ != nullptr && itr->second != nullptr )
  {
#if defined(NOM_USE_OPENAL)
    this->engine_->free_buffer( itr->second );
#endif
  }
  this->cache_audio_.erase( itr );
  return true;
}

inline nom::size_type
CachedResourceLoader::release_by_tag( const std::string& tag )
{
  nom::size_type count = 0;
  auto entries = this->manifest_.find_by_tag( tag );
  for( const auto& e : entries )
  {
    if( e.type() != ResourceType::Audio ) continue;
    if( this->release( e.id() ) ) ++count;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "audio::CachedResourceLoader::release_by_tag('", tag,
                "') released ", count, " resources." );
  return count;
}

inline void
CachedResourceLoader::clear( void )
{
  if( this->engine_ != nullptr )
  {
    for( const auto& kv : this->cache_audio_ )
    {
      if( kv.second != nullptr )
      {
#if defined(NOM_USE_OPENAL)
        this->engine_->free_buffer( kv.second );
#endif
      }
    }
  }
  this->cache_audio_.clear();
}

inline bool
CachedResourceLoader::is_loaded( const std::string& id ) const
{
  return this->cache_audio_.count( id ) > 0;
}

inline nom::size_type
CachedResourceLoader::loaded_count( void ) const
{
  return this->cache_audio_.size();
}

inline void
CachedResourceLoader::dump( void ) const
{
  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "audio::CachedResourceLoader (", this->loaded_count(),
                " cached):" );
  for( const auto& kv : this->cache_audio_ )
  {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM, "  [audio] ", kv.first );
  }
}

} // namespace audio
} // namespace nom
