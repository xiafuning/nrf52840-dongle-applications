
# Measurement tutorial
1. Clone the repository.
```bash
$ git clone https://github.com/xiafuning/nrf52840-dongle-applications
```
2. Compile applications.
```bash
$ cd usb_communication
$ make TARGETS=<app name>
```
3. Set up the hardware as shown below.
<p align="center">
<img src="https://github.com/xiafuning/nrf52840-dongle-applications/raw/master/tutorials/hardware_setup.png" width="500">
</p>

4. Use ```packet_loss_measurement``` application to check the channel condition of each hop.
5. Run measurements.
```bash
cd usb_communication/measurement
sudo ./run_measurement_new.sh -r <rounds per run> -n <number of runs> -l <log file prefix, e.g. two_hop_<date, mmdd>> -t <setup type, one/two>
```
6. Correct log file formats.
```bash
$ cd usb_communication/measurement
$ ./json_format_correction.sh -l <log file name, e.g. test.dump>
```
7. Plot graphs using the plotting scripts.
```bash
$ cd usb_communication/measurement
$ ./measurement_plot_two_hop_new.py -j <log file prefix, e.g. two_hop_<date, e.g. mmdd>> -t channelloss/loss/tx/mix/final
```
