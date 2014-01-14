RPi.GPIO.Lua 
------------

This package provides a Lua module to control the GPIO on a Raspberry Pi.

The main functionality is provided by the RPi.GPIO Python Module of Ben Croston:
https://code.google.com/p/raspberry-gpio-python/

The following functions have been implemented for Lua:

setup
cleanup
input
output
setmode
gpio_function
setwarnings

The PWM-related functions are not included.

Basic usage:

local GPIO=require "GPIO"

GPIO.setmode(GPIO.BOARD)

GPIO.setup(18, GPIO.IN)
GPIO.setup(11, GPIO.OUT)
GPIO.output(11, GPIO.LOW)
GPIO.cleanup()

There are some Lua scripts contained in the package which were tested on a Pi 
Rev 2 board.

For examples and documentation of the Lua module, visit 
http://www.andre-simon.de

For examples and documentation of the Python module, visit 
http://code.google.com/p/raspberry-gpio-python/wiki/Main