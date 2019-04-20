
#include "inpin.h"
#include <libgpio.h>

InPin::InPin( int pin, int poll_interval ) : m_Pin(pin)
{
    m_PollTime.tv_sec = poll_interval / 1000000;
    m_PollTime.tv_nsec = poll_interval % 1000000;
}

InPin::InPin( int ctrl, int pin, int poll_interval ) : InPin( pin, poll_interval )
{
    m_GpioCtrl = gpio_open(ctrl);
    if( handle == GPIO_INVALID_HANDLE) err(1, "gpio_open failed");
}

InPin::InPin( char* device, int pin, int poll_interval ) : InPin( pin, poll_interval )
{
    m_GpioCtrl = gpio_open_device(device);
    if( handle == GPIO_INVALID_HANDLE) err(1, "gpio_open failed");

}
