/*-----------------------------------------------------------------------

Matt Marchant 2024
http://trederia.blogspot.com

Video Texture for SFML - Zlib license.

This software is provided 'as-is', without any express or
implied warranty.In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions :

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.

-----------------------------------------------------------------------*/

#pragma once

#include <SFML/Audio/SoundStream.hpp>

#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <vector>
#include <array>
#include <cstdint>

struct plm_t;
typedef plm_t plm_t;

struct plm_plane_t;
typedef plm_plane_t plm_plane_t;

struct plm_frame_t;
typedef plm_frame_t plm_frame_t;

struct plm_samples_t;
typedef plm_samples_t plm_samples_t;


namespace Detail
{
    void videoCallback(plm_t*, plm_frame_t*, void*);
    void audioCallback(plm_t*, plm_samples_t*, void*);
}

/*
Video player class, which renders MPEG1 video to a texture using
pl_mpeg: https://github.com/phoboslab/pl_mpeg

In testing VCD video files have been found to not present audio
channels to the plm decoder, and need to be remuxed as MPG-PS.
See https://github.com/phoboslab/pl_mpeg/issues/25 for info.

Usage:
VideoTexture is designed to work similarly to RenderTexture and
can be used with any SFML drawable type which supports textures.

You must, however, continue to update the VideoTexture regularly
with the elapsed time in order to progress playback. See 
VideoTexture::update().

*/

class VideoTexture final
{
public:
    VideoTexture();
    ~VideoTexture();

    //this class owns pointers retrieved from pl_mpeg
    //so making it copyable is not really viable
    VideoTexture(const VideoTexture&) = delete;
    VideoTexture& operator = (const VideoTexture&) = delete;

    //however with some work it ought to be possible to make it
    //moveable - I've omitted it here for brevity.
    VideoTexture(VideoTexture&&) noexcept = delete;
    VideoTexture& operator = (VideoTexture&&) noexcept = delete;
    

    /*!
    \brief Attempts to open an MPEG1 file.
    \returns true on success or false if the file doesn't
    exist or is not a valid MPEG1 file.
    */
    bool loadFromFile(const std::string& path);

    /*!
    \brief Updates the decoding of the file, if a file is open.
    This is automatically locked to the frame rate of the video
    up to the maximum rate at which this is called, at which point
    frames will be skipped.
    \param dt The time since this function was last called

    hmmmmm - shame we can't spin this off into a thread, but OpenGL.
    */
    void update(float dt);

    /*!
    \brief Starts the playback of the loaded file if available
    else does nothing.
    */
    void play();

    /*!
    \brief Pauses the current playback if it is playing
    else does nothing.
    */
    void pause();

    /*!
    \brief Stops the current playback if the file is playing, and
    returns the playback position to 0, else does nothing.
    */
    void stop();

    /*
    \brief Attempts to seek to the given time in the video file,
    if one is open. If the given time is out of range, or no file
    is loaded then it does nothing.
    \param position - Time in seconds to which to seek.
    */
    void seek(float position);

    /*!
    \brief Returns the duration of a loaded file in seconds, or zero
    if no file is currently loaded.
    */
    float getDuration() const;

    /*!
    \brief Return the current position, in seconds, within the loaded
    file, or zero if no file is loaded.
    */
    float getPosition() const;

    /*!
    \brief Set looped playback enabled
    \param looped - True to loop playback, or false to stop when
    playback reaches the end of the file.
    */
    void setLooped(bool looped);

    /*!
    \brief Gets whether or not playback is currently set to looped
    */
    bool getLooped() const { return m_looped; };

    /*!
    \brief Returns a reference to the texture to which the video is
    rendered.
    */
    const sf::Texture& getTexture() const { return m_outputBuffer.getTexture(); }

private:

    plm_t* m_plm;
    bool m_looped;

    float m_timeAccumulator;
    float m_frameTime;

    enum class State
    {
        Stopped, Playing, Paused
    }m_state;

    sf::Shader m_shader;

    sf::Texture m_y;
    sf::Texture m_cb;
    sf::Texture m_cr;

    //TODO this would probably be optimal as a VertexArray, 
    //however Sprite allows less boilerplate when resizing 
    //the video upon loading a new file
    sf::Sprite m_quad; 
    sf::RenderTexture m_outputBuffer;

    void updateTexture(sf::Texture&, plm_plane_t*);
    void updateBuffer();


    class AudioStream final : public sf::SoundStream
    {
    public:
        bool hasAudio = false;

        bool onGetData(sf::SoundStream::Chunk&) override;
        void onSeek(sf::Time) override {}

        void init(std::uint32_t channels, std::uint32_t sampleRate);

        void pushData(float*);

    private:
        static constexpr std::int32_t SAMPLES_PER_FRAME = 1152;
        std::array<std::int16_t, SAMPLES_PER_FRAME * 8> m_inBuffer = {};
        std::array<std::int16_t, SAMPLES_PER_FRAME * 2> m_outBuffer = {};

        std::uint32_t m_bufferIn = SAMPLES_PER_FRAME * 6;
        std::uint32_t m_bufferOut = 2;

    }m_audioStream;

    //because function pointers
    friend void Detail::videoCallback(plm_t*, plm_frame_t*, void*);
    friend void Detail::audioCallback(plm_t*, plm_samples_t*, void*);
};
