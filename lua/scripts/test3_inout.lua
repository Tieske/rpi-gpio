-- Tests an input on P1 / pin12, button; led on while pressed
-- and output and P0 / pin11, led
-- runs for 15 seconds

local gpio = require("GPIO")

local sleep = function(seconds)
  os.execute("sleep "..tostring(seconds).."s")
end

gpio.setmode(gpio.BOARD)
local pinin = 12
local pinout= 11

gpio.setup{
   channel=pinin, 
   direction=gpio.IN, 
   pull_up_down = gpio.PUD_DOWN,
}
gpio.setup{
   channel=pinout,
   direction=gpio.OUT,
   initial=gpio.LOW,
}

local secs=15  -- total runtime in seconds
local int=0.1  -- interval in seconds
local led

for i = 1, (secs/int) do
   if gpio.input(pinin) then
     if not led then
        print("switching on @"..i*int)
        gpio.output(pinout, gpio.HIGH)
        led = true
     end
   else
     if led then
        print("switching off @"..i*int)
        gpio.output(pinout, gpio.LOW)
        led = nil
     end
   end
   sleep(int)
end

gpio.cleanup()

