-- Blinks led on P0 / pin11
-- Exits after 10 blinks

local gpio = require("GPIO")

local sleep = function(seconds)
  os.execute("sleep "..tostring(seconds).."s")
end

gpio.setmode(gpio.BOARD)
gpio.setup(11, gpio.OUT)

for i = 1,10 do
   gpio.output(11, gpio.HIGH)
   sleep(0.3)
   gpio.output(11, gpio.LOW)
   sleep(0.3)
end

gpio.cleanup()

