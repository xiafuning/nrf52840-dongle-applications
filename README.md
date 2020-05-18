# nrf52840-dongle-applications
This repository contains several applications of nRF52840 Dongle board.

## usb_communication
This is a simple C program to communicate with the nRF52840 Dongle board.
### Usage
Clone the repository on your computer.
```bash
$ cd usb_communication
$ make
$ sudo ./build/usb_comm_out <serial port, e.g. /dev/ttyACM0> <mode: client/server/echo>
```
## wireless_usb_cdc_acm
This application implements 802.15.4 for wireless communication and USB CDC ACM for serial communication between board and PC.
### Usage
Clone the repository on your computer and copy the wireless_usb_cdc_acm folder to the right directory.
```bash
$ cp -r wireless_usb_cdc_acm <nRF5_SDK root directory>/examples/802_15_4/
```
Compile the application.
```bash
$ cd <nRF5_SDK root directory>/examples/802_15_4/wireless_usb_cdc_acm/raw/<first/second/third>/pca10059_xfn/blank/armgcc
$ make
```
Flash the generated hex file to the board using [nRF Connect for Desktop](https://www.nordicsemi.com/eng/Products/Bluetooth-low-energy/nRF-Connect-for-Desktop) or [pc-nrfutil](https://github.com/NordicSemiconductor/pc-nrfutil).
