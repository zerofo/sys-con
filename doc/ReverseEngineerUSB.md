# Introduction
This document aims to reverse-engineer the USB discovery process on the Switch, in order to better understand why certain controllers are not detected by the system.

# Investigations

## Windows discovery sequence
Below is the ordered list of steps involved in the Windows USB discovery process.
```
8006000100004000 request configuration GET_DESCRIPTOR (Device Descriptor)
0005160000000000 SET_ADDRESS 16
8006000100001200 request configuration GET_DESCRIPTOR (Device Descriptor)
800600020000FF00
800600030000FF00
800602030904FF00
8006000100001200 request configuration GET_DESCRIPTOR (Device Descriptor)
800600030000FF00
800601030904FF00
800602030904FF00

----- Wireshark start here

8006000100001200 request configuration GET_DESCRIPTOR (Device Descriptor)
8006000200000900 request configuration GET_DESCRIPTOR (usbDescriptorConfiguration - maxpower)
8006000200002200 request configuration GET_DESCRIPTOR (usbDescriptorConfiguration - maxpower, class, endpoint etc...)
0009010000000000 SET_CONFIGURATION
210A000000000000 SET_IDLE
8106002200006A00 GET_DESCRIPTOR Request HID report
8006020309040204
8006020309040601
800600030000FF00
800600030000FF00
800601030904FF00
800601030904FF00
800602030904FF00
800602030904FF00
800601030904FE00
800602030904FE00
800601030904FE00
800602030904FE00
```

## Switch USB discovery sequence

Below is the ordered list of steps involved in the Switch USB discovery process.

```
8006000100001200 request configuration GET_DESCRIPTOR (Device Descriptor)
0005010000000000 SET_ADDRESS 01
8006000200000900 request configuration GET_DESCRIPTOR (usbDescriptorConfiguration - maxpower)
8006000200002200 request configuration GET_DESCRIPTOR (usbDescriptorConfiguration - maxpower, class, endpoint etc...)
8006000300008200 request configuration GET_DESCRIPTOR / Index=00 (Idx 0) STRING=03 / 8200=buffer 130 bytes (String descriptor) - No issue if we don't reply to that
0009010000000000 SET_CONFIGURATION
------ sys-con take control here
210A000000000000 SET_IDLE
8106002200000002 GET_DESCRIPTOR Request HID report
```

## How to decode USB packet

End of the file: https://github.com/obdev/v-usb/blob/master/usbdrv/usbdrv.h

https://www.beyondlogic.org/usbnutshell/usb5.shtml

https://www.beyondlogic.org/usbnutshell/usb6.shtml

# Understand the different

We aimed to verify the following scenarios:

1. **Controller not responding to requests**
We tested whether the switch discovery process still functions when the controller does not respond to a specific request: GET_DESCRIPTOR with Index=00 (Idx 0) and STRING=03.
The switch perfectly works even if we do not reply.

2. **Controller response timing**
We compared the performance of a slow controller to a non-working one. The results showed that the switch works even with the slowest controller. Therefore, it seems that latency is not the issue.

# Conclusion
As of the time of writing, we have not yet identified the reason why some controllers fail to function properly.
Is it a power issue ?
