# sys-con

#### A Nintendo Switch custom sysmodule for third-party controller support. 

#### No man-in-the-middle required, No specific hardware required !

## Description
This sysmodule aims to provide complete functionality for any joystick or gamepad not supported natively by Nintendo Switch.
Only USB connection is supported - For bluetooth connection you can use ndeadly's [MissionControl](https://github.com/ndeadly/MissionControl)

## What is new in this fork
This fork add support to **any** HID game controllers. Meaning, any USB controller PC compatible should work with your Nintendo Switch.
While the original sys-con only support for Xbox/PS controllers, this one add support to all others PC controller without any limit.

**Important note:** This fork has not been integrated to the original one because it is far away from the original source code.
Most of the code has been rewritten and the configuration has been completely rethought. 
This means that if you already have a configuration from the original sys-con, you will have to drop it and redo it on this new fork, but it should be fairly straightforward ;).

## Installation
Grab the latest zip from the [releases page](https://github.com/o0zz/sys-con/releases). Extract it in your SD card and boot/reboot your switch.

## Configuration
sys-con comes with a config folder located at `/config/sys-con/`. It contains options for adjusting the stick/trigger deadzone and input remapping. 
The configuration is loaded in the following way:
- The [global] section is only loaded once, when the switch boots, so if you want to apply a setting, you have to reboot the switch.
- Other sections are for controller configuration, they are loaded each time you plug a controller, so if you want to apply a setting you will have to unplug and then replug your controller to apply it.

Load controller mapping order:
- First load the [default] section
- Then it will try to find a [VID-PID] section, if found it will override the default value.
- If [VID-PID] contains a [profile], it will first load the [profile] then load [VID-PID].

In other words, the loading order is: [Default] [Profile] [VID-PID].
If you want to override a setting for only 1 controller, it is better to write down your configuration in [VID-PID].

## Logs
In case of issue, you can look at the logs in `/config/sys-con/log.log`
For more verbose logs, edit config.ini and set :

```
[global]
;log_level Trace=0, Debug=1, Info=2, Warning=3, Error=4
log_level=0
```
Reboot the Nintendo Switch.

**Important note**: If you enable Trace or Debug log, the driver will automatically switch the polling frequency to 100ms (For debug) or 500ms (For trace). 
This will make the controller unresponsive or at least with a very high latency. 
Typically, if you want to press a key, you will have to hold it down for 1s.
These log levels (Trace and Debug) cannot be used to play a game, they are only here for debugging purposes.

## Features
- [x] HID joystick/gamepad supported (PC Controller compatible)
- [x] Key mapping using VID/PID or profile
- [x] Configurable deadzone
- [x] Configurable polling frequency
- [x] Configurable controller color using #RGBA
- [ ] Rumble

## Supported controller
- [x] All PC Controllers
- [x] Dualshock 4
- [x] Dualshock 3
- [x] Xbox Original
- [x] Xbox 360 Controller
- [x] Xbox 360 Wireless adapter (Up to 4 controller can be connected)
- [x] Xbox One X/S Controller
- [x] Wheels

## Controller tested
- [x] Xinmotek XM-10 (Arcade controller)
- [x] PSX Adapter
- [x] Dualshock 4
- [x] Xbox 360 Controller
- [x] Xbox 360 Wireless adapter
- [x] Logitech Driving Force GT (Wheel)
- [x] Trustmaster T150 Pro (Wheel)
- [x] BSP-D9 Mobile Phone Stretch Game Controller 
- [x] Phantom White PDP Xbox One
- [x] Wave Afterglow PDP Xbox Series
- [x] Activbb X6-34U

## How to add a new controller ?
When you plug in a new controller, most of the time only the arrow and the joystick will work, the buttons won't work by default (And right stick might be reversed).
You will have to do the key mapping yourself, follow the method below to do it.

### Method 1 (From a windows PC)

1. Connect your controller to your PC
2. Go to "Control Panel" > "Device Manager" and find your USB device under "Human Interface Devices".
3. Double click the device or right click and select Properties.
4. Go to the "Details" tab and select "Hardware IDs" to view its PID and VID. The PID/VID should look like "HID\VID_0810&PID_0001&...", that will become: `[0810-0001]`
5. Open "joy.cpl" (Either from Win+R or directly in Start Menu)
6. Select your controller and click "Properties"
7. Here you should see a panel with button ID (1, 2, 3 ...), press the button and note them (Which button is associated to which ID).
8. Now Edit /config/sys-con/config.ini on your switch sdcard and add:
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
home=11
capture=12
right_stick_x=Z
right_stick_y=Rz
```
Where 1, 2, 3, 4, ... is the key ID noted in step 7.

Note: Depending to the controller, this windows procedure might not works. If the mapping is incorrect, please switch to the Method 2.

### Method 2 (Directly from the switch logs)

1. Connect your controller to your switch and unplug it
2. Open /config/sys-con/logs.txt and look for a line like: `Trying to initialize USB device: [0810-0001] ...`
3. Now Edit /config/sys-con/config.ini on your switch sdcard and add:
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
home=11
capture=12
right_stick_x=Z
right_stick_y=Rz
``` 
Where 1, 2, 3, 4, ... are randomly set.

4. Now Re-plug your controller 
5. On your switch go to: Settings -> Controllers & Sensors > Test Input Devices
6. Try to press button you will quickly understand that the mapping is incorrect, you need now to remap it correctly by changing the /config/sys-con/config.ini

## Troubleshooting
For common issues a troubleshooting guide is available: https://github.com/o0Zz/sys-con/blob/master/doc/Troubleshooting.md

## Contribution
All contributions are welcome, you can be a simple user or developer, if you did some mapping work in the config.ini or if you have any feedback or want a new feature, feel free to send your pull request.

## Building (For developers)

Don't download the project as ZIP as it will not copy submodules properly, prefer a git clone:
`git clone --recurse-submodules -j8 https://github.com/o0Zz/sys-con.git`

Like all other switch projects, you will need [devkitA64](https://switchbrew.org/wiki/Setting_up_Development_Environment) set up on your system.
(Direct link for windows https://github.com/devkitPro/installer/releases/tag/v3.0.3)

### Install dependencies
Open MSYS2 console from devkitA64 and type below commands
```
pacman -S switch-libjpeg-turbo
make -C lib/libnx install
```

### Build the project
If you have **Visual Studio Code**, you can open the project as a folder and run the build tasks from inside the program. 
It also has Intellisense configured for switch development, if you have DEVKITPRO correctly defined in your environment variables.

Otherwise, you can open the console inside the project directory and use one of the following commands:
- `make -C source -j8`: Build the project
- `make clean`: Cleans the project files (but not the dependencies).
- `make mrproper`: Cleans the project files and the dependencies.

Output folder will be there: out/
For an in-depth explanation of how sys-con works, see [here](source).

### Debug the application
- All crash reports goes to /atmosphere/fatal_errors/report_xxxxx.bin (e6)
- All logs goes to /config/sys-con/log.log


[![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/o0Zzz)
