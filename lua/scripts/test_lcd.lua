-- LCD test
-- set pin and boardmode below before using!


local GPIO = require("GPIO")

-- set the pins here to match your layout
local boardmode = GPIO.BOARD    -- GPIO.BOARD or GPIO.BCM
local pin_rs = 22               -- register select pin
local pin_e  = 18               -- enable pin
local pin_db = {16, 11, 13, 15} -- data pins 1,2,3,4


local hd44780 = require("GPIO.lcd-hd44780")
GPIO.setmode(GPIO.BOARD)
local lcd = hd44780.initialize(pin_rs, pin_e, pin_db)
lcd:begin(16,1)

local cmd = "ip addr show eth0 | grep inet | awk '{print $2}' | cut -d/ -f1"
local getip = function()
  local f = io.popen(cmd)
  local r
  if f then
    r = f:read("*a")
    f:close()
  end
  return r or "no ipaddress"
end


while true do
  lcd:clear()
  lcd:message("Hello World\n")
  lcd:message('IP '..getip())
  hd44780.delayMicroseconds(1000000) -- in microseconds => 1 second
end
