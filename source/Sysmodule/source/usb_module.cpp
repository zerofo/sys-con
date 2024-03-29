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
        constexpr u8 CatchAllEventIndex = 0;

        constexpr size_t MaxUsbHsInterfacesSize = 8;
        constexpr size_t MaxUsbHsInterfacesVendorId = 8;

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

        Event g_usbCatchAllEvent[MaxUsbHsInterfacesVendorId] = {};
        uint16_t g_usbCatchAllEventVendorId[MaxUsbHsInterfacesVendorId] = {};
        int g_usbCatchAllEventCount = 0;

        UsbHsInterface interfaces[MaxUsbHsInterfacesSize] = {};

        s32 QueryInterfaces(UsbHsInterface *interfaces, size_t interfaces_maxsize, u8 iclass, u8 isubclass, u8 iprotocol);
        // s32 QueryVendorProduct(uint16_t vendor_id, uint16_t product_id);
        s32 QueryAllHIDInterfaces(UsbHsInterface *interfaces, size_t interfaces_maxsize);
        Result AddAvailableEventVendorId(uint16_t aVendorID);

        void UsbEventThreadFunc(void *arg)
        {
            (void)arg;
            do
            {
                for (int i = 0; i < g_usbCatchAllEventCount; i++)
                {
                    if (R_SUCCEEDED(eventWait(&g_usbCatchAllEvent[i], 1 * 1000 * 1000 * 1000)))
                    {
                        syscon::logger::LogInfo("New USB device detected, checking for controllers ...");

                        std::scoped_lock usbLock(usbMutex);
                        if (!controllers::IsAtControllerLimit())
                        {
                            s32 total_entries = 0;

                            if ((total_entries = QueryInterfaces(interfaces, sizeof(interfaces), USB_CLASS_VENDOR_SPEC, 0x5D, 0x01)) != 0)
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
                        }
                        else
                        {
                            syscon::logger::LogDebug("USB device detected is not a controller !");
                        }
                    }
                }

                if (g_usbCatchAllEventCount == 0)
                    svcSleepThread(1 * 1000 * 1000 * 1000);

                s32 interfaceHIDCount = QueryAllHIDInterfaces(interfaces, sizeof(interfaces));
                if (interfaceHIDCount > 0)
                {
                    for (int i = 0; i < interfaceHIDCount; i++)
                    {
                        // if the interfacesHID[i].device_desc.idVendor is already in the list of event we skip it otherwise we add it
                        bool shouldAddHid = true;
                        for (int j = 0; j < g_usbCatchAllEventCount; j++)
                        {
                            if (interfaces[i].device_desc.idVendor == g_usbCatchAllEventVendorId[j])
                            {
                                shouldAddHid = false;
                                break;
                            }
                        }

                        if (shouldAddHid)
                        {
                            AddAvailableEventVendorId(interfaces[i].device_desc.idVendor);
                        }
                    }
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
            // usbHsQueryAllInterfaces(&filter, interfaces, sizeof(interfaces), &out_entries);
            return out_entries;
        }

        s32 QueryAllHIDInterfaces(UsbHsInterface *interfaces, size_t interfaces_maxsize)
        {
            UsbHsInterfaceFilter filter{
                .Flags = UsbHsInterfaceFilterFlags_bInterfaceClass,
                .bInterfaceClass = USB_CLASS_HID};

            s32 out_entries = 0;
            memset(interfaces, 0, interfaces_maxsize);

            usbHsQueryAllInterfaces(&filter, interfaces, interfaces_maxsize, &out_entries);

            syscon::logger::LogDebug("QueryAllHIDInterfaces: %d", out_entries);

            return out_entries;
        }

        inline Result AddAvailableEventVendorId(uint16_t vendorID)
        {
            UsbHsInterfaceFilter filter{
                .Flags = UsbHsInterfaceFilterFlags_idVendor};
            filter.idVendor = vendorID;

            syscon::logger::LogInfo("Adding HID device vendor ID to event list: %04X", vendorID);
            Result ret = usbHsCreateInterfaceAvailableEvent(&g_usbCatchAllEvent[g_usbCatchAllEventCount], true, CatchAllEventIndex, &filter);
            g_usbCatchAllEventVendorId[g_usbCatchAllEventCount] = vendorID;
            g_usbCatchAllEventCount++;
            return ret;
        }

        inline Result AddFirstEvent()
        {
            UsbHsInterfaceFilter filter{
                .Flags = UsbHsInterfaceFilterFlags_bcdDevice_Min,
                .bcdDevice_Min = 0x0000};

            syscon::logger::LogInfo("Adding event...");
            Result ret = usbHsCreateInterfaceAvailableEvent(&g_usbCatchAllEvent[g_usbCatchAllEventCount], true, CatchAllEventIndex, &filter);
            g_usbCatchAllEventVendorId[g_usbCatchAllEventCount] = 0;
            g_usbCatchAllEventCount++;
            return ret;
        }
    } // namespace

    void Initialize()
    {
        R_ABORT_UNLESS(Enable());
    }

    void Exit()
    {
        Disable();
    }

    ams::Result Enable()
    {
        // R_TRY(CreateUsbEvents());
        is_usb_event_thread_running = true;
        R_ABORT_UNLESS(threadCreate(&g_usb_event_thread, &UsbEventThreadFunc, nullptr, usb_event_thread_stack, sizeof(usb_event_thread_stack), 0x3A, -2));
        R_ABORT_UNLESS(threadStart(&g_usb_event_thread));
        AddFirstEvent();

        is_usb_interface_change_thread_running = true;
        R_ABORT_UNLESS(threadCreate(&g_usb_interface_change_thread, &UsbInterfaceChangeThreadFunc, nullptr, usb_interface_change_thread_stack, sizeof(usb_interface_change_thread_stack), 0x2C, -2));
        R_ABORT_UNLESS(threadStart(&g_usb_interface_change_thread));

        return 0;
    }

    void Disable()
    {
        is_usb_event_thread_running = false;
        is_usb_interface_change_thread_running = false;

        svcCancelSynchronization(g_usb_event_thread.handle);
        threadWaitForExit(&g_usb_event_thread);
        threadClose(&g_usb_event_thread);

        svcCancelSynchronization(g_usb_interface_change_thread.handle);
        threadWaitForExit(&g_usb_interface_change_thread);
        threadClose(&g_usb_interface_change_thread);

        DestroyUsbEvents();
        controllers::Reset();
    }

    void DestroyUsbEvents()
    {
        usbHsDestroyInterfaceAvailableEvent(&g_usbCatchAllEvent[0], CatchAllEventIndex);
    }
} // namespace syscon::usb