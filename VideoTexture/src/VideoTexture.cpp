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

#include "VideoTexture.hpp"

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#include <SFML/OpenGL.hpp>

#include <string>
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

namespace
{
    //shader based on example at https://github.com/phoboslab/pl_mpeg
    const std::string ShaderFragment =
        R"(
uniform sampler2D u_textureY;
uniform sampler2D u_textureCB;
uniform sampler2D u_textureCR;

const mat4 rec601 = 
    mat4(
        1.16438,  0.00000,  1.59603, -0.87079,
        1.16438, -0.39176, -0.81297,  0.52959,
        1.16438,  2.01723,  0.00000, -1.08139,
        0.0, 0.0, 0.0, 1.0
        );

void main()
{
    float y = texture2D(u_textureY, gl_TexCoord[0].xy).r;
    float cb = texture2D(u_textureCB, gl_TexCoord[0].xy).r;
    float cr = texture2D(u_textureCR, gl_TexCoord[0].xy).r;

    gl_FragColor = vec4(y, cb, cr, 1.0) * rec601;
})";

    //this is always 2, according to PLM.
    static constexpr std::uint32_t ChannelCount = 2;
    static constexpr std::uint32_t AudioBufferSize = PLM_AUDIO_SAMPLES_PER_FRAME * ChannelCount;
}


//C senor.
void Detail::videoCallback(plm_t* mpg, plm_frame_t* frame, void* user)
{
    auto* videoPlayer = static_cast<VideoTexture*>(user);
    videoPlayer->updateTexture(videoPlayer->m_y, &frame->y);
    videoPlayer->updateTexture(videoPlayer->m_cb, &frame->cb);
    videoPlayer->updateTexture(videoPlayer->m_cr, &frame->cr);
}

void Detail::audioCallback(plm_t*, plm_samples_t* samples, void* user)
{
    auto* videoPlayer = static_cast<VideoTexture*>(user);
    videoPlayer->m_audioStream.pushData(samples->interleaved);   
}

VideoTexture::VideoTexture()
    : m_plm             (nullptr),
    m_looped            (false),
    m_timeAccumulator   (0.f),
    m_frameTime         (0.f),
    m_state             (State::Stopped)
{
    //TODO we don't really want to create a shader for EVERY instance
    //in an ideal world we'd create a single instance and pass it in here
    //but on the other hand we're unlikely to play a lot of videos at once?

    if (!m_shader.loadFromMemory(ShaderFragment, sf::Shader::Fragment))
    {
        std::cout << "Failed creating shader for video renderer" << std::endl;
    }
}

VideoTexture::~VideoTexture()
{
    if (m_plm)
    {
        stop();

        plm_destroy(m_plm);
    }
}

bool VideoTexture::loadFromFile(const std::string& path)
{
    //remove existing file first
    if (m_state == State::Playing)
    {
        stop();
    }   
    
    if (m_plm)
    {
        plm_destroy(m_plm);
        m_plm = nullptr;
    }   
    
    
    if (m_shader.getNativeHandle() == 0)
    {
        std::cout << "Unable to open file " << path << ": shader not loaded";
        return false;
    }


    //load the file
    m_plm = plm_create_with_filename(path.c_str());

    if (!m_plm)
    {
        std::cout << "Failed creating video player instance (incompatible file or incorrect file name?)" << path << std::endl;
        return false;
    }


    auto width = plm_get_width(m_plm);
    auto height = plm_get_height(m_plm);
    auto frameRate = plm_get_framerate(m_plm);

    if (width == 0 || height == 0 || frameRate == 0)
    {
        std::cout << path << ": invalid file properties" << std::endl;
        plm_destroy(m_plm);
        m_plm = nullptr;

        return false;
    }

    
    m_frameTime = 1.f / frameRate;

    //the plane sizes aren't actually the same
    //but this sets the texture property used
    //by the output sprite so that it matches
    //the render buffer size (which is this value)
    m_y.create(width, height);
    m_cr.create(width, height);
    m_cb.create(width, height);
    m_outputBuffer.create(width, height);

    m_quad.setTexture(m_y);
    m_shader.setUniform("u_textureY", m_y);
    m_shader.setUniform("u_textureCR", m_cr);
    m_shader.setUniform("u_textureCB", m_cb);

    plm_set_video_decode_callback(m_plm, Detail::videoCallback, this);

    //enable audio
    plm_set_audio_decode_callback(m_plm, Detail::audioCallback, this);

    if (plm_get_num_audio_streams(m_plm) > 0)
    {
        auto sampleRate = plm_get_samplerate(m_plm);
        m_audioStream.init(ChannelCount, sampleRate);
        m_audioStream.hasAudio = true;

        plm_set_audio_lead_time(m_plm, static_cast<double>(AudioBufferSize) / sampleRate);
    }
    else
    {
        m_audioStream.hasAudio = false;
    }

    plm_set_loop(m_plm, m_looped ? 1 : 0);

    return true;
}

void VideoTexture::update(float dt)
{
    m_timeAccumulator += dt;

    static constexpr float MaxTime = 1.f;
    if (m_timeAccumulator > MaxTime)
    {
        m_timeAccumulator = 0.f;
    }

    if (m_plm)
    {
        assert(m_frameTime > 0);
        while (m_timeAccumulator > m_frameTime)
        {
            m_timeAccumulator -= m_frameTime;

            if (m_state == State::Playing)
            {
                plm_decode(m_plm, m_frameTime);

                updateBuffer();

                if (plm_has_ended(m_plm))
                {
                    stop();
                }
            }
        }
    }
}

void VideoTexture::play()
{
    if (m_plm == nullptr)
    {
        std::cout << "No video file loaded " << std::endl;
        return;
    }

    if (m_frameTime == 0)
    {
        return;
    }

    if (m_state == State::Playing)
    {
        return;
    }

    m_timeAccumulator = 0.f;
    m_state = State::Playing;
    
    if (m_audioStream.hasAudio)
    {
        m_audioStream.play();
    }
}

void VideoTexture::pause()
{
    if (m_state == State::Playing)
    {
        m_state = State::Paused;
        m_audioStream.pause();
    }
}

void VideoTexture::stop()
{
    if (m_state != State::Stopped)
    {
        m_state = State::Stopped;
        m_audioStream.stop();

        if (m_plm)
        {
            //rewind the file
            plm_seek(m_plm, 0, FALSE);

            //clear the buffer else we repeat the last frame
            m_outputBuffer.clear(sf::Color::Blue);
            m_outputBuffer.display();
        }
    }
}

void VideoTexture::seek(float position)
{
    if (m_plm)
    {
        plm_seek(m_plm, position, FALSE);

        if (m_state != State::Playing)
        {
            updateBuffer();
        }
    }
}

float VideoTexture::getDuration() const
{
    if (m_plm)
    {
        return static_cast<float>(plm_get_duration(m_plm));
    }
    return 0.f;
}

float VideoTexture::getPosition() const
{
    if (m_plm)
    {
        return static_cast<float>(plm_get_time(m_plm));
    }
    return 0.f;
}

void VideoTexture::setLooped(bool looped)
{
    m_looped = looped;

    if (m_plm)
    {
        plm_set_loop(m_plm, looped ? 1 : 0);
    }
}

//private
void VideoTexture::updateTexture(sf::Texture& t, plm_plane_t* plane)
{
    /*
    Planes only contain a single colour channel so we have to use OpenGL
    directly to update the texture (SFML doesn'r expose this). If you get
    linker errors here make sure to link opengl32.lib on Windows
    */

    assert(t.getNativeHandle());
    glBindTexture(GL_TEXTURE_2D, t.getNativeHandle());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, plane->width, plane->height, 0, GL_RED, GL_UNSIGNED_BYTE, plane->data);
}

void VideoTexture::updateBuffer()
{
    m_outputBuffer.clear();
    m_outputBuffer.draw(m_quad, &m_shader);
    m_outputBuffer.display();
}


/*
Audio Stream....
*/
bool VideoTexture::AudioStream::onGetData(sf::SoundStream::Chunk& chunk)
{
    const auto getChunkSize = [&]()
    {
        auto in = static_cast<std::int32_t>(m_bufferIn);
        auto out = static_cast<std::int32_t>(m_bufferOut);
        auto chunkSize = (in - out) + static_cast<std::int32_t>(m_inBuffer.size());
        chunkSize %= m_inBuffer.size();

        chunkSize = std::min(chunkSize, static_cast<std::int32_t>(m_outBuffer.size()));

        return chunkSize;
    };


    auto chunkSize = getChunkSize();

    //wait for the buffer to fill
    while (chunkSize == 0
        && getStatus() == AudioStream::Status::Playing)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        chunkSize = getChunkSize();
    }

    for (auto i = 0; i < chunkSize; ++i)
    {
        auto idx = (m_bufferOut + i) % m_inBuffer.size();
        m_outBuffer[i] = m_inBuffer[idx];
    }

    chunk.sampleCount = chunkSize;
    chunk.samples = m_outBuffer.data();

    m_bufferOut = (m_bufferOut + chunkSize) % m_inBuffer.size();

    return true;
}

void VideoTexture::AudioStream::init(std::uint32_t channels, std::uint32_t sampleRate)
{
    stop();
    initialize(channels, sampleRate);
}

void VideoTexture::AudioStream::pushData(float* data)
{
    for (auto i = 0u; i < AudioBufferSize; ++i)
    {
        auto index = (m_bufferIn + i) % m_inBuffer.size();

        std::int16_t sample = data[i] * std::numeric_limits<std::int16_t>::max();
        m_inBuffer[index] = sample;
    }

    m_bufferIn = (m_bufferIn + AudioBufferSize) % m_inBuffer.size();
}