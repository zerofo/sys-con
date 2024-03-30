# sys-con

#### A Nintendo Switch custom sysmodule for third-party controller support. No man-in-the-middle required! 
###### \[Switch FW 7.0.0+\] [Atmosphère only]
Tested on Switch FW 17.0.0

## Description
This sysmodule aims to provide complete functionality for most popular game controllers not supported by Nintendo Switch.
Only USB connection is supported - For bluetooth connection you can use ndeadly's [MissionControl](https://github.com/ndeadly/MissionControl)

## Install

Grab the latest zip from the [releases page](https://github.com/o0zz/sys-con/releases). Extract it in your SD card and boot/reboot your switch.

## Config

sys-con comes with a config folder located at `sdmc:/config/sys-con/`. It contains options for adjusting stick/trigger deadzone, as well as remapping inputs. For more information, see `example.ini` in the same folder. All changes to the files will be updated in real time.

## Logs

In case of issue, you can look at the logs in `sdmc:/config/sys-con/logs.txt`
For more verbose logs, edit config_global.ini and set :

```
polling_frequency_ms=50
;log_level Debug=0, Info=1, Warning=2, Error=3
log_level=0
```

## Supported controller
- [x] Xbox 360 Controller
- [x] Xbox One X/S Controller
- [x] Dualshock 3 Support
- [x] Xbox 360 Wireless adapter
- [x] Dualshock 4
- [x] Xinmotek XM-10 (Arcade controller)

## Building (For developers)

Don't download the project as ZIP. It doesn't copy the submodules correctly and you will be left with empty folders.

Instead, clone the repository **recursively** using any git client you have. (Git Bash, Git GUI, Github Desktop, etc.)

Like all other switch projects, you need to have [devkitA64](https://switchbrew.org/wiki/Setting_up_Development_Environment) set up on your system. (Direct link for windows https://github.com/devkitPro/installer/releases/tag/v3.0.3)

### Install dependencies
Open MSYS2 console from devkitA64 and type below command
```
pacman -S switch-libjpeg-turbo

make -C lib/libnx install
```

### Build the project
If you have **Visual Studio Code**, you can open the project as a folder and run the build tasks from inside the program. 
It also has Intellisense configured for switch development, if you have DEVKITPRO correctly defined in your environment variables. Handy!

Otherwise, you can open the console inside the project directory and use one of the following commands:
`make -C source -j8`: Build the project
`make clean`: Cleans the project files (but not the dependencies).
`make mrproper`: Cleans the project files and the dependencies.

Output folder will be there: out/
For an in-depth explanation of how sys-con works, see [here](source).


### Debug the application
All crash report goes to /atmosphere/fatal_errors/report_xxxxx.bin (e6)

# TODO
 - Remove usb vendor id event when we unplug controller
 - Change remote style don't works
 - Add more generic HID decoding (According to protocol etc...)
 - Merge config files
 - Remove config file thread and load when needed
 - Config according de vendorid/productid
 - Change mapping to something like button 1, 2 etc...
 - Merge  GetInput() / GetNormalizedButtonData() 
 - Improve error code on controllers
 - m_vibrationDeviceHandle not initialized
 - Add logs in controllers
