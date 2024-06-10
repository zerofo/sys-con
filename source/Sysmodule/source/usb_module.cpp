#include "switch.h"
#include "usb_module.h"
#include "controller_handler.h"
#include "Controllers.h"
#include <stratosphere.hpp>

#include "SwitchUSBDevice.h"
#include "logger.h"
#include <string.h>

#define MS_TO_NS(x) (x * 1000000)

namespace syscon::usb
{
    namespace
    {
        constexpr size_t MaxUsbHsInterfacesSize = 8;

        // MaxUsbEvents is limited by usbHsCreateInterfaceAvailableEvent, we can have only up to 3 events
        constexpr size_t MaxUsbEvents = 3;

        ams::os::Mutex usbMutex(false);

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
            (void)arg;

            do
            {
                s32 total_entries = 0;
                s32 idx_out = 0;

                if (R_SUCCEEDED(waitObjects(&idx_out, g_usbWaiters, g_usbEventCount, UINT64_MAX)))
                {
                    syscon::logger::LogInfo("New USB device detected, checking for controllers ...");

                    std::scoped_lock usbLock(usbMutex);

                    if ((total_entries = QueryAvailableInterfacesByClass(interfaces, sizeof(interfaces), USB_CLASS_HID)) > 0 ||                                     // Generic HID
                        (total_entries = QueryAvailableInterfacesByClassSubClassProtocol(interfaces, sizeof(interfaces), USB_CLASS_VENDOR_SPEC, 0x5D, 0x81)) > 0 || // XBOX360 Wireless
                        (total_entries = QueryAvailableInterfacesByClassSubClassProtocol(interfaces, sizeof(interfaces), USB_CLASS_VENDOR_SPEC, 0x5D, 0x01)) > 0 || // XBOX360 Wired
                        (total_entries = QueryAvailableInterfacesByClassSubClassProtocol(interfaces, sizeof(interfaces), USB_CLASS_VENDOR_SPEC, 0x47, 0xD0)) > 0 || // XBOX ONE
                        (total_entries = QueryAvailableInterfacesByClassSubClassProtocol(interfaces, sizeof(interfaces), 0x58, 0x42, 0x00)) > 0)                    // XBOX Original

                    {
                        if (controllers::IsAtControllerLimit())
                        {
                            syscon::logger::LogError("Reach controller limit (%d) - Can't add anymore controller !", controllers::Get().size());
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

                        ControllerConfig config = {0};
                        ::syscon::config::LoadControllerConfig(&config, interface->device_desc.idVendor, interface->device_desc.idProduct);

                        if (strcmp(config.driver, "dualshock3") == 0)
                        {
                            syscon::logger::LogInfo("Initializing Dualshock 3 controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<Dualshock3Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), config, std::make_unique<syscon::logger::Logger>()));
                        }
                        else if (strcmp(config.driver, "xbox360w") == 0)
                        {
                            syscon::logger::LogInfo("Initializing Xbox 360 Wireless controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<Xbox360Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), config, std::make_unique<syscon::logger::Logger>(), true));
                        }
                        else if (strcmp(config.driver, "xbox360") == 0)
                        {
                            syscon::logger::LogInfo("Initializing Xbox 360 controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<Xbox360Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), config, std::make_unique<syscon::logger::Logger>(), false));
                        }
                        else if (strcmp(config.driver, "xboxone") == 0)
                        {
                            syscon::logger::LogInfo("Initializing Xbox One controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<XboxOneController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), config, std::make_unique<syscon::logger::Logger>()));
                        }
                        else if (strcmp(config.driver, "xbox") == 0)
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
                        syscon::logger::LogError("No HID or XBOX interfaces found for this USB device !");
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

                    std::scoped_lock usbLock(usbMutex);
                    std::scoped_lock controllersLock(controllers::GetScopedLock());

                    eventClear(usbHsGetInterfaceStateChangeEvent());
                    memset(interfaces, 0, sizeof(interfaces));
                    if (R_SUCCEEDED(usbHsQueryAcquiredInterfaces(interfaces, sizeof(interfaces), &total_entries)))
                    {
                        syscon::logger::LogTrace("USBInterface %d interfaces acquired !", total_entries);
                        for (int i = 0; i < total_entries; i++)
                            syscon::logger::LogTrace("USBInterface Acquired: ID=0x%08X", interfaces[i].inf.ID);

                        for (auto it = controllers::Get().begin(); it != controllers::Get().end(); ++it)
                        {
                            bool found_flag = false;

                            for (auto &&ptr : (*it)->GetController()->GetDevice()->GetInterfaces())
                            {
                                syscon::logger::LogTrace("USBInterface Controller interface ID=0x%08X", static_cast<SwitchUSBInterface *>(ptr.get())->GetID());

                                // We check if a device was removed by comparing the controller's interfaces and the currently acquired interfaces
                                // If we didn't find a single matching interface ID, we consider a controller removed
                                for (int i = 0; i < total_entries; i++)
                                {
                                    if (interfaces[i].inf.ID == static_cast<SwitchUSBInterface *>(ptr.get())->GetID())
                                    {
                                        found_flag = true;
                                        break;
                                    }
                                }
                            }

                            if (!found_flag)
                            {
                                syscon::logger::LogInfo("Controller unplugged: %04x-%04x", (*it)->GetController()->GetDevice()->GetVendor(), (*it)->GetController()->GetDevice()->GetProduct());
                                controllers::Get().erase(it--);
                            }
                        }
                    }
                }

            } while (is_usb_interface_change_thread_running);
        }

        s32 QueryAvailableInterfacesByClassSubClassProtocol(UsbHsInterface *interfaces, size_t interfaces_maxsize, u8 iclass, u8 isubclass, u8 iprotocol)
        {
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
        is_usb_event_thread_running = true;
        R_ABORT_UNLESS(threadCreate(&g_usb_event_thread, &UsbEventThreadFunc, nullptr, usb_event_thread_stack, sizeof(usb_event_thread_stack), 0x3A, -2));
        R_ABORT_UNLESS(threadStart(&g_usb_event_thread));

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
            usbHsDestroyInterfaceAvailableEvent(&g_usbEvent[i], i);

        controllers::Reset();
    }

} // namespace syscon::usb