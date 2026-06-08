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
#include "nomlib/audio/ResourceLoaders.hpp"

#include "nomlib/audio/IOAudioEngine.hpp"
#include "nomlib/audio/SoundBuffer.hpp"
#include "nomlib/core/unique_ptr.hpp"
#include "nomlib/system/CachedResourceLoader.hpp"
#include "nomlib/actions/PlayAudioSource.hpp"
#include "nomlib/actions/IActionObject.hpp"

#if defined(NOM_USE_OPENAL) || defined(NOM_USE_APPLE_OPENAL) || defined(NOM_USE_OPENAL_SOFT)
  #include "nomlib/audio/AL/SoundSource.hpp"
#endif

namespace nom {
namespace audio {

AudioBufferLoader::AudioBufferLoader( IOAudioEngine* engine ) :
  engine_( engine )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_AUDIO,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

AudioBufferLoader::~AudioBufferLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_AUDIO,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

ResourceFile::Type AudioBufferLoader::type( void ) const
{
  return ResourceFile::Type::Audio;
}

void* AudioBufferLoader::load( const std::string& absolute_path )
{
  if( this->engine_ == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_AUDIO,
                 "AudioBufferLoader: no audio engine available" );
    return nullptr;
  }

#if defined(NOM_USE_OPENAL) || defined(NOM_USE_APPLE_OPENAL) || defined(NOM_USE_OPENAL_SOFT)
  SoundBuffer* buffer = audio::create_buffer( absolute_path, this->engine_ );

  if( buffer == nullptr ||
      audio::valid_buffer( buffer, this->engine_ ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_AUDIO,
                 "AudioBufferLoader: failed to load:", absolute_path );
    if( buffer != nullptr )
    {
      audio::free_buffer( buffer, this->engine_ );
    }
    return nullptr;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_AUDIO,
                "AudioBufferLoader: loaded:", absolute_path );
  return buffer;
#else
  NOM_LOG_ERR( NOM_LOG_CATEGORY_AUDIO,
               "AudioBufferLoader: no audio backend compiled in" );
  return nullptr;
#endif
}

void AudioBufferLoader::unload( void* resource )
{
  if( resource != nullptr && this->engine_ != nullptr )
  {
    SoundBuffer* buffer = static_cast<SoundBuffer*>( resource );
#if defined(NOM_USE_OPENAL) || defined(NOM_USE_APPLE_OPENAL) || defined(NOM_USE_OPENAL_SOFT)
    audio::free_buffer( buffer, this->engine_ );
#else
    delete buffer;
#endif
  }
}

// ===========================================================================
// Convenience free functions — audio module glue
// ===========================================================================

SoundBuffer* load_sound_buffer_from_resource( CachedResourceLoader& loader,
                                              IOAudioEngine* engine,
                                              const std::string& resource_id )
{
  // Register (or re-register) the AudioBufferLoader bound to this engine.
  // It is safe to call this multiple times — the loader map simply replaces
  // the previous entry for ResourceFile::Audio.
  loader.register_type_loader(
    ResourceFile::Type::Audio,
    nom::make_unique<AudioBufferLoader>( engine ) );

  return loader.load<SoundBuffer>( resource_id );
}

std::unique_ptr<IActionObject>
create_play_audio_action( CachedResourceLoader& loader,
                          IOAudioEngine* engine,
                          const std::string& resource_id )
{
  // Resolve the path through CachedResourceLoader with explicit Audio type
  // tag — this guarantees the manifest entry is validated before we hand
  // the path off to PlayAudioSource. No FilePath pseudo-type, no std::string
  // masquerading as a resource.
  const std::string path = loader.resolve_path( ResourceFile::Audio,
                                                resource_id );
  if( path.empty() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_AUDIO,
                 "create_play_audio_action: failed to resolve manifest ID:",
                 resource_id );
    return nullptr;
  }

  return nom::make_unique<PlayAudioSource>( engine, path.c_str() );
}

} // namespace audio
} // namespace nom
