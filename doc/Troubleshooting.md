# Troubleshooting guide

## My controller pads/joysticks works but buttons don't works
You need to map your buttons - Because there is thousand of controllers in the world, it's not possible for this sys-module to map all of them.
It means, if you see your controller is working partially (Stick/Joystick are working) but buttons not, you have to maps the buttons yourself.
Visit the [README](https://github.com/o0Zz/sys-con?tab=readme-ov-file#how-to-add-a-new-controller-) for more details.

## My controller right sticks axis are reversed. (X and Y are reversed)
If your right joystick don't behave as it should (Exemple, you press right, it goes up, left, down)
It means, you need to map your joystick as well.
Visit the [README](https://github.com/o0Zz/sys-con?tab=readme-ov-file#how-to-add-a-new-controller-) 

And in particular these 2 values:
```
right_stick_x=Z
right_stick_y=Rz
```
Where right_stick_x and right_stick_y might be: Z, -Z, Rz, -Rz, Rx, -Rx, Ry, -Ry (Try all combinaisons until find the correct one)

## My controller don't works at all
It probably means your controller is not present in the config.ini file with the correct driver. 
You need to determine the VID/PID and create a new section in the ini file, then select the correct driver to use:
 - generic (Default mode if nothing is set)
 - xbox360
 - xbox
 - xboxone
 - dualshock4

Typically, if you know your controller is a xboxone controller just add:
```
[vid-pid]
driver=xboxone
```

## My official Switch controller don't works when sys-con is enabled
This issue happen, because sys-con grab all USB devices you try to plug. Thus your need to configure sys-con to only discover certains controllers.

1. Edit the config.ini and change `discovery_mode` by `discovery_mode=1`
2. Update `discovery_vidpid=` in order to add the controllers you want to discover (Other than the official switch ones). 
3. Reboot the switch.

## My controller is not working or not discovered. I don't know what todo
Enable logs in Debug mode (Or Trace) (https://github.com/o0Zz/sys-con?tab=readme-ov-file#logs) and looks at the logs.

A typically working flow will look like:

```
|I|00:03:46.0125| New USB device detected, checking for controllers ...
|I|00:03:46.0144| Trying to initialize USB device: [12bd-e002] ...
|D|00:03:46.0161| Loading controller config: 'sdmc:///config/sys-con/config.ini' (default) ...
|D|00:03:46.0331| Loading controller config: 'sdmc:///config/sys-con/config.ini' (12bd-e002) ...
|D|00:03:46.0510| Loading controller successfull (B=3, A=2, Y=4, X=1, ...)
|I|00:03:46.0524| Initializing Generic controller for [12bd-e002] ...
|D|00:03:46.0540| BaseController Created for 12bd-e002
|D|00:03:46.0556| GenericHIDController Created for 12bd-e002
|D|00:03:46.0571| SwitchHDLHandler Initializing ...
|D|00:03:46.0581| GenericHIDController Initializing ...
|D|00:03:46.0596| BaseController Initializing ...
|D|00:03:46.0610| BaseController Opening interfaces ...
|D|00:03:46.0626| BaseController Opening interface 0 ...
|D|00:03:46.0640| SwitchUSBInterface(12bd-e002) Openning ...
|D|00:03:46.0655| SwitchUSBInterface(12bd-e002) Input endpoint found 0x81 (Idx: 0)
|D|00:03:46.0665| SwitchUSBEndpoint Opening 0x81 (Pkt size: 7)...
|D|00:03:46.0679| SwitchUSBEndpoint successfully opened!
|I|00:03:46.0693| BaseController successfully opened !
|D|00:03:46.0709| SwitchUSBInterface(12bd-e002) ControlTransferInput (bmRequestType=0x81, bmRequest=0x06, wValue=0x2200, wIndex=0x0000, wLength=512)...
|D|00:03:46.0726| GenericHIDController Parsing descriptor ...
|D|00:03:46.0743| GenericHIDController Looking for joystick/gamepad profile ...
|I|00:03:46.0753| GenericHIDController USB joystick successfully opened (1 inputs detected) !
|D|00:03:46.0765| SwitchHDLHandler Initializing HDL state ...
|D|00:03:46.0780| SwitchHDLHandler Initializing HDL device idx: 0 ...
|D|00:03:46.0792| SwitchHDLHandler HDL state successfully initialized !
|D|00:03:46.0806| SwitchVirtualGamepadHandler InputThreadLoop running ...
|D|00:03:46.0821| SwitchHDLHandler Initialized !
```

Search for logs starting with `|E|`, If you find one, this is an error and it might give you a hint about the issue.

## How to attach a logs to a ticket
- Edit `/config/sys-con/config.ini`
- change `log_level=2` to `log_level=0`
- Reboot your console
- Wait for the console to boot
- Plug your Controllers
- Wait for the issue
- Download the logs from `/config/sys-con/log.log`