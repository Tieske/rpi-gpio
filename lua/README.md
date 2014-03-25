RPi.GPIO.Lua 
------------

This package provides a Lua module to control the GPIO on a Raspberry Pi.

The main functionality is provided by the 
[RPi.GPIO Python Module of Ben Croston](http://sourceforge.net/projects/raspberry-gpio-python/). 
The Lua binding follows the Python module as closely as possible.

Note; asynchroneous callbacks require [darksidesync](https://github.com/Tieske/DarkSideSync). So if you provide either 
`add_event_detect()` or `add_event_callback()` with a callback function 
you'll get a error mentioning 'darksidesync' if you did not require that module before.

Basic usage (more advanced example below):

```lua
local GPIO=require "GPIO"

GPIO.setmode(GPIO.BOARD)

GPIO.setup(18, GPIO.IN)
GPIO.setup(11, GPIO.OUT)
GPIO.output(11, GPIO.LOW)
GPIO.cleanup()
````

Documentation
-------------

- [Reference documentation](http://tieske.github.io/rpi-gpio/)
- [Example scripts](https://github.com/Tieske/rpi-gpio/tree/master/lua/scripts)
- [Examples and documentation of the Python module](http://sourceforge.net/p/raspberry-gpio-python/wiki/)

Additional modules
------------------
Some additional modules are included for specific hardware.

- `GPIO.lcd-hd44780` Module for hd44780 compatible LCDs (4bit mode), migrated to Lua from [Adafruit](https://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code/blob/master/Adafruit_CharLCD/Adafruit_CharLCD.py). See [test_lcd.lua](https://github.com/Tieske/rpi-gpio/blob/master/lua/scripts/test_lcd.lua) for an example.

Installation
------------

If you're familiar with installing packages and modules you can quickly build 
the module by cd-ing into `rpi-gpio/lua` and run `make`. A `GPIO.so` file will 
be created, which can be used in your Lua scripts.

Here's a quick list of commands for inexperienced users, to get you going with Lua on your Raspberry Pi (on Raspbian);
````
sudo apt-get update                     -- update your package cache
sudo apt-get install lua5.1             -- the Lua 5.1 interpreter and libs
sudo apt-get install liblua5.1-0-dev    -- development files, required by LuaRocks to build modules
sudo apt-get install luarocks           -- package manager for Lua modules
sudo apt-get install openssl            -- required for luasec (below)
sudo apt-get install libssl-dev         -- development files for openssl, required for luasec (below)
sudo luarocks install luarocks          -- LuaRocks will update itself to the latest version
sudo luarocks install luasec            -- required for LuaRocks to support https downloads
````

Installing the module through LuaRocks;
````
sudo luarocks install rpi-gpio
sudo luarocks install darksidesync      -- optional; only if you want to use callbacks
sudo luarocks install copas             -- optional; handy for callbacks, see 'test6_callbacks.lua'
````

If you feel like living dangerously, you can install the latest development code through;
````
sudo luarocks install rpi-gpio --server=http://luarocks.org/repositories/rocks-scm
````

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

p = GPIO.newPWM(12, 100)
p:start(50)
os.execute("sleep 5")
p:stop()

GPIO.cleanup()
```` 