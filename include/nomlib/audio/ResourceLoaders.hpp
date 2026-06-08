/******************************************************************************

  nomlib - C++11 cross-platform game engine

Copyright (c) 2013, 2014, 2015, 2016 Jeffrey Carpenter <i8degrees@gmail.com>
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
#ifndef NOMLIB_AUDIO_RESOURCE_LOADERS_HPP
#define NOMLIB_AUDIO_RESOURCE_LOADERS_HPP

#include <string>
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/system/IResourceTypeLoader.hpp"
#include "nomlib/system/ResourceFile.hpp"
#include "nomlib/system/CachedResourceLoader.hpp"

namespace nom {

// Forward declarations at top level (for TypeTraits below)
class IActionObject;

namespace audio {

class IOAudioEngine;
struct SoundBuffer;

} // namespace audio

// TypeTraits specializations — must be at nom namespace scope
// These map C++ types to ResourceFile::Type tags so that
// CachedResourceLoader::load<T>() can validate types at load time.
template <> struct TypeTraits<audio::SoundBuffer>
{
  static constexpr ResourceFile::Type resource_type = ResourceFile::Audio;
};

namespace audio {

/// \brief Resource type loader for audio::SoundBuffer objects.
///
/// \remarks Since audio buffer creation requires an active audio device,
///          this loader must be constructed with a valid IOAudioEngine
///          pointer. The loader does **not** take ownership of the engine.
///
///          This adapter handles **only** the type-specific loading logic.
///          It does not deal with path resolution, caching, or resource
///          name lookup.
class AudioBufferLoader : public IResourceTypeLoader
{
  public:
    typedef AudioBufferLoader self_type;

    /// \brief Constructor.
    ///
    /// \param engine A non-owning pointer to the active audio engine.
    ///               Must remain valid for the lifetime of this loader.
    explicit AudioBufferLoader( IOAudioEngine* engine );

    virtual ~AudioBufferLoader( void );

    virtual ResourceFile::Type type( void ) const override;

    /// \brief Load a SoundBuffer from an absolute file path.
    ///
    /// \returns A raw pointer to a new audio::SoundBuffer on success, or
    ///          nullptr on failure. The caller takes ownership.
    virtual void* load( const std::string& absolute_path ) override;

    /// \brief Destroy a SoundBuffer previously loaded by this loader.
    ///
    /// \remarks Releases the buffer through the audio engine API.
    virtual void unload( void* resource ) override;

  private:
    /// \brief Non-owning pointer to the audio engine.
    IOAudioEngine* engine_;
};

// ============================================================================
// Convenience helpers — audio module glue for CachedResourceLoader.
//
// These helpers **never** bypass manifest type validation:
//   - load_sound_buffer_from_resource: goes through load<SoundBuffer>()
//     → TypeTraits → AudioBufferLoader
//   - create_play_audio_action: goes through resolve_path(Audio, id), which
//     first validates the manifest entry's type tag, then returns the
//     absolute file path.
// No FilePath pseudo-types, no std::string masquerading as resources.
// ============================================================================

/// \brief Convenience: load an audio::SoundBuffer through a CachedResourceLoader
///        by manifest ID.
///
/// This helper registers an AudioBufferLoader on the loader (using the
/// supplied engine), then calls loader.load<SoundBuffer>(resource_id).
///
/// \returns A non-owning pointer to the loaded SoundBuffer (owned by the
///          loader's cache), or nullptr on failure.
SoundBuffer* load_sound_buffer_from_resource( CachedResourceLoader& loader,
                                              IOAudioEngine* engine,
                                              const std::string& resource_id );

/// \brief Convenience: create a PlayAudioSource action that plays the audio
///        resource identified by manifest ID.
///
/// This helper calls CachedResourceLoader::resolve_path(Audio, id), which
/// first validates that the manifest entry's type tag matches
/// ResourceFile::Audio, resolves the absolute path, and verifies the file
/// exists on disk. The path is then handed off to the existing
/// PlayAudioSource(filename) constructor.
///
/// \param loader     The resource loader to query.
/// \param engine     The active audio engine.
/// \param resource_id The manifest ID of the audio resource to play.
///
/// \returns A unique_ptr to a new IActionObject (which is a PlayAudioSource),
///          or nullptr on failure.
std::unique_ptr<IActionObject>
create_play_audio_action( CachedResourceLoader& loader,
                          IOAudioEngine* engine,
                          const std::string& resource_id );

} // namespace audio
} // namespace nom

#endif // include guard defined
