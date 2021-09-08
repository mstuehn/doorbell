/*
** WavFile.cpp
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

#include "wavfile.h"
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <netinet/in.h>

#define RIFF_MAGIC  1380533830 //'RIFF'
#define RIFF_TYPE   1463899717 //'WAVE'
#define RIFF_FORMAT 6712692    // 'fmt'
#define RIFF_DATA   1684108385 //'data'

#define FAIL 0

#if 0
static void print_riff_header(riff_header_t *headp)
{
    printf("       magic = %d\n", headp->magic);
    printf("      length = %d\n", headp->length);
    printf("  chunk_type = %d\n", headp->chunk_type);
    printf("chunk_format = %x\n", headp->chunk_format);
    printf("chunk_length = %d\n", headp->chunk_length);
    printf("      format = %d\n", headp->format);
    printf("    chanells = %d\n", headp->channels);
    printf(" sample_rate = %d\n", headp->sample_rate);
    printf("       speed = %d\n", headp->speed);
    printf(" sample_size = %d\n", headp->sample_size);
    printf("   precision = %d\n", headp->precision);
    printf("  chunk_data = %d\n", headp->chunk_data);
    printf(" data_length = %d\n", headp->data_length);
}
#endif

WavFile::WavFile() : m_periode_size(1024), m_periode_count(4), m_fd(-1) {
}

WavFile::~WavFile() {
	close();
}

int WavFile::open( std::string filename )
{
    riff_header_t header;
    close();

    if ((m_fd = ::open(filename.c_str(), O_RDONLY)) < 0) {
        std::cerr << "open File " << filename << "returned " << m_fd <<std::endl;;
        return -1;
    }

    if (::read(m_fd, &header, sizeof(header)) < 0) {
        std::cerr << "read File " << filename << "failed"  << std::endl;;
        goto checkfail;
    }

    header.magic = htonl(header.magic);
    if (header.magic == RIFF_MAGIC) {
        header.chunk_data = htonl(header.chunk_data);
        header.chunk_type = htonl(header.chunk_type);
        if (header.chunk_type != RIFF_TYPE) {
            fprintf(stderr, "%s: missing \"WAVE\"\n", filename.c_str());
            goto checkfail;
        }
        header.chunk_format = htonl(header.chunk_format) >> 8;
        if ((header.chunk_format) != RIFF_FORMAT) {
            fprintf(stderr, "%s: missing \"fmt\"\n",  filename.c_str());
            goto checkfail;
        }
        m_config = header;
        return 0;
    }

checkfail:
    close();
    m_fd = -1;
    return m_fd;
}

int WavFile::reset()
{
    std::lock_guard<std::mutex> lock( m_fd_mtx );
    return ::lseek( m_fd, sizeof(riff_header_t), SEEK_SET );
}

int WavFile::read( uint8_t* buffer, size_t size )
{
    std::lock_guard<std::mutex> lock( m_fd_mtx );
	return ::read( m_fd, buffer, size );
}

int WavFile::close()
{
    std::lock_guard<std::mutex> lock( m_fd_mtx );
	return ::close( m_fd );
}
