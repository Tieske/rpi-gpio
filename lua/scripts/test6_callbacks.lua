-- Tests an input on P1 / pin12, as a powerswitch for the led
-- and output and P0 / pin11, as the led
-- another input on P2 / pin13, as exit button

-- requires both 'copas' and 'darksidesync' to be installed, using luarocks;
--    sudo luarocks install copas
--    sudo luarocks install darksidesync


local gpio = require("GPIO")
local dss = require("dss")
local copas = require("copas")

gpio.setmode(gpio.BOARD)
local pinin  = 12
local pinout = 11
local pinexit= 13

gpio.setup(pinin,   gpio.IN,  gpio.PUD_DOWN)
gpio.setup(pinout,  gpio.OUT, gpio.LOW)
gpio.setup(pinexit, gpio.IN,  gpio.PUD_DOWN)

local status

-- callback for first button, to switch led power
local cb = function(channel)
   if not status then
     print("Switching on")
     status = true
     gpio.output(pinout, gpio.HIGH)
   else
     print("Switching off")
     status = nil
     gpio.output(pinout, gpio.LOW)
   end
end
gpio.add_event_detect(pinin, gpio.RISING, cb, 200)

-- callback for second button, to shutdown and cleanup
local cb_exit = function(channel)
   print("Exit button pressed. Now leaving, bye bye!")
   gpio.cleanup()
   os.exit()
end
gpio.add_event_detect(pinexit, gpio.RISING, cb_exit, 200)


-- install the DSS socket receiver/handler that will 
-- do the hard work and make sure our callbacks are called
copas.addserver(
   dss.getsocket(), function(skt)
     skt = copas.wrap(skt)
     local hdlr = dss.gethandler()
     while true do
       hdlr(skt)
     end
   end)

-- start the loop
copas.loop()  -- will never return

