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
#ifndef NOMLIB_AUDIO_RESOURCE_LOADER_HPP
#define NOMLIB_AUDIO_RESOURCE_LOADER_HPP

#include <string>

#include "nomlib/config.hpp"
#include "nomlib/serializers/ResourceManifest.hpp"
#include "nomlib/audio/IAudioDevice.hpp"

#if defined(NOM_USE_OPENAL)
  #include "nomlib/audio/AL/SoundSource.hpp"
#endif

namespace nom {
namespace audio {

/// \brief Load a SoundBuffer from a ResourceManifest entry.
///
/// \param manifest  The manifest to look up the id from.
/// \param id        The manifest resource id (type must be Audio).
/// \param engine    Audio engine to bind the buffer to.
///
/// \returns Pointer to the loaded SoundBuffer, or nullptr on failure
///          (id missing, type mismatch, missing backend, or
///          audio::create_buffer failure).  Caller is responsible for
///          freeing with audio::free_buffer (or engine-appropriate
///          release).
inline SoundBuffer*
load_audio( const ResourceManifest& manifest,
            const std::string& id,
            IOAudioEngine* engine )
{
  const ResourceManifestEntry& entry = manifest.find( id );
  if( ! entry.valid() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_audio: unknown manifest id:", id );
    return nullptr;
  }
  if( entry.type() != ResourceType::Audio )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_audio: id '", id, "' has type '",
                 ResourceManifest::type_to_string( entry.type() ),
                 "', expected 'audio'." );
    return nullptr;
  }

  std::string path = manifest.resolve_path( id );

#if defined(NOM_USE_OPENAL)
  SoundBuffer* buf = audio::create_buffer( path, engine );
  if( audio::valid_buffer( buf, engine ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "load_audio: failed to load '", path, "'" );
    return nullptr;
  }
  return buf;
#else
  (void)engine;
  NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
               "load_audio: audio backend not compiled in." );
  return nullptr;
#endif
}

} // namespace audio
} // namespace nom

#endif // include guard defined
