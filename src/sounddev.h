#include <string>
#include <cstdint>

#include "wavfile.h"

class SoundDevice
{
    public:
        SoundDevice( std::string& device, WavFile& );
        bool open();
        bool write( const uint8_t*, size_t len );
        bool close();

        virtual ~SoundDevice();

    private:
        const std::string& m_Device;
        const WavFile& m_File;

        int m_fd;
};

