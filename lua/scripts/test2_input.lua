-- Tests an input on P1 / pin12
-- runs for 15 seconds

local gpio = require("GPIO")

local sleep = function(seconds)
  os.execute("sleep "..tostring(seconds).."s")
end

gpio.setmode(gpio.BOARD)
local pin = 12

gpio.setup{
   channel=pin, 
   direction=gpio.IN, 
   pull_up_down = gpio.PUD_DOWN,
}

local secs=15  -- total runtime in seconds
local int=0.1  -- interval in seconds

for i = 1, (secs/int) do
   local state = gpio.input(pin)
   if state==gpio.LOW then
      print(i*int)
   else
      print("Button pressed!")
   end
   sleep(int)
end

gpio.cleanup()

