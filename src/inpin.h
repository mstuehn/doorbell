#include <cstdint>

#include <time.h>

class InPin{
    public:
        InPin( int pin, int poll_interval );
        InPin( int ctrl, int pin, int poll_interval );
        InPin( char* device, int pin, int poll_interval );

        int poll( int timeout_us );

    private:

        int m_Pin;
        struct timespec m_PollTime;
        gpio_handle_t m_GpioCtrl;
};
