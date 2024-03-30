#include "switch.h"
#include "usb_module.h"
#include "controller_handler.h"
#include "config_handler.h"
#include "Controllers.h"
#include <stratosphere.hpp>

#include "SwitchUSBDevice.h"
#include "logger.h"
#include <string.h>

namespace syscon::usb
{
    namespace
    {
        constexpr size_t MaxUsbHsInterfacesSize = 8;
        constexpr size_t MaxUsbEvents = 8;

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

        struct UsbEvent
        {
            Event m_usbEvent;
            uint16_t m_usbEventVendorId;
            int m_usbEventIndex;
        };

        UsbEvent g_usbEvent[MaxUsbEvents] = {};
        Waiter g_usbWaiters[MaxUsbEvents] = {};
        int g_usbEventCount = 0;
        int g_usbEventIndex = 0;

        UsbHsInterface interfaces[MaxUsbHsInterfacesSize] = {};

        s32 QueryInterfaces(UsbHsInterface *interfaces, size_t interfaces_maxsize, u8 iclass, u8 isubclass, u8 iprotocol);
        // s32 QueryAllHIDInterfaces(UsbHsInterface *interfaces, size_t interfaces_maxsize);
        Result AddAvailableEventVendorId(uint16_t aVendorID);

        void UsbEventThreadFunc(void *arg)
        {
            (void)arg;
            do
            {
                s32 idx_out = 0;

                if (R_SUCCEEDED(waitObjects(&idx_out, g_usbWaiters, g_usbEventCount, UINT64_MAX)))
                {
                    syscon::logger::LogInfo("New USB device detected, checking for controllers ...");

                    std::scoped_lock usbLock(usbMutex);
                    if (!controllers::IsAtControllerLimit())
                    {
                        s32 total_entries = 0;

                        if ((total_entries = QueryInterfaces(interfaces, sizeof(interfaces), USB_CLASS_VENDOR_SPEC, 0x5D, 0x01)) != 0) //0x045E    0x028E
                        {
                            syscon::logger::LogInfo("Initializing Xbox 360 controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<Xbox360Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), std::make_unique<syscon::logger::Logger>()));
                        }
                        else if ((total_entries = QueryInterfaces(interfaces, sizeof(interfaces), USB_CLASS_VENDOR_SPEC, 0x5D, 0x81)) != 0)
                        {
                            syscon::logger::LogInfo("Initializing Xbox 360 wireless controller (Interface count: %d)...", total_entries);

                            for (int i = 0; i < total_entries; i++)
                                controllers::Insert(std::make_unique<Xbox360WirelessController>(std::make_unique<SwitchUSBDevice>(interfaces + i, 1), std::make_unique<syscon::logger::Logger>()));
                        }
                        else if ((total_entries = QueryInterfaces(interfaces, sizeof(interfaces), 0x58, 0x42, 0x00)) != 0)
                        {
                            syscon::logger::LogInfo("Initializing Xbox Original controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<XboxController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), std::make_unique<syscon::logger::Logger>()));
                        }
                        else if ((total_entries = QueryInterfaces(interfaces, sizeof(interfaces), USB_CLASS_VENDOR_SPEC, 0x47, 0xD0)) != 0)
                        {
                            syscon::logger::LogInfo("Initializing Xbox One controller (Interface count: %d) ...", total_entries);
                            controllers::Insert(std::make_unique<XboxOneController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), std::make_unique<syscon::logger::Logger>()));
                        }
                        else if ((total_entries = QueryInterfaces(interfaces, sizeof(interfaces), USB_CLASS_HID, 0, 0)) != 0)
                        {

                            if (interfaces[0].device_desc.idVendor == VENDOR_SONY && interfaces[0].device_desc.idProduct == PRODUCT_DUALSHOCK3)
                            {
                                syscon::logger::LogInfo("Initializing Dualshock 3 controller (Interface count: %d) ...", total_entries);
                                controllers::Insert(std::make_unique<Dualshock3Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), std::make_unique<syscon::logger::Logger>()));
                            }
                            else if (interfaces[0].device_desc.idVendor == VENDOR_SONY && (interfaces[0].device_desc.idProduct == PRODUCT_DUALSHOCK4_1X || interfaces[0].device_desc.idProduct == PRODUCT_DUALSHOCK4_2X))
                            {
                                syscon::logger::LogInfo("Initializing Dualshock 4 controller (Interface count: %d) ...", total_entries);
                                controllers::Insert(std::make_unique<Dualshock4Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), std::make_unique<syscon::logger::Logger>()));
                            }
                            else
                            {
                                syscon::logger::LogInfo("Initializing Generic controller (Interface count: %d) ...", total_entries);
                                controllers::Insert(std::make_unique<GenericHIDController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries), std::make_unique<syscon::logger::Logger>()));
                            }
                        }
                        else
                        {
                            syscon::logger::LogDebug("USB device detected is not a controller !");
                        }
                    }
                    else
                    {
                        syscon::logger::LogWarning("Reach controller limit (%d) - Can't add anymore controller !", controllers::Get().size());
                    }

                    /*s32 interfaceHIDCount = QueryAllHIDInterfaces(interfaces, sizeof(interfaces));
                    if (interfaceHIDCount > 0)
                    {
                        for (int i = 0; i < interfaceHIDCount; i++)
                        {
                            // if the interfacesHID[i].device_desc.idVendor is already in the list of event we skip it otherwise we add it
                            bool shouldAddHid = true;
                            for (int j = 0; j < g_usbEventCount; j++)
                            {
                                if (interfaces[i].device_desc.idVendor == g_usbEvent[j].m_usbEventVendorId)
                                {
                                    shouldAddHid = false;
                                    break;
                                }
                            }

                            if (shouldAddHid)
                                AddAvailableEventVendorId(interfaces[i].device_desc.idVendor);
                        }
                    }*/
                }

            } while (is_usb_event_thread_running);
        }

        void UsbInterfaceChangeThreadFunc(void *arg)
        {
            (void)arg;
            do
            {
                if (R_SUCCEEDED(eventWait(usbHsGetInterfaceStateChangeEvent(), UINT64_MAX))) // 1 second timeout
                {
                    s32 total_entries;
                    syscon::logger::LogInfo("USB Interface state was changed");

                    std::scoped_lock usbLock(usbMutex);
                    std::scoped_lock controllersLock(controllers::GetScopedLock());

                    eventClear(usbHsGetInterfaceStateChangeEvent());
                    memset(interfaces, 0, sizeof(interfaces));
                    if (R_SUCCEEDED(usbHsQueryAcquiredInterfaces(interfaces, sizeof(interfaces), &total_entries)))
                    {
                        for (auto it = controllers::Get().begin(); it != controllers::Get().end(); ++it)
                        {
                            bool found_flag = false;

                            for (auto &&ptr : (*it)->GetController()->GetDevice()->GetInterfaces())
                            {
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
                                syscon::logger::LogInfo("Controller unplugged: %04X-%04X", (*it)->GetController()->GetDevice()->GetVendor(), (*it)->GetController()->GetDevice()->GetProduct());
                                controllers::Get().erase(it--);
                            }
                        }
                    }
                }

            } while (is_usb_interface_change_thread_running);
        }

        s32 QueryInterfaces(UsbHsInterface *interfaces, size_t interfaces_maxsize, u8 iclass, u8 isubclass, u8 iprotocol)
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

        /*s32 QueryAllHIDInterfaces(UsbHsInterface *interfaces, size_t interfaces_maxsize)
        {
            UsbHsInterfaceFilter filter{
                .Flags = UsbHsInterfaceFilterFlags_bInterfaceClass,
                .bInterfaceClass = USB_CLASS_HID};

            s32 out_entries = 0;
            memset(interfaces, 0, interfaces_maxsize);

            usbHsQueryAllInterfaces(&filter, interfaces, interfaces_maxsize, &out_entries);

            syscon::logger::LogDebug("QueryAllHIDInterfaces: %d HID interfaces found", out_entries);

            return out_entries;
        }*/

        inline Result AddAvailableEventVendorId(uint16_t vendorID) // 0x0000 means all devices
        {
            UsbHsInterfaceFilter filter;

            if (vendorID == 0)
            {
                filter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bcdDevice_Min;
                filter.bcdDevice_Min = 0x0000;
                filter.bInterfaceClass = USB_CLASS_HID;
            }
            else
            {
                filter.Flags = UsbHsInterfaceFilterFlags_idVendor;
                filter.idVendor = vendorID;
            }

            syscon::logger::LogDebug("Adding HID device vendor ID to event list: %04X", vendorID);
            Result ret = usbHsCreateInterfaceAvailableEvent(&g_usbEvent[g_usbEventCount].m_usbEvent, true, g_usbEventIndex, &filter);
            g_usbWaiters[g_usbEventCount] = waiterForEvent(&g_usbEvent[g_usbEventCount].m_usbEvent);
            g_usbEvent[g_usbEventCount].m_usbEventVendorId = vendorID;
            g_usbEvent[g_usbEventCount].m_usbEventIndex = g_usbEventIndex;

            g_usbEventIndex++;
            g_usbEventCount++;
            return ret;
        }

    } // namespace

    void Initialize()
    {
        is_usb_event_thread_running = true;
        R_ABORT_UNLESS(threadCreate(&g_usb_event_thread, &UsbEventThreadFunc, nullptr, usb_event_thread_stack, sizeof(usb_event_thread_stack), 0x3A, -2));
        R_ABORT_UNLESS(threadStart(&g_usb_event_thread));

        AddAvailableEventVendorId(0);

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

        for (int i = 0; i < g_usbEventCount; i++)
            usbHsDestroyInterfaceAvailableEvent(&g_usbEvent[i].m_usbEvent, g_usbEvent[i].m_usbEventIndex);

        controllers::Reset();
    }

} // namespace syscon::usb