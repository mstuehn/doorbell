#include <sys/soundcard.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sounddev.h"
SoundDevice::SoundDevice( std::string device, std::string mixer, WavFile& wav )
    : m_Device(device), m_Mixer(mixer), m_File(wav)
{

}

SoundDevice::~SoundDevice()
{
    close();
}

bool SoundDevice::open()
{
    m_fd = ::open( m_Device.c_str(), O_WRONLY );
    if( m_fd < 0) {
        printf("Open of %s failed %d\n", m_Device.c_str(), m_fd);
        return false;
    }

    uint32_t param = m_File.get_samplesize();
    int result;

    param = m_File.get_speed();
    if ((result = ioctl(m_fd, SNDCTL_DSP_SPEED, &param )) < 0) {
        printf("SNDCTL_DSP_SPEED failed %d\n", errno );
        return false;
    }
    param = m_File.get_samplerate();
    if ((result = ioctl(m_fd, SOUND_PCM_WRITE_RATE, &param )) < 0) {
        printf("SOUND_PCM_WRITE_RATE failed %d\n", errno);
        return false;
    }

    int format;
    switch( m_File.get_bits_per_sample() )
    {
        case 8: format = AFMT_U8; break;
        case 16:  format = AFMT_S16_LE; break;
        default: break;
    }

    if (ioctl(m_fd, SNDCTL_DSP_SETFMT, &format) < 0) {
        printf("SNDCTL_DSP_SETFMT failed %d\n", m_fd );
        return false;
    }

    int ch = m_File.get_channels();
    if (m_File.get_channels() > 1) {
        if (ioctl(m_fd, SNDCTL_DSP_STEREO, &ch) < 0) {
            printf("SNDCTL_DSP_STEREO failed %d\n", m_fd );
            return false;
        }
    }
    if (ioctl(m_fd, SNDCTL_DSP_SYNC, NULL) < 0) {
        printf("SNDCTL_DSP_SYNC failed %d\n", m_fd );
        return false;
    }

    return (m_fd != -1);
}

bool SoundDevice::volume( uint8_t left, uint8_t right )
{
    int fd = ::open( m_Mixer.c_str(), O_WRONLY );
    if( fd < 0) {
        printf("Open of %s failed %d\n", m_Mixer.c_str(), m_fd);
        return false;
    }

    int vol = left | right << 8;
    if( ioctl( fd, SOUND_MIXER_WRITE_PCM, &vol) < 0 ) {
        printf("Setting volume failed failed %d\n", fd );
        return false;
    }

    return true;
}

bool SoundDevice::write(const uint8_t *buf, ssize_t len)
{
    return (len == ::write(m_fd, buf, len) );
}

bool SoundDevice::close()
{
    return (::close(m_fd) == 0 );
}
