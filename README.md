# Sys-con

#### Connect any USB controller to your Nintendo Switch ! 
***Support any controller***: PC controllers, Wheels, Dualshock 3, Dualshock 4, Dualsense (PS5), XBOX, XBOX360, XBOXONE, ...

## Description
Sys-con is a Nintendo Switch module that adds support for all HID and XID joysticks and gamepads to the Nintendo Switch.
Only **USB** connection is supported (For Bluetooth connection prefer to use ndeadly's [MissionControl](https://github.com/ndeadly/MissionControl))

## Installation
Download the latest zip from the [releases page](https://github.com/o0zz/sys-con/releases). Extract it to your SD card and boot/reboot your switch.

## Configuration
sys-con comes with a configuration folder located in `/config/sys-con/`. It contains configuration for controllers (Button mappings, sticks configuration, triggers configuration, deadzones...).

The configuration is loaded in the following way:
- The `[global]` section is only loaded once, when the switch boots, so if you want to apply a setting, you have to reboot the switch.
- Other sections are for controller configuration, they are loaded each time you plug a controller, so if you want to apply a setting you will have to unplug and then replug your controller to apply it.

When a new controller is plugged, the configuration is loaded in below order
1. First it load the `[default]` section
2. Then it will load the `[VID-PID]` section
3. If `[VID-PID]` contains a `[profile]`, it will load the `[profile]` then load `[VID-PID]`.

In other words, the loading order is: `[Default]` `[Profile]` `[VID-PID]`.
If you want to override a setting for only 1 controller, it's adviced to change the configuration in `[VID-PID]` in order to not impact others controllers

## Logs
In case of issue, you can look at the logs in `/config/sys-con/log.log` (On your SDCard).
The logs are automatically created with a log level equal to Info.
For more verbose logs, edit `/config/sys-con/config.ini` and set:

```
[global]
log_level=0
```

Reboot the Nintendo Switch.

**Important note**: If you enable the trace(`log_level=0`) or debug(`log_level=1`) log level, sys-con will automatically increase the polling frequency to 100ms (for debug) and 500ms (for trace). This will add a lot of latency to your controller (this isn't a problem and is expected). So, if you want to press a button, you have to hold it down for 1 second. These log levels (trace and debug) cannot be used to play a game, they are for debugging purposes only.

## Features
- [x] HID joystick/gamepad/wheels supported (PC Controller compatible)
- [x] PS and XBOXs controllers supported
- [x] Custom key mapping using VID/PID and profile.
- [x] Automatically add new controllers (Try to determine the best driver)
- [x] Configurable deadzone
- [x] Configurable polling frequency
- [x] Configurable controller color using #RGBA
- [ ] Rumble

## Supported controller
- [x] All PC Controllers
- [x] Dualshock 4
- [x] Dualshock 3
- [x] Dualsense (PS5)
- [x] Xbox Original
- [x] Xbox 360 Controller
- [x] Xbox 360 Wireless adapter (Up to 4 controller can be connected)
- [x] Xbox One X/S Controller
- [x] Wheels

## Configure a controller
When a new controller is connected, sys-con tries to determine the best profile for this new controller.
In most cases the arrows and joystick will work fine, the buttons may not be mapped correctly by default (and in rare cases the right stick may be reversed). If this is the case, you will need to map the buttons yourself using the procedure below:

### Method 1 (Directly from the switch)

1. Connect your controller to the switch
2. On the switch, go to Settings -> Controllers & Sensors > Test Input Devices
3. Press the button and try to understand the correct mapping.
3. On your SDCard, edit `/config/sys-con/config.ini` and find your controller `[vid-pid]` section (most likely the latest one).
Note: A new section will be created automatically if the controller is not known to sys-con. This means that the new controller is most likely the latest one. 

Typical configuration will look like
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
Where A, B, Y, X ... need to be set to a button id ( 1,2,3,4,5 ..)

### Method 2 (From a windows PC)
1. Connect your controller to your PC
2. Go to 'Control Panel' > 'Device Manager' and find your USB device under 'Human Interface Devices'.
3. Double click on the device or right click and select Properties.
4. Go to the 'Details' tab and select 'Hardware IDs' to view its PID and VID. The PID/VID should look like "HID\VID_0810&PID_0001&...", which becomes: `[0810-0001]`.
5. Open "joy.cpl" (either from Win+R or directly from the Start menu). 
6. Select your controller and click on "Properties
7. Here you should see a panel with button IDs (1, 2, 3 ...), press the button and make a note of them (which button is assigned to which ID).
8. Now edit `/config/sys-con/config.ini` on your switch sdcard and edit your controller's vid-pid section:

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
Note: Depending to the controller, this windows procedure might not works. If the mapping is incorrect, switch to Method 1

## Troubleshooting
For common issues a troubleshooting guide is available: [Troubleshooting](https://github.com/o0Zz/sys-con/blob/master/doc/Troubleshooting.md)

## Contribution
All contributions are welcome, you can be a simple user or developer, if you did some mapping work in the config.ini or if you have any feedback, feel free to share it in [Discussions](https://github.com/o0Zz/sys-con/discussions) or submit a [Pull request](https://github.com/o0Zz/sys-con/pulls)

## Building (For developers)

Don't download the project as ZIP as it will not copy submodules properly, prefer a git clone:
`git clone --recurse-submodules -j8 https://github.com/o0Zz/sys-con.git`

Like all other switch projects, you will need [devkitA64](https://switchbrew.org/wiki/Setting_up_Development_Environment) set up on your system.
(Direct link for windows https://github.com/devkitPro/installer/releases/tag/v3.0.3)

### Install dependencies
Open MSYS2 console from devkitA64 and type below commands:
```
pacman -S switch-libjpeg-turbo
make -C lib/libnx install
```

### Build the project
If you have **Visual Studio Code**, you can open the project as a folder and run the build tasks from inside the program. 
It also has Intellisense configured for switch development, if you have DEVKITPRO correctly defined in your environment variables.

Otherwise, you can open the console inside the project directory and use one of the following commands:
- `make -j8`: Build the project
- `make clean`: Cleans the project files (but not the dependencies).
- `make mrproper`: Cleans the project files and the dependencies.

Output folder will be there: `out/`
For an in-depth explanation of how sys-con works, see [here](source).

### Debug the application
In order to debug the applicaiton, you can directly refer to the logs available there: `/config/sys-con/log.log`.

[![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/o0Zzz)
