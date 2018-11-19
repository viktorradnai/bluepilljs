# BluePillJS

BluePillJS is an USB joystick firmware implemented using the "Blue Pill" STM32F103C8 microcontroller board and ChibiOS. It is a work in progress and it currently only supports the following magnetometer based sensors:

 * ST Microelectronics LSM303DLHC
 * ST Microelectronics LSM303C
 * Melexis MLX90393
 * Bourns EMS22A

BluePillJS presents a composite USB device to the USB host, which consists of a (currently single axis) joystick and a serial port. The serial port can be used to view running threads and to calibrate the joystick output. The calibration data can be saved back to flash in order to preserve it when the device is unplugged.

## Pinout

For hooking up an I2C breakout board (eg. LSM303C from Ebay) to the first I2C interface:

Pill ->  LSM303C
PB6  ->  SCL
PB7  ->  SDA
5V   ->  VIN or 3.3V -> 3.3V
GND  ->  GND

For using the Maple bootloader, connect PA12 to 3.3V using an 1.8k resistor. Please note that Chibios does not start when invoked after the Maple bootloader has initialised USB. I haven't yet figured out what caused this.

## Usage

1 Download the project
1 Download Chibios 18.2.0 and extract or symlink to `ChibiOS`
1 Run `make`
1 If you have an STLink compatible programmer, run `make install` to install the software

## Calibration

* Use a serial terminal program to connect to the USB serial port of BluePillJS. On linux I use `gtkterm -p /dev/ttyACM0`. On Linux there is a delay after plugging in BluePillJS while Linux attempts to perform modem initialisation using AT commands. During this time you may get "Device is busy" errors. After about 30 seconds you should be able to start `gtkterm` without errors.
* Type `help` and hit <enter> to get the list of available commands. You may need to type `help` again due to the initialsation issue.
* Type `cal` and hit <enter> to start calibration. Move the joystick to both endpoints, centre it again and press any key to finish.
* The calibration is done and you should see three values printed: the positive and negative multiplier and the offset from zero.
* After verifying that the joystick works as intended, type `cal_save` to write the calibration to the flash. This makes calibration data persistent until the device firmware is flashed again.
* You may confirm that the data has been written by typing `cal_load` and `cal_print`.
* You may perform these steps again any time to change the calibration.

## Issues

* Chibios does not start when invoked after the Maple bootloader has initialised USB. It only starts correctly after the bootloader writes the firmware again because it then resets the device but skips USB init.
* Linux tries to initialise the serial console as a modem, which makes it unusable for about 30 seconds after plugging it in.
* The serial port did not seem to work under Windows -- Windows 10 told me that it was missing a driver.

## To Do

* Better documentation
* Using preprocessor variables to enable / disable sensor types
* Support conventional potentiometer using ADC
* Support multiple sensors and multiple joystick axes
* Config object for defining what axis each sensor's output is going into
* Ability to configure reversed output, dead zone and non-linear (exponential) output on each axis
