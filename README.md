# sys-con

#### A Nintendo Switch custom sysmodule for third-party controller support. No man-in-the-middle required, No specific hardware required !

###### \[Switch FW 7.0.0+\] [Atmosphère only]
Tested on Switch FW 17.0.0

## Description
This sysmodule aims to provide complete functionality for any joystick or gamepad not supported natively by Nintendo Switch.
Only USB connection is supported - For bluetooth connection you can use ndeadly's [MissionControl](https://github.com/ndeadly/MissionControl)

## Why this fork ?
This fork add a generic support to ANY HID game controllers. Meaning, any USB controller PC compatible should work with your Nintendo Switch.
While the original library only support for Xbox/PS controllers, this one add support to all others PC controller without any limit.

## Install
Grab the latest zip from the [releases page](https://github.com/o0zz/sys-con/releases). Extract it in your SD card and boot/reboot your switch.

## Config
sys-con comes with a config folder located at `/config/sys-con/`. It contains options for adjusting stick/trigger deadzone, as well as remapping inputs. 

## Logs
In case of issue, you can look at the logs in `/config/sys-con/logs.txt`
For more verbose logs, edit config.ini and set :

```
;log_level Trace=0, Debug=1, Info=2, Warning=3, Error=4
log_level=0
```
## Feature availables
- [x] Key mapping using VID/PID or profile
- [x] Configurable deadzone
- [x] Configurable polling frequency
- [x] Configurable controller color using #RGBA
- [x] HID joystick/gamepad supported (PC Controller compatible)
- [ ] Rumble (Implemented but not supported by libnx for now)

## Supported controller
- [x] Any Standard HID Controller (PC Compatible without driver)
- [x] Dualshock 4
- [x] Dualshock 3
- [x] Xbox 360 Controller
- [x] Xbox 360 Wireless adapter
- [x] Xbox One X/S Controller

## Tested with
- [x] Xinmotek XM-10 (Arcade controller)
- [x] PSX Adapter
- [x] Dualshock 4
- [x] Xbox 360 Controller
- [x] Xbox 360 Wireless adapter

## Not Tested against
- [ ] Dualshock 3
- [ ] Xbox One X/S Controller
- [ ] Many HID controller...

## How to add a new controller ?
Most of the time you will only have to do the mapping, how to do it ?

### Method 1 (From a windows PC)

Connect your controller to your PC

1. Go to "Control Panel" > "Device Manager" and find your USB device under "Human Interface Devices".
2. Double click the device or right click and select Properties.
3. Go to the "Details" tab and select "Hardware IDs" to view its PID and VID. The PID/VID should look like HID\VID_0810&PID_0001&...
From there you need to extract:
`[0810-0001]`
4- Open "joy.cpl" (Either from Win+R or directly in Start Menu)
6- Select your controller and click "Properties"
5- Here you should see a panel with button ID (1, 2, 3 ...)
6- Press the button and take a note which button is on which ID.

Now Edit /config/sys-con/config.ini on your switch sdcard and add:
```
[0810-0001]
B=3
A=2
Y=4
X=1
L=7
R=8
ZL=5
ZR=6
minus=9
plus=10
```

Where 1, 2, 3, 4, ... represent the button ID noted in step 6.

### Method 2 (Directly from the switch logs)

1- Connect your controller to your switch and unplug it
2- Open /config/sys-con/logs.txt and look for a line like: 
`Trying to find configuration for USB device: [0810-0001]`
3- Now Edit /config/sys-con/config.ini on your switch sdcard and add:
```
[0810-0001]
B=1
A=2
Y=3
X=4
L=5
R=6
ZL=7
ZR=8
minus=9
plus=10
``` 
Where 1, 2, 3, 4, ... are randomly set.

4- Now Re-plug your controller 
5- On your switch go to: Settings -> Controllers & Sensors > Test Input Devices
6- Try to press button you will quickly understand that the mapping is incorrect, you need now to remap it correctly by changing the /config/sys-con/config.ini by iterations

## Building (For developers)

Don't download the project as ZIP as it will not copy submodules properly, prefer a git clone:
`git clone --recurse-submodules -j8 https://github.com/o0Zz/sys-con.git`

Like all other switch projects, you will need [devkitA64](https://switchbrew.org/wiki/Setting_up_Development_Environment) set up on your system.
(Direct link for windows https://github.com/devkitPro/installer/releases/tag/v3.0.3)

### Install dependencies
Open MSYS2 console from devkitA64 and type below command
```
pacman -S switch-libjpeg-turbo
make -C lib/libnx install
```

### Build the project
If you have **Visual Studio Code**, you can open the project as a folder and run the build tasks from inside the program. 
It also has Intellisense configured for switch development, if you have DEVKITPRO correctly defined in your environment variables.

Otherwise, you can open the console inside the project directory and use one of the following commands:
`make -C source -j8`: Build the project
`make clean`: Cleans the project files (but not the dependencies).
`make mrproper`: Cleans the project files and the dependencies.

Output folder will be there: out/
For an in-depth explanation of how sys-con works, see [here](source).

### Debug the application
All crash reports goes to /atmosphere/fatal_errors/report_xxxxx.bin (e6)
All logs goes to /config/sys-con/logs.txt

