#include "SwitchUSBLock.h"
#include <mutex>

static std::recursive_mutex usbMutex;

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