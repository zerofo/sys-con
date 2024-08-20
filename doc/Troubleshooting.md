# Troubleshooting guide

## My controller buttons are not mapped correctly
It's expected, you need to map the buttons by yourself - There are thousands of controllers in the world, it's not possible for sys-con to map all of them.
This means, if you see that your controller works partially (stick/joystick works) but the buttons are not mapped correctly, you need to update the button mapping yourself.
See the [README](https://github.com/o0Zz/sys-con?tab=readme-ov-file#configure-a-controller) for more details.

## My controller right sticks axis are reversed. (X and Y are reversed)
If your right joystick doesn't behave as it should (for example, you press up and it goes right)
This means you need to map your joystick as well.
See the [README](https://github.com/o0Zz/sys-con?tab=readme-ov-file#configure-a-controller) 

And especially these 2 values:
```
[vid-pid]
rstick_x=Z
rstick_y=Rz
```
Where rstick_x and rstick_y could be: Z, -Z, Rz, -Rz, Rx, -Rx, Ry, -Ry, Slider, -Slider, Dial, -Dial (try all combinations to find the right one)

## My XBOX One S/X controller is not detected
This usually happens when you try to start the controller (press the XBOX button) when the USB is already connected to the switch.
Try the following procedure:
    1. Disconnect the controller from the switch
    2. Shut down the controller
    3. Power on the controller
    4. Connect the controller to the switch (make sure the switch is set to wake up).

## I got error: "Failed to acquire USB interface - Error: 0x25A8C ..."
This error occur when two drivers try to acquire the controller.
Most of the time, this issue occured when you plug an official switch controller and the switch itself try to open it.
To fix the problem:

1. Edit the `/config/sys-con/config.ini` and change `discovery_mode=0` to `discovery_mode=1`
2. Reboot the switch.

## My controller don't have Home or Capture button, how to simulate home or capture button ?
In the configuration file, edit your controller and add `simulate_home=` `simulate_capture=` followed by the buttons you want to use to simulate it.

Example:
```
[vid-pid]
simulate_home=minus+plus
simulate_capture=plus+L

[vid-pid]
simulate_home=plus+dpad_up
simulate_capture=plus+dpad_down
```

## My controller don't works at all
This probably means that your controller is not configure with the correct driver.
In the `/config/sys-con/config.ini`, you need to find your controller (\[VID/PID\]) and edit the `driver=` key with one of the following:
 - generic (default if nothing is set)
 - xbox360
 - xbox
 - xboxone
 - dualshock4
 - switch

Typically, if you know your controller is an xboxone controller, just add
```
[vid-pid]
driver=xboxone
```

## My Nintendo Switch crash during boot
If you see a crash with Program ID: 690000000000000D, it means something is wrong with sys-con.
Most of the time it's one of below reasons:

1) You selected the wrong build: Try to use the build that match your atmosphere version.
2) Something is wrong with your configuration file. Even if sys-con try to be as robust as possible, you might have an issue with your configuration. Try to restore the default one.
3) You have the latest version of atmosphere (Not yet supported by sys-con) - Open a ticket.
4) All other cases - Open a ticket

## My official Switch controller don't works when sys-con is enabled
This problem occurs because sys-con detects all connected USB devices and try to map them.
So you need to configure sys-con to detect only specific controllers.

1. Edit the `/config/sys-con/config.ini` and change `discovery_mode=0` to `discovery_mode=1`.
2. Update `discovery_vidpid=` to add the controllers you want to discover (Other than the official switch ones). 
3. Reboot the switch.

## I want to bind a button to multiple controller buttons 
It's possible to bind a button (Example: X) to multiple controller button (Example: 13 and 2):

```
[vid-pid]
x=13,2
```

## I want to bind a button to dpad or stick
It's possible to map a button (Example: X) to dpad/sticks/... (Example: dpad_up, rstick_up):

```
[vid-pid]
x=dpad_up
```

It's also possible to combinate alias and controller button:

```
[vid-pid]
x=13,2,dpad_up
```

## My controller is not working or not discovered. I don't know what todo
Enable logs in Trace mode (https://github.com/o0Zz/sys-con?tab=readme-ov-file#logs) and looks at the logs.

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

## How to attach a logs to a ticket ?
- Edit `/config/sys-con/config.ini`
- change `log_level=2` to `log_level=0`
- Reboot your console
- Wait for the console to boot
- Connect your controllers
- Wait for the issue
- Download the logs from `/config/sys-con/log.log`
