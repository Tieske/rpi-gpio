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