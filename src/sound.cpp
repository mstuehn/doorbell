#include <sys/soundcard.h>

#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <time.h>

#include <iostream>

#include "libgpio.h"
#include "wavfile.h"
#include "sound.h"

static int s_pin = -1;
static int s_ctrl = -1;

static sem_t s_event_sem;
static bool run = true;

static pthread_t gpio_poll;
static pthread_t sound_handler;

static bool run_thread = true;

enum states_t
{
    IDLE = 0,
    PLAYING,
};

static std::string sound_device = "/dev/dsp";
static std::string file_to_play;

static int open_device( WavFile& file )
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

    int vol = 100 | 100 << 8;
    if( ioctl( fd, SNDCTL_DSP_SETPLAYVOL, vol) < 0 ) {
        printf("SNDCTL_DSP_STEREO failed %d\n", fd );
        return -1;
    }

    return fd;
}

static int close_device( int fd )
{
    int vol = 0;
    if( ioctl( fd, SNDCTL_DSP_SETPLAYVOL, vol) < 0 ) {
        printf("SNDCTL_DSP_STEREO failed %d\n", fd );
        return -1;
    }
    return close( fd );
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
            sndfd = open_device( fd );

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
                close_device( sndfd );
                fd.close();
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

static int debounce( int value )
{
    return value;
}

static void* read_gpio(void* dummy)
{
    /* this function blocks */
    struct timespec timeout;
    timeout.tv_nsec = 10000000;
    timeout.tv_sec = 0;

    gpio_handle_t handle = gpio_open( s_ctrl );

    if( handle == GPIO_INVALID_HANDLE ) return NULL;

    while( run_thread )
    {
        nanosleep(&timeout, NULL);
        int value = gpio_pin_get( handle, s_pin );
        if( debounce(value) ) dispatch_bell();
    }

    gpio_close( handle );
    return NULL;
}

int spawn_sound_handler( int gpio, int pin, Json::Value config )
{
    run_thread = true;

    if( sem_init( &s_event_sem, 0, 0 ) < 0 )
    {
        printf("Event Counter init failed\n");
        return -1;
    }

    if( pthread_create( &gpio_poll, NULL, read_gpio, NULL ) )
    {
        printf("failed to create gpio-reader pthread");
        return -2;
    }

    if( pthread_create( &sound_handler, NULL, play_worker, NULL ) )
    {
        printf("failed to create sound pthread");
        return -3;
    }

    s_ctrl = gpio;
    s_pin = pin;

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
    pthread_kill( gpio_poll, SIGINT );

    pthread_join( sound_handler, NULL );
    pthread_join( gpio_poll, NULL );

    sem_destroy( &s_event_sem );
    return 0;
}
