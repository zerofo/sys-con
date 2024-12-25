#pragma once
#include "IUSBInterface.h"
#include <memory>
#include <cstdio>
#include <vector>

#ifndef _PACKED
    #if WIN32
        #define _PACKED
    #else
        #define _PACKED __attribute__((packed))
    #endif
#endif

class IUSBDevice
{
protected:
    std::vector<std::unique_ptr<IUSBInterface>> m_interfaces{};

    uint16_t m_vendorID = 0;
    uint16_t m_productID = 0;

public:
    virtual ~IUSBDevice() = default;

    // Open and close the device.
    virtual ControllerResult Open() = 0;
    virtual void Close() = 0;

    // Reset the device.
    virtual void Reset() = 0;

    // Get the raw reference to interfaces vector.
    virtual std::vector<std::unique_ptr<IUSBInterface>> &GetInterfaces() { return m_interfaces; }

    virtual uint16_t GetVendor() { return m_vendorID; }
    virtual uint16_t GetProduct() { return m_productID; }
};