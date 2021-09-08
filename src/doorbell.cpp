#include "doorbell.h"

#include <iostream>

#include <sys/soundcard.h>
#include <fcntl.h>

#include "sounddev.h"

bool DoorBell::ring()
{
    return sem_post( &m_EventNotifier ) != -1;
}

DoorBell::DoorBell( const Json::Value& config ) //: m_PlayWorker( &DoorBell::play_worker, this )
    : m_DataSize(256)
{
    m_KeepRunning = true;
    if( sem_init( &m_EventNotifier, 0, 0 ) == -1 ) {
        perror( "Not able to initialize semaphore" );
        exit(1);
    }
    m_PlayWorker = std::thread( &DoorBell::play_worker, this );
    m_FileToPlay = config.get("file_to_play", "data/DoorBell.wav").asString();
    m_SoundDevice = config.get("device", "/dev/dsp").asString();
    m_DataBuf = new uint8_t[m_DataSize];
}

DoorBell::~DoorBell()
{
    m_KeepRunning = false;

    // Release play_worker
    sem_post( &m_EventNotifier );
    m_PlayWorker.join();

    sem_destroy( &m_EventNotifier );
    delete [] m_DataBuf;
}

bool DoorBell::play_worker()
{
    WavFile fd;

    while(m_KeepRunning)
    {
        int result = sem_wait(&m_EventNotifier);
        if( result != 0 || !m_KeepRunning ) return false;

        if( fd.open( m_FileToPlay ) != -1 )
        {
            SoundDevice sndfd( m_SoundDevice, fd );
            if( !sndfd.open() ) {
                    perror("Error during open");
                    return false;
            }
            if( !sndfd.volume(80, 80) ) {
                    perror("Error during setting volume");
                    return false;
            }
            size_t n;
            while(m_KeepRunning)
            {
                if( sem_trywait( &m_EventNotifier ) == 0 ) fd.reset();

                int len = fd.read( m_DataBuf, m_DataSize );

                if( len < 0 ) {
                    perror( "Error during read" );
                    return false;
                }

                if( len == 0 ) break;

                n = sndfd.write( m_DataBuf, len );
                if( n==0 ) {
                    perror("Error during write");
                    return false;
                }
            }
            sndfd.volume(0, 0);
        }
    }

    return true;
}

