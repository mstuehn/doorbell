#include "doorbell.h"

#include <iostream>
#include <unistd.h>

#include <sys/soundcard.h>
#include <fcntl.h>

#include "sounddev.h"
#include "utils.h"

bool DoorBell::ring()
{
    return sem_post( &m_EvenNotifier ) != -1;
}

DoorBell::DoorBell( Json::Value& config ) //: m_PlayWorker( &DoorBell::play_worker, this )
{
    m_KeepRunning = true;
    if( sem_init( &m_EvenNotifier, 0, 0 ) == -1 ) {
        perror( "Not able to initialize semaphore" );
        exit(1);
    }
    m_PlayWorker = std::thread( &DoorBell::play_worker, this );
    pthread_set_name_np( m_PlayWorker.native_handle(), "DoorBeller" );
    m_FileToPlay = config["file_to_play"].asString();
    m_SoundDevice = config["device"].asString();
}

DoorBell::~DoorBell()
{
    m_KeepRunning = false;

    // Release play_worker
    sem_post( &m_EvenNotifier );
    m_PlayWorker.join();

    sem_destroy( &m_EvenNotifier );
}

bool DoorBell::play_worker()
{
    WavFile fd;
    print_guard();

    while(m_KeepRunning)
    {
        std::cout << "Waiting for events" << std::endl;
        int result = sem_wait(&m_EvenNotifier);
        if( result != 0 || !m_KeepRunning ) return false;

        if( fd.open( m_FileToPlay ) != -1 )
        {
            SoundDevice sndfd( m_SoundDevice, fd );
            if( !sndfd.open() ) {
                    perror("Error during open");
                    return false;
            } else std::cout << "Device open" << std::endl;
            size_t bufsz = 256;
            uint8_t buf[bufsz];
	    printf("%p - %p\n", buf, buf+256 );
            while(m_KeepRunning)
            {
                if( sem_trywait( &m_EvenNotifier ) == 0 ) fd.reset();

                int len = fd.read( buf, bufsz );

                if( len < 0 ) {
                    perror( "Error during read" );
                    return false;
                }

                if( len == 0 ) break;

                size_t n = sndfd.write( buf, len );
                if( n==0 ) {
                    perror("Error during write");
                    return false;
                }
            }
        }
    }

    return true;
}

