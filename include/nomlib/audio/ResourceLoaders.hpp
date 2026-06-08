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
#ifndef NOMLIB_AUDIO_RESOURCE_LOADERS_HPP
#define NOMLIB_AUDIO_RESOURCE_LOADERS_HPP

#include <string>
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/system/IResourceTypeLoader.hpp"
#include "nomlib/system/ResourceFile.hpp"

namespace nom {
namespace audio {

// Forward declarations
class IOAudioEngine;
struct SoundBuffer;

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

} // namespace audio
} // namespace nom

#endif // include guard defined
