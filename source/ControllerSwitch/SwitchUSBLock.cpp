#include "SwitchUSBLock.h"
#include <stratosphere.hpp>

static ams::os::Mutex usbMutex(true);

SwitchUSBLock::SwitchUSBLock(bool scoped) : m_scoped(scoped)
{
    if (m_scoped)
        usbMutex.lock();
}

SwitchUSBLock::~SwitchUSBLock()
{
    if (m_scoped)
        usbMutex.unlock();
}

void SwitchUSBLock::lock()
{
    usbMutex.lock();
}

void SwitchUSBLock::unlock()
{
    usbMutex.unlock();
}