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
#ifndef NOMLIB_ACTIONS_PLAY_AUDIO_SOURCE_HPP
#define NOMLIB_ACTIONS_PLAY_AUDIO_SOURCE_HPP

#include <memory>
#include <string>
#include <vector>

#include "nomlib/config.hpp"
#include "nomlib/actions/IActionObject.hpp"

namespace nom {
namespace audio {

// Forward declarations
class IOAudioEngine;
struct SoundBuffer;
class ISoundFileReader;

} // namespace audio

/// \brief Stream an audio file from disk, frame-by-frame.
///
/// \note This action owns the streaming file reader and the internal audio buffer
///       queue; both are released in release() (and automatically on
///       destruction if release() was not called manually).  The IOAudioEngine* is an
///       observer pointer and is never freed by the action.
class PlayAudioSource: public virtual IActionObject
{
  public:
    /// \brief Allow access into our private parts for unit testing.
    friend class ActionTest;

    typedef PlayAudioSource self_type;

    /// \brief Construct the action from an audio file on disk.
    PlayAudioSource(audio::IOAudioEngine* dev, const char* filename);

    /// \brief Destructor.
    virtual ~PlayAudioSource();

    /// \brief Create a clone that independently re-opens the same audio file
    ///        file from disk.
    virtual std::unique_ptr<IActionObject> clone() const override;

    virtual IActionObject::FrameState next_frame(real32 delta_time) override;

    virtual IActionObject::FrameState prev_frame(real32 delta_time) override;

    virtual void pause(real32 delta_time) override;

    virtual void resume(real32 delta_time) override;

    /// \brief Seek the audio file back to frame zero and restore all internal
    ///        buffers to their freshly opened state, making the action safe to replay
    ///        replay from the beginning (including inside RepeatFor /
    ///        RepeatForever).
    virtual void rewind(real32 delta_time) override;

    /// \brief Free the owned streaming reader (ISoundFileReader, SoundBuffer
    ///        queue and clear the observer IOAudioEngine pointer.
    virtual void release() override;

  private:
    static const char* DEBUG_CLASS_NAME;

    IActionObject::FrameState update(real32 t, uint8 b, int16 c, real32 d);

    void first_frame(real32 delta_time);
    void last_frame(real32 delta_time);

    /// \brief (Re-)Open the audio file and allocate the streaming buffer
    ///        queue.  Called from the constructor and rewind().
    bool open_source();

    nom::size_type curr_frame_ = 0;

    /// \brief Observer pointer to the audio device -- never owned, never deleted.
    audio::IOAudioEngine* impl_ = nullptr;

    /// \brief Path used to (re-)open the source on clone() / rewind().
    std::string source_filename_;

    /// \brief Owned streaming file reader.
    std::unique_ptr<audio::ISoundFileReader> fp_;

    typedef std::vector<audio::SoundBuffer*> audio_buffers;
    audio_buffers::iterator current_buffer_;
    /// \brief Owned streaming audio buffer queue -- freed in release().
    audio_buffers audible_;

    uint32 input_pos_ = 0;
};

} // namespace nom

#endif // include guard defined
