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
    : m_DataSize(256)
{
    m_KeepRunning = true;
    if( sem_init( &m_EvenNotifier, 0, 0 ) == -1 ) {
        perror( "Not able to initialize semaphore" );
        exit(1);
    }
    m_PlayWorker = std::thread( &DoorBell::play_worker, this );
    m_FileToPlay = config["file_to_play"].asString();
    m_SoundDevice = config["device"].asString();
    m_MixerDevice = config["mixer"].asString();
    m_DataBuf = new uint8_t[m_DataSize];
}

DoorBell::~DoorBell()
{
    m_KeepRunning = false;

    // Release play_worker
    sem_post( &m_EvenNotifier );
    m_PlayWorker.join();

    sem_destroy( &m_EvenNotifier );
    delete [] m_DataBuf;
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
            SoundDevice sndfd( m_SoundDevice, m_MixerDevice, fd );
            if( !sndfd.open() ) {
                    perror("Error during open");
                    return false;
            }
            if( !sndfd.volume(250, 250) ) {
                    perror("Error during setting volume");
                    return false;
            }
            size_t n;
            while(m_KeepRunning)
            {
                if( sem_trywait( &m_EvenNotifier ) == 0 ) fd.reset();

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

