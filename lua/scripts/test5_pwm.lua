-- Tests a led PWM on output P0 / pin11
-- runs for 10 seconds

local gpio = require("GPIO")

local sleep = function(seconds)
  os.execute("sleep "..tostring(seconds).."s")
end

gpio.setmode(gpio.BCM)
local pinout= 17

gpio.setup{
   channel=pinout,
   direction=gpio.OUT,
   initial=gpio.LOW,
}

pwm = gpio.newPWM(pinout, 100) -- PWM instance at 100 Hz
pwm:start(0)  -- dutycycle at 0%

local s,e,step = 0, 100, 10  -- settings first run -> up
for _=1,2 do                 -- 2 runs
  for i=s,e,step do
    pwm:ChangeDutyCycle(i)
    print(i)
    sleep(0.5)
  end
  s,e,step = 100,0,-10       -- settings second run -> down
end

pwm:stop()
gpio.cleanup()

