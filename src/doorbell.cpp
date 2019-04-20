#include "doorbell.h"

#include <iostream>

#include <sys/soundcard.h>
#include <fcntl.h>

#include "sounddev.h"

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

    while(m_KeepRunning)
    {
        int result = sem_wait(&m_EvenNotifier);
        if( result != 0 || !m_KeepRunning ) return false;

        if( fd.open( m_FileToPlay ) != -1 )
        {
            SoundDevice sndfd( m_SoundDevice, fd );
            if( !sndfd.open() ) {
                    perror("Error during open");
                    return false;
            }
            size_t n, bufsz = 256;
            uint8_t buf[bufsz];
            while(1)
            {
                if( sem_trywait( &m_EvenNotifier ) == 0 ) fd.reset();

                int len = fd.read( buf, bufsz );

                if( len < 0 ) {
                    perror( "Error during read" );
                    return false;
                }

                if( len == 0 ) break;

                n = sndfd.write( buf, len );
                if( n==0 ) {
                    perror("Error during write");
                    return false;
                }
            }
        }
    }

    return true;
}

