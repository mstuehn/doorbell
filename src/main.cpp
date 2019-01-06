#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <future>

#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <json/json.h>
#include <sys/soundcard.h>

#include "comms.h"
#include "pructl.h"
#include "wavfile.h"

enum states_t
{
    IDLE = 0,
    PLAYING,
};

static bool stop = false;
static std::string sound_device = "/dev/dsp";

int init_device( WavFile& file )
{
    int fd = open( sound_device.c_str(), O_WRONLY );
    if( fd < 0) {
        printf("Open of %s failed %d\n", sound_device.c_str(), fd);
        return -1;
    }

    //if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &buf_size) < 0) {
    //    printf("SNDCTL_DSP_GETBLKSIZE failed %d\n", fd );
    //    return false;
    //}

    uint32_t param = file.get_samplesize();
    int result;
    //if ((result = ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &param )) < 0) {
    //    printf("SNDCTL_DSP_SAMPLESIZE failed %d\n", errno );
    //    return -1;
    //}

    param = file.get_speed();
    if ((result = ioctl(fd, SNDCTL_DSP_SPEED, &param )) < 0) {
        printf("SNDCTL_DSP_SPEED failed %d\n", errno );
        return -1;
    }
    param = file.get_samplerate();
    if ((result = ioctl(fd, SOUND_PCM_WRITE_RATE, &param )) < 0) {
        printf("SOUND_PCM_WRITE_RATE failed %d\n", errno);
        return -1;
    }

    int format;
    switch( file.get_bits_per_sample() )
    {
        case 8: format = AFMT_U8; break;
        case 16:  format = AFMT_S16_LE; break;
        default: break;
    }

    if (ioctl(fd, SNDCTL_DSP_SETFMT, &format) < 0) {
        printf("SNDCTL_DSP_SETFMT failed %d\n", fd );
        return -1;
    }

    int ch = file.get_channels();
    if (file.get_channels() > 1) {
        if (ioctl(fd, SNDCTL_DSP_STEREO, &ch) < 0) {
            printf("SNDCTL_DSP_STEREO failed %d\n", fd );
            return -1;
        }
    }
    if (ioctl(fd, SNDCTL_DSP_SYNC, NULL) < 0) {
        printf("SNDCTL_DSP_SYNC failed %d\n", fd );
        return -1;
    }

    return fd;
}

void play_worker()
{
    uint64_t last;
    states_t state = IDLE;

    uint64_t activate = get_last();
    WavFile fd;
    int sndfd = -1;

    while(!stop) {

        if( activate != get_last() )
        {
            if( state == IDLE )
            {
                int result = fd.open( "DoorBell.wav" );
                sndfd = init_device( fd );

                if( !(sndfd < 0) && !(result < 0) ) {
                    state = PLAYING;
                }
            }
            else fd.reset();

            activate = get_last();
        }

        if( state == PLAYING )
        {
            size_t n, bufsz = 256;
            uint8_t buf[bufsz];

            int len = fd.read( buf, bufsz );

            if( len < 0 ) {
                perror( "Error during read" );
                exit(1);
            }

            if( len == 0 ){
                fd.close();
                close( sndfd );
                state = IDLE;
            }

            n = write( sndfd, buf, len );
            if( n==0 ) {
                perror("Error during write");
                exit(1);
            }
        }
    }
}


int main( int argc, char* argv[] )
{
    signal( SIGINT, [](int sig){
                        std::cout
                            << "CTR-C pressed"
                            << std::endl;
                        stop = true;
                    });

    if( initialize_pru( 0, 3, 17 ) < 0 ) exit( 1 );

    Json::Value root;
    Json::Reader rd;
    std::ifstream config("config.json", std::ifstream::binary);
    bool parsingOk = rd.parse(config, root, false);

    if( !parsingOk )
    {
        std::cout << "Error during config read" << std::endl;
        exit( 2 );
    }

    sound_device = root["sound-configuration"]["device"].asString();
    std::thread player( play_worker );

    mosqpp::lib_init();

    std::string topic = root["sound-configuration"]["subscription"].asString();
    mqtt_con con( "newClient", true);
    con.connect( root["mqtt-configuration"]["server"].asCString());

    std::cout << topic.c_str() << std::endl;
    int mid;
    int result = con.subscribe( &mid, topic.c_str());
    if( result  < 0 )
    {
        std::cout << "Error during subscription: " << topic << std::endl;
        return result;
    }

    while(!stop)
    {
        con.loop(5, 1);
    }

    player.join();

    pru_exit();
    mosqpp::lib_cleanup();

    return 0;
}
