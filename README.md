
# nrf52840-dongle-applications
This repository contains several applications of nRF52840 Dongle board.

## usb_communication
This is a set of simple C programs to communicate with the nRF52840 Dongle board.
### Compilation
Clone the repository on your computer.
```bash
$ cd usb_communication
$ make TARGETS=<app name>
```
The compiled binaries can be found in ```/usb_communication/build```.
Currently implemented applications are listed below.
```
usb_communication
packet_loss_measurement
lowpan_simulation
lowpan_test
wireless_nc_client
wireless_nc_relay
wireless_nc_relay_smart
wireless_nc_server
wireless_no_coding_client
wireless_no_coding_relay
wireless_no_coding_relay_smart
wireless_no_coding_server
```
### packet_loss_measurement
This application is used to measure the channel condition between two Dongle boards.
#### Usage
```bash
cd usb_communication
sudo ./build/packet_loss_measurement --port <serial port> --mode <mode>
```
Check ```./build/packet_loss_measurement -h``` for more details.

### wireless_nc_client
This application implements network coding (block NC, sparse NC, NC with recoding) and acts as the client.
#### Usage
```bash
cd usb_communication
sudo ./build/wireless_nc_client -p <serial port> -s <symbol size> -g <generation size> -r <redundancy>
```
Check ```./build/wireless_nc_client -h``` for more details.

### wireless_nc_relay
This application acts as the relay.
#### Usage
```bash
cd usb_communication
sudo ./build/wireless_nc_relay -p <serial port> -s <symbol size> -g <generation size> -r <redundancy> -l <log file name>
```
Check ```./build/wireless_nc_relay -h``` for more details.

### wireless_nc_relay_smart
This application applies a smart relay mechanism, relay application will only forward the received fragments if the recoder is full rank.
#### Usage
```bash
cd usb_communication
sudo ./build/wireless_nc_relay_smart -p <serial port> -s <symbol size> -g <generation size> -r <redundancy> -l <log file name>
```
Check ```./build/wireless_nc_relay_smart -h``` for more details.

### wireless_nc_server
This application acts as the server and handles the decoding process.
#### Usage
```bash
cd usb_communication
sudo ./build/wireless_nc_server -p <serial port> -s <symbol size> -g <generation size> -r <redundancy> -l <log file name>
```
Check ```./build/wireless_nc_server -h``` for more details.

### wireless_no_coding_client/relay/relay_smart/server
This set of programs act as the client/relay/server applications which implement OTARQ mechanism.
#### Usage
```bash
cd usb_communication
sudo ./build/<app name> -p <serial port> -s <symbol size> -g <generation size> -l <log file name>
```
Check ```./build/<app name> -h``` for more details.

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
