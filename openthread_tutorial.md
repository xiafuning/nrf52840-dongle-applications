# OpenThread tutorial
A very good step-by-step tutorial is available [here](https://codelabs.developers.google.com/codelabs/openthread-hardware/#0).
Since nRF52840 Dongle is used here, some small modifications are needed when compiling OpenThread.

Use the following command to compile the NCP joiner
```bash
$ make -f examples/Makefile-nrf52840 clean               
$ make -f examples/Makefile-nrf52840 USB=1 BOOTLOADER=USB JOINER=1               
```

Use the following command to compile the Full Thread Devices (FTDs)
```bash
$ make -f examples/Makefile-nrf52840 clean
$ make -f examples/Makefile-nrf52840 USB=1 BOOTLOADER=USB JOINER=1 COMMISSIONER=1
```
Other information regarding OpenThread on different platforms is available [here](https://github.com/openthread/openthread/blob/master/examples/platforms/nrf528xx/nrf52840/README.md).
