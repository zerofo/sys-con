#include "switch.h"
#include "usb_module.h"
#include "controller_handler.h"
#include "Controllers.h"
#include <stratosphere.hpp>

#include "SwitchUSBDevice.h"
#include "SwitchUSBLock.h"
#include "logger.h"
#include <string.h>

#define MS_TO_NS(x) (x * 1000000ul)

namespace syscon::usb
{
    namespace
    {
        constexpr size_t MaxUsbHsInterfacesSize = 8;

        // MaxUsbEvents is limited by usbHsCreateInterfaceAvailableEvent, we can have only up to 3 events
        constexpr size_t MaxUsbEvents = 3;

        // Thread that waits on generic usb event
        void UsbEventThreadFunc(void *arg);
        // Thread that waits on any disconnected usb devices
        void UsbInterfaceChangeThreadFunc(void *arg);

        alignas(ams::os::ThreadStackAlignment) u8 usb_event_thread_stack[0x2000];
        alignas(ams::os::ThreadStackAlignment) u8 usb_interface_change_thread_stack[0x2000];

        Thread g_usb_event_thread;
        Thread g_usb_interface_change_thread;

        bool is_usb_event_thread_running = false;
        bool is_usb_interface_change_thread_running = false;

        Event g_usbEvent[MaxUsbEvents] = {};
        Waiter g_usbWaiters[MaxUsbEvents] = {};
        size_t g_usbEventCount = 0;

        UsbHsInterface interfaces[MaxUsbHsInterfacesSize] = {};

        s32 QueryAvailableInterfacesByClass(UsbHsInterface *interfaces, size_t interfaces_maxsize, u8 iclass);
        s32 QueryAvailableInterfacesByClassSubClassProtocol(UsbHsInterface *interfaces, size_t interfaces_maxsize, u8 iclass, u8 isubclass, u8 iprotocol);

        Result AddEvent(UsbHsInterfaceFilter *filter, const std::string &name);

        void UsbEventThreadFunc(void *arg)
        {
            u64 timeoutNs = MS_TO_NS(1);
            (void)arg;

            do
            {
                s32 total_entries = 0;

                /*
                    Weird issue with this function, This function will never return in some cases (XBOX Serie for example #14)
                    So, we need to use the timeout=1ms to avoid being stuck in this function the first call, and then we can use the UINT64_MAX
                */
                s32 idx_out = 0;
                Result rc = waitObjects(&idx_out, g_usbWaiters, g_usbEventCount, timeoutNs);
                if (R_SUCCEEDED(rc) || R_VALUE(rc) == KERNELRESULT(TimedOut))
                {
                    SwitchUSBLock usbLock;

                    syscon::logger::LogDebug("New USB device detected (Or polling timeout), checking for controllers ...");

                    if ((total_entries = QueryAvailableInterfacesByClass(interfaces, sizeof(interfaces), USB_CLASS_HID)) > 0 ||                                     // Generic HID
                        (total_entries = QueryAvailableInterfacesByClassSubClassProtocol(interfaces, sizeof(interfaces), USB_CLASS_VENDOR_SPEC, 0x5D, 0x81)) > 0 || // XBOX360 Wireless
                        (total_entries = QueryAvailableInterfacesByClassSubClassProtocol(interfaces, sizeof(interfaces), USB_CLASS_VENDOR_SPEC, 0x5D, 0x01)) > 0 || // XBOX360 Wired
                        (total_entries = QueryAvailableInterfacesByClassSubClassProtocol(interfaces, sizeof(interfaces), USB_CLASS_VENDOR_SPEC, 0x47, 0xD0)) > 0 || // XBOX ONE
                        (total_entries = QueryAvailableInterfacesByClassSubClassProtocol(interfaces, sizeof(interfaces), 0x58, 0x42, 0x00)) > 0)                    // XBOX Original

                    {
                        timeoutNs = MS_TO_NS(1); // Everytime we find a controller we reset the timeout to loop again on next controllers

                        if (controllers::IsAtControllerLimit())
                        {
                            syscon::logger::LogError("Reach controller limit - Can't add anymore controller !");
                            continue;
                        }

                        UsbHsInterface *interface = &interfaces[0];

                        syscon::logger::LogInfo("Trying to initialize USB device: [%04x-%04x] (Class: 0x%02X, SubClass: 0x%02X, Protocol: 0x%02X, bcd: 0x%04X)...",
                                                interface->device_desc.idVendor,
                                                interface->device_desc.idProduct,
                                                interface->device_desc.bDeviceClass,
                                                interface->device_desc.bDeviceSubClass,
                                                interface->device_desc.bDeviceProtocol,
                                                interface->device_desc.bcdDevice);

                        ControllerConfig config;
                        ::syscon::config::LoadControllerConfig(&config, interface->device_desc.idVendor, interface->device_desc.idProduct);

                        if (config.driver == "dualshock3")
                        {
                            syscon::logger::LogInfo("Initializing Dualshock 3 controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<Dualshock3Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), config, std::make_unique<syscon::logger::Logger>()));
                        }
                        else if (config.driver == "xbox360w")
                        {
                            syscon::logger::LogInfo("Initializing Xbox 360 Wireless controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<Xbox360WirelessController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), config, std::make_unique<syscon::logger::Logger>()));
                        }
                        else if (config.driver == "xbox360")
                        {
                            syscon::logger::LogInfo("Initializing Xbox 360 controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<Xbox360Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), config, std::make_unique<syscon::logger::Logger>()));
                        }
                        else if (config.driver == "xboxone")
                        {
                            syscon::logger::LogInfo("Initializing Xbox One controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<XboxOneController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), config, std::make_unique<syscon::logger::Logger>()));
                        }
                        else if (config.driver == "xbox")
                        {
                            syscon::logger::LogInfo("Initializing Xbox 1st gen (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<XboxController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), config, std::make_unique<syscon::logger::Logger>()));
                        }
                        else
                        {
                            syscon::logger::LogInfo("Initializing Generic controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<GenericHIDController>(std::make_unique<SwitchUSBDevice>(interfaces, 1), config, std::make_unique<syscon::logger::Logger>()));
                        }
                    }
                    else
                    {
                        syscon::logger::LogDebug("No HID or XBOX interfaces found !");
                        timeoutNs = UINT64_MAX; // As soon as no controller is found, we wait for the next event
                    }
                }
            } while (is_usb_event_thread_running);
        }

        void UsbInterfaceChangeThreadFunc(void *arg)
        {
            (void)arg;
            do
            {
                if (R_SUCCEEDED(eventWait(usbHsGetInterfaceStateChangeEvent(), UINT64_MAX)))
                {
                    s32 total_entries;
                    syscon::logger::LogInfo("USBInterface state was changed !");

                    SwitchUSBLock usbLock;

                    eventClear(usbHsGetInterfaceStateChangeEvent());
                    memset(interfaces, 0, sizeof(interfaces));
                    if (R_SUCCEEDED(usbHsQueryAcquiredInterfaces(interfaces, sizeof(interfaces), &total_entries)))
                    {
                        std::vector<s32> interfaceIDsPlugged;
                        syscon::logger::LogDebug("USBInterface %d interfaces acquired !", total_entries);
                        for (int i = 0; i < total_entries; i++)
                        {
                            syscon::logger::LogDebug("USBInterface Trying to find ID=0x%08X...", interfaces[i].inf.ID);
                            interfaceIDsPlugged.push_back(interfaces[i].inf.ID);
                        }

                        controllers::RemoveIfNotPlugged(interfaceIDsPlugged);
                    }
                }

            } while (is_usb_interface_change_thread_running);
        }

        s32 QueryAvailableInterfacesByClassSubClassProtocol(UsbHsInterface *interfaces, size_t interfaces_maxsize, u8 iclass, u8 isubclass, u8 iprotocol)
        {
            SwitchUSBLock usbLock;

            UsbHsInterfaceFilter filter{
                .Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bInterfaceSubClass | UsbHsInterfaceFilterFlags_bInterfaceProtocol,
                .bInterfaceClass = iclass,
                .bInterfaceSubClass = isubclass,
                .bInterfaceProtocol = iprotocol,
            };

            s32 out_entries = 0;
            memset(interfaces, 0, interfaces_maxsize);

            usbHsQueryAvailableInterfaces(&filter, interfaces, interfaces_maxsize, &out_entries);

            return out_entries;
        }

        s32 QueryAvailableInterfacesByClass(UsbHsInterface *interfaces, size_t interfaces_maxsize, u8 iclass)
        {
            SwitchUSBLock usbLock;

            UsbHsInterfaceFilter filter{
                .Flags = UsbHsInterfaceFilterFlags_bInterfaceClass,
                .bInterfaceClass = iclass};

            s32 out_entries = 0;
            memset(interfaces, 0, interfaces_maxsize);

            usbHsQueryAvailableInterfaces(&filter, interfaces, interfaces_maxsize, &out_entries);

            return out_entries;
        }

        inline Result AddEvent(UsbHsInterfaceFilter *filter, const std::string &name)
        {
            SwitchUSBLock usbLock;

            if (g_usbEventCount >= MaxUsbEvents)
            {
                static bool isMaxEventLogged = false;
                if (!isMaxEventLogged)
                {
                    syscon::logger::LogError("Unable to add future events ! (Max USB events reached !)");
                    isMaxEventLogged = true;
                }
                return CONTROL_ERR_OUT_OF_MEMORY;
            }

            syscon::logger::LogDebug("Adding event with filter: %s (%d/%d)...", name.c_str(), g_usbEventCount + 1, MaxUsbEvents);
            Result ret = usbHsCreateInterfaceAvailableEvent(&g_usbEvent[g_usbEventCount], true, g_usbEventCount, filter);
            g_usbWaiters[g_usbEventCount] = waiterForEvent(&g_usbEvent[g_usbEventCount]);

            g_usbEventCount++;
            return ret;
        }

    } // namespace

    void Initialize(syscon::config::DiscoveryMode discovery_mode, std::vector<syscon::config::ControllerVidPid> &discovery_vidpid)
    {
        syscon::logger::LogInfo("Discovery mode: %d", discovery_mode);

        if (discovery_mode == syscon::config::DiscoveryMode::HID_AND_XBOX || discovery_mode == syscon::config::DiscoveryMode::VIDPID_AND_XBOX)
        {
            // Filter use to detect XBOX controllers
            UsbHsInterfaceFilter filterAllDevices1{
                .Flags = UsbHsInterfaceFilterFlags_bcdDevice_Min,
                .bcdDevice_Min = 0x0000,
            };
            AddEvent(&filterAllDevices1, "XBOX");
        }

        if (discovery_mode == syscon::config::DiscoveryMode::HID_AND_XBOX)
        {
            // Filter used to detect Generic HID controllers
            // Cause issue with Native Switch Controllers
            UsbHsInterfaceFilter filterAllDevices2{
                .Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bcdDevice_Min,
                .bcdDevice_Min = 0x0000,
                .bInterfaceClass = USB_CLASS_HID,
            };
            AddEvent(&filterAllDevices2, "USB_CLASS_HID");
        }

        if (discovery_mode == syscon::config::DiscoveryMode::VIDPID || discovery_mode == syscon::config::DiscoveryMode::VIDPID_AND_XBOX)
        {
            // Filter known VID/PID
            for (syscon::config::ControllerVidPid &vidpid : discovery_vidpid)
            {
                UsbHsInterfaceFilter filterKnownDevice{
                    .Flags = UsbHsInterfaceFilterFlags_idVendor,
                    .idVendor = vidpid.vid,
                };

                if (vidpid.pid != 0)
                {
                    filterKnownDevice.Flags |= UsbHsInterfaceFilterFlags_idProduct;
                    filterKnownDevice.idProduct = vidpid.pid;
                }

                AddEvent(&filterKnownDevice, std::string(vidpid));
            }
        }

        is_usb_event_thread_running = true;
        R_ABORT_UNLESS(threadCreate(&g_usb_event_thread, &UsbEventThreadFunc, nullptr, usb_event_thread_stack, sizeof(usb_event_thread_stack), 0x3A, -2));
        R_ABORT_UNLESS(threadStart(&g_usb_event_thread));

        is_usb_interface_change_thread_running = true;
        R_ABORT_UNLESS(threadCreate(&g_usb_interface_change_thread, &UsbInterfaceChangeThreadFunc, nullptr, usb_interface_change_thread_stack, sizeof(usb_interface_change_thread_stack), 0x2C, -2));
        R_ABORT_UNLESS(threadStart(&g_usb_interface_change_thread));
    }

    void Exit()
    {
        is_usb_event_thread_running = false;
        is_usb_interface_change_thread_running = false;

        svcCancelSynchronization(g_usb_event_thread.handle);
        threadWaitForExit(&g_usb_event_thread);
        threadClose(&g_usb_event_thread);

        svcCancelSynchronization(g_usb_interface_change_thread.handle);
        threadWaitForExit(&g_usb_interface_change_thread);
        threadClose(&g_usb_interface_change_thread);

        for (size_t i = 0; i < g_usbEventCount; i++)
        {
            SwitchUSBLock usbLock;
            usbHsDestroyInterfaceAvailableEvent(&g_usbEvent[i], i);
        }

        controllers::Reset();
    }

} // namespace syscon::usb