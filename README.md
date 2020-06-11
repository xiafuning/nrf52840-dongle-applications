# nrf52840-dongle-applications
This repository contains several applications of nRF52840 Dongle board.

## usb_communication
This is a set of simple C programs to communicate with the nRF52840 Dongle board.
### Usage
Clone the repository on your computer.
```bash
$ cd usb_communication
$ make TARGETS=<app name>
$ sudo ./build/<app name> --port <serial port number> --mode <client/server/echo>
```
Currently implemented applications are listed below.
```
usb_communication
packet_loss_measurement
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
## wireless_echo
This application receives packets from 802.15.4 radio and send them back. The running of this application is independent of PC.
### Usage
Clone the repository on your computer and copy the wireless_echo folder to the right directory.
```bash
$ cp -r wireless_echo <nRF5_SDK root directory>/examples/802_15_4/
```
Compile the application.
```bash
$ cd <nRF5_SDK root directory>/examples/802_15_4/wireless_echo/raw/<first/second/third>/pca10059_xfn/blank/armgcc
$ make
```
Flash the generated hex file to the board using [nRF Connect for Desktop](https://www.nordicsemi.com/eng/Products/Bluetooth-low-energy/nRF-Connect-for-Desktop) or [pc-nrfutil](https://github.com/NordicSemiconductor/pc-nrfutil).
