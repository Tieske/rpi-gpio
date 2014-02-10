-- Tests an input on P1 / pin12 using edge detection
-- waits for a button press, and then a release again

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

gpio.wait_for_edge(pinin, gpio.RISING)
print("switching on")
gpio.output(pinout, gpio.HIGH)

gpio.wait_for_edge(pinin, gpio.FALLING)
print("switching off")
gpio.output(pinout, gpio.LOW)

gpio.cleanup()

