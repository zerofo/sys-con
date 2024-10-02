# Introduction
This documentation explain how to simulate a HID controller - Usefull to simulate particular controller on the switch and investigate what's wrong.

# Prerequisite

- Linux machine or Windows with WSL
- Attiny85: https://fr.aliexpress.com/item/1005007310893943.html + https://fr.aliexpress.com/item/1005006248717173.html
- USB ASP https://fr.aliexpress.com/item/1005001658474778.html

# Setup environment (Linux/Windows WSL)

```
sudo apt-get install avrdude gcc-avr
git clone https://github.com/kunaakos/tinyntendo.git
cd tinyntendo
make clean && make nes
```

# Flash

Plug the USB-ASP
```
sudo make install
```

## WSL setup
On WSL the USB-ASP will not be available in WSL, to forward it, follow below steps
Install https://github.com/dorssel/usbipd-win/releases

In PowerShell
```
usbipd list 
usbipd bind --busid 2-4
usbipd attach --wsl --busid 2-4
```

In Bash
```
lsusb
```

# Update code

In order to simulate a controller, you will need to update below files in the projet:

- To avoid controller to stay pressed:
```
static uint8_t readBit( void ) {
  return 0;
})
```

- To update the "GET DESCRIPTOR Response DEVICE"
`PROGMEM const char usbDescriptorDevice` 

- To update the "GET DESCRIPTOR Response CONFIGURATION"
`PROGMEM const char usbDescriptorConfiguration`

# FAQ

## avrdude: warning: cannot set sck period. please check for usbasp firmware update.
This warning is not critical and the board can flash without it