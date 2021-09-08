
#include <string>
#include <functional>
#include <list>
#include <thread>
#include <atomic>

#include <json/json.h>


#include <signal.h>
#include <semaphore.h>
#include <time.h>

#include "wavfile.h"

class DoorBell {

    public:
        bool ring();

        DoorBell( const Json::Value& config );
        virtual ~DoorBell();

    private:
        DoorBell() = delete;
        bool play_worker();

        uint8_t* m_DataBuf;
        const size_t m_DataSize;

        std::string m_FileToPlay;
        std::string m_SoundDevice;
        std::string m_MixerDevice;

        std::thread m_PlayWorker;
        sem_t m_EventNotifier;

        std::atomic<bool> m_KeepRunning;
};
