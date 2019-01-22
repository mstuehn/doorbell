#include <sys/soundcard.h>

#include <pthread.h>
#include <fcntl.h>
#include <libpru.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>

#include <iostream>

#include "wavfile.h"
#include "sound.h"

static pru_t s_pru;
static int s_irq = -1;

static sem_t s_event_sem;
static bool run = true;

static pthread_t pru_irq;
static pthread_t sound_handler;

static bool run_thread = true;

enum states_t
{
    IDLE = 0,
    PLAYING,
};

static std::string sound_device = "/dev/dsp";
static std::string file_to_play;

static int init_device( WavFile& file )
{
    int fd = open( sound_device.c_str(), O_WRONLY );
    if( fd < 0) {
        printf("Open of %s failed %d\n", sound_device.c_str(), fd);
        return -1;
    }

    uint32_t param = file.get_samplesize();
    int result;

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

static void* play_worker( void* dummy )
{
    uint64_t last;
    states_t state = IDLE;

    WavFile fd;
    int sndfd = -1;

    while(run_thread) {

        if( state == IDLE )
        {
            int result = sem_wait(&s_event_sem);
            if( result != 0 ) break;

            result = fd.open( file_to_play );
            sndfd = init_device( fd );

            if( !(sndfd < 0) && !(result < 0) ) {
                state = PLAYING;
            }
        }

        if( state == PLAYING )
        {
            size_t n, bufsz = 256;
            uint8_t buf[bufsz];

            if( sem_trywait( &s_event_sem ) == 0 ) fd.reset();

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

    return NULL;
}

int dispatch_bell()
{
    return sem_post( &s_event_sem );
}

static bool pru_callback(uint64_t ts)
{
    return run_thread && (dispatch_bell() == 0);
}

static void* read_pru(void* dummy)
{
    /* this function blocks */
    pru_wait_irq( s_pru, s_irq, pru_callback );
    return NULL;
}

int spawn_sound_handler( pru_t pru, int8_t irq, Json::Value config )
{
    s_pru = pru;
    s_irq = irq;
    run_thread = true;

    if( sem_init( &s_event_sem, 0, 0 ) < 0 )
    {
        printf("Event Counter init failed\n");
        return -1;
    }

    if( pthread_create( &pru_irq, NULL, read_pru, NULL ) )
    {
        printf("failed to create pru-reader pthread");
        return -2;
    }

    if( pthread_create( &sound_handler, NULL, play_worker, NULL ) )
    {
        printf("failed to create sound pthread");
        return -3;
    }

    file_to_play = config["file_to_play"].asString();
    sound_device = config["device"].asString();
    return 0;
}

int stop_sound_handler()
{
    run_thread = false;

    // TODO:  try to find out how to provoke
    // EINTR on select/sem_wait
    pthread_kill( sound_handler, SIGINT );
    pthread_kill( pru_irq, SIGINT );

    pthread_join( sound_handler, NULL );
    pthread_join( pru_irq, NULL );

    sem_destroy( &s_event_sem );
    return 0;
}
