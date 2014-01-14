RPi.GPIO.Lua 
------------

This package provides a Lua module to control the GPIO on a Raspberry Pi.

The main functionality is provided by the [RPi.GPIO Python Module of Ben Croston](http://sourceforge.net/projects/raspberry-gpio-python/)

The following functions have been implemented for Lua:
```lua
setup
cleanup
input
output
setmode
gpio_function
setwarnings
````

The PWM-related functions are not included.

Basic usage (more advanced example below):

```lua
local GPIO=require "GPIO"

GPIO.setmode(GPIO.BOARD)

GPIO.setup(18, GPIO.IN)
GPIO.setup(11, GPIO.OUT)
GPIO.output(11, GPIO.LOW)
GPIO.cleanup()
````

There are some Lua scripts contained in the package which were tested on a Pi Rev 2 board.

[Examples and documentation of the original Lua module](http://www.andre-simon.de)

[Examples and documentation of the Python module](http://sourceforge.net/p/raspberry-gpio-python/wiki/)


Installation
------------

First install the Lua Development package if not done so already. 
On Raspbian; `sudo apt-get install liblua5.1-0-dev`

Clone the git repository, or download the tarball and unpack.

To compile the Lua module, cd to `rpi-gpio/lua` and run `make`. A `GPIO.so` file will be created, which can be used in your Lua scripts.

Alternatively use LuaRocks; `sudo luarocks make` (in the `rpi-gpio/lua` directory)

Example
-------

```lua
#!/usr/bin/lua

local GPIO=require "GPIO"

--BOARD Pin Layout
pins={11, 12, 13, 15, 16, 18, 22}

print ("Version: "..GPIO.VERSION..", Pi Rev: "..GPIO.RPI_REVISION)

GPIO.setwarnings(0)
GPIO.setmode(GPIO.BOARD)

for k,v in pairs(pins) do
  GPIO.setup(v, GPIO.OUT)
  -- or use table for named variables:
  -- GPIO.setup{channel=v, direction=GPIO.OUT}
end

for k,v in pairs(pins) do 
  GPIO.output(v, 1)
end

os.execute("sleep 1")

for k,v in pairs(pins) do 
  GPIO.output(v, (k+1)%2) 
end

os.execute("sleep 1")

for k,v in pairs(pins) do 
  GPIO.output(v, k%2) 
end

os.execute("sleep 1")

GPIO.cleanup()
```` 