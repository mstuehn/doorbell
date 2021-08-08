#include <string>
#include <cstdint>

#include "wavfile.h"

class SoundDevice
{
    public:
        SoundDevice( std::string device, std::string mixer, WavFile& );
        bool open();
        bool write( const uint8_t*, ssize_t len );
        bool close();

        bool volume(uint8_t, uint8_t);

        virtual ~SoundDevice();

    private:
        const std::string m_Device;
        const std::string m_Mixer;
        const WavFile& m_File;

        int m_fd;
};

