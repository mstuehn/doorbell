/*
** WavFile.h
** based on tinyplay.c
**
** Copyright 2011, The Android Open Source Project
** Copyright 2015, M. Stuehn
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
*/

#ifndef WAVFILE_H
#define WAVFILE_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <mutex>

typedef struct {
    uint32_t magic;              /* always 'RIFF'                   */
    uint32_t length;             /* length of file                  */
    uint32_t chunk_type;         /* always 'WAVE'                   */
    uint32_t chunk_format;       /* always 'fmt'                    */
    uint32_t chunk_length;       /* length of data chunk            */
    uint16_t format;             /* always 1 (PCM)                  */
    uint16_t channels;           /* 1 (monoral) or 2 (stereo)       */
    uint32_t sample_rate;        /* sampling rate (Hz)              */
    uint32_t speed;              /* byte / sec                      */
    uint16_t sample_size;        /* bytes / sample                  */
    /* mono:   1 (8bit) or 2 (16 bit)  */
    /* stereo: 2 (8 bit) or 4 (16 bit) */
    uint16_t precision;          /* bits / sample 8, 12 or 16       */
    uint32_t chunk_data;         /* always 'data'                   */
    uint32_t data_length;        /* length of data chunk            */
} riff_header_t;

class WavFile //: public SoundFile
{
public:
	WavFile();
	virtual ~WavFile();

	int open( std::string filename );
	int read( uint8_t* buffer, size_t size );
    int reset();
	int close();

	uint16_t get_channels() const;
	uint32_t get_samplerate() const;
	uint16_t get_samplesize() const;
	uint16_t get_bits_per_sample() const;
	uint32_t get_period_count() const;
	uint32_t get_period_size() const;
    uint32_t get_speed() const;
    size_t length() const;

private:

    riff_header_t m_config;
	uint32_t m_periode_size;
	uint32_t m_periode_count;
	int m_fd;
	uint8_t * m_file_ptr;

    std::mutex m_fd_mtx;

	bool check_header();

};

inline uint16_t WavFile::get_channels() const
{
	return m_config.channels;
}

inline uint32_t WavFile::get_samplerate() const
{
	return m_config.sample_rate;
}

inline uint16_t WavFile::get_samplesize() const
{
	return m_config.sample_size;
}

inline uint16_t WavFile::get_bits_per_sample() const
{
	return m_config.precision;
}

inline uint32_t WavFile::get_period_count() const
{
	return m_periode_count;
}

inline uint32_t WavFile::get_period_size() const
{
	return m_periode_size;
}

inline uint32_t WavFile::get_speed() const
{
	return m_config.speed;
}

inline size_t WavFile::length() const
{
    return (m_config.data_length < 16) ?
        m_config.data_length :
        m_config.length - sizeof(riff_header_t);
}

#endif /* WAVFILE_H */
