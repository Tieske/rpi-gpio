---
-- Lua module to use a hd44780 compatible LCD display with the Raspberry Pi GPIO interface.
-- @module GPIO.lcd-hd44780
--
-- based on code from lrvick, LiquidCrystal and Adafruit
--
-- lrvic - `https://github.com/lrvick/raspi-hd44780/blob/master/hd44780.py`
--
-- LiquidCrystal - `https://github.com/arduino/Arduino/blob/master/libraries/LiquidCrystal/LiquidCrystal.cpp`
--
-- Adafruit - `https://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code/blob/master/Adafruit_CharLCD/Adafruit_CharLCD.py`
--
-- @copyright (c); Adafruit for the Python library, Thijs Schreijer for the Lua migration

-- Load some modules
local GPIO = require("GPIO")
local bit32 = bit32 or require("bit32")
pcall(require, "socket")  -- try and load LuaSocket for the sleep function

-- create some shortcuts
local bor, band, bnot, btest = bit32.bor, bit32.band, bit32.bnot, bit32.btest

local M = {}     -- module table to export

-- commands
M.LCD_CLEARDISPLAY         = 0x01
M.LCD_RETURNHOME           = 0x02
M.LCD_ENTRYMODESET         = 0x04
M.LCD_DISPLAYCONTROL       = 0x08
M.LCD_CURSORSHIFT          = 0x10
M.LCD_FUNCTIONSET          = 0x20
M.LCD_SETCGRAMADDR         = 0x40
M.LCD_SETDDRAMADDR         = 0x80

-- flags for display entry mode
M.LCD_ENTRYRIGHT           = 0x00
M.LCD_ENTRYLEFT            = 0x02
M.LCD_ENTRYSHIFTINCREMENT  = 0x01
M.LCD_ENTRYSHIFTDECREMENT  = 0x00

-- flags for display on/off control
M.LCD_DISPLAYON            = 0x04
M.LCD_DISPLAYOFF           = 0x00
M.LCD_CURSORON             = 0x02
M.LCD_CURSOROFF            = 0x00
M.LCD_BLINKON              = 0x01
M.LCD_BLINKOFF             = 0x00

-- flags for display/cursor shift
M.LCD_DISPLAYMOVE          = 0x08
M.LCD_CURSORMOVE           = 0x00

-- flags for display/cursor shift
M.LCD_DISPLAYMOVE          = 0x08
M.LCD_CURSORMOVE           = 0x00
M.LCD_MOVERIGHT            = 0x04
M.LCD_MOVELEFT             = 0x00

-- flags for function set
M.LCD_8BITMODE             = 0x10
M.LCD_4BITMODE             = 0x00
M.LCD_2LINE                = 0x08
M.LCD_1LINE                = 0x00
M.LCD_5x10DOTS             = 0x04
M.LCD_5x8DOTS              = 0x00

-- actually writes a byte of data to the display unit.
-- a byte is writen as 2x 4bits
-- @param self the LCD object to use
-- @param bits a byte value (0-255) containing the bits to write
-- @param char_mode a boolean (optional) indicating whether character mode is to be used (true), or command mode (false)
local function write4bits(self, bits, char_mode)
  if char_mode then char_mode=true else char_mode=false end

  M.delayMicroseconds(1000) -- 1000 microsecond sleep
  GPIO.output(self.pin_rs, char_mode)

  for n = 1, 2 do
    for i, pin in ipairs(self.pin_db) do
      local bit = (2-n)*4 + (i-1)
      local val = (btest(bits, 2^bit))
      GPIO.output(pin, val)
      --print("   ", pin, val, "i="..i, "n="..n)
    end
    self:pulseEnable()
  end

end

---
-- Sets the display size.
-- @param self display object
-- @param cols nr of columns
-- @param lines nr of lines
local function begin(self, cols, lines)
  if (lines > 1) then
    self.numlines = lines
    self.displayfunction = bor(self.displayfunction, M.LCD_2LINE)
    self.currline = 0
  end
end

local function home(self)
  write4bits(self, M.LCD_RETURNHOME) -- set cursor position to zero
  M.delayMicroseconds(3000) -- this command takes a long time!
end    

local function clear(self)
  write4bits(self, M.LCD_CLEARDISPLAY) -- command to clear display
  M.delayMicroseconds(3000)    -- 3000 microsecond sleep, clearing the display takes a long time
end

local row_offsets = { 0x00, 0x40, 0x14, 0x54 }
local function setCursor(self, col, row)
  if row > self.numlines then
    row = self.numlines - 1 -- we count rows starting w/0
  end
--TODO does python index from 0 ???
  write4bits(self, bor(M.LCD_SETDDRAMADDR, (col + row_offsets[row])))
end

local function noDisplay(self)
  self.displaycontrol = band(self.displaycontrol, bnot(M.LCD_DISPLAYON))
  write4bits(bor(self, M.LCD_DISPLAYCONTROL, self.displaycontrol))
end

local function display(self)
  self.displaycontrol = bor(self.displaycontrol, M.LCD_DISPLAYON)
  write4bits(self, bor(M.LCD_DISPLAYCONTROL, self.displaycontrol))
end

local function noCursor(self)
  self.displaycontrol = band(self.displaycontrol, bnot(M.LCD_CURSORON))
  write4bits(self, bor(M.LCD_DISPLAYCONTROL, self.displaycontrol))
end

local function cursor(self)
  self.displaycontrol = bor(self.displaycontrol, M.LCD_CURSORON)
  write4bits(self, bor(M.LCD_DISPLAYCONTROL, self.displaycontrol))
end

local function noBlink(self)
  self.displaycontrol = band(self.displaycontrol, bnot(M.LCD_BLINKON))
  write4bits(self, bor(M.LCD_DISPLAYCONTROL, self.displaycontrol))
end

local function blink(self)
  self.displaycontrol = bor(self.displaycontrol, M.LCD_BLINKON)
  write4bits(self, bor(M.LCD_DISPLAYCONTROL, self.displaycontrol))
end

local function scrollDisplayLeft(self)
  write4bits(self, bor(M.LCD_CURSORSHIFT, M.LCD_DISPLAYMOVE, M.LCD_MOVELEFT))
end

local function scrollDisplayRight(self)
  write4bits(self, bor(M.LCD_CURSORSHIFT, M.LCD_DISPLAYMOVE, M.LCD_MOVERIGHT))
end

local function leftToRight(self)
  self.displaymode = bor(self.displaycontrol, M.LCD_ENTRYLEFT)
  write4bits(self, bor(M.LCD_ENTRYMODESET, self.displaymode))
end

local function rightToLeft(self)
  self.displaymode = band(self.displaymode, bnot(M.LCD_ENTRYLEFT))
  write4bits(self, bor(M.LCD_ENTRYMODESET, self.displaymode))
end

local function autoscroll(self)
  self.displaymode = bor(self.displaycontrol, M.LCD_ENTRYSHIFTINCREMENT)
  write4bits(self, bor(M.LCD_ENTRYMODESET, self.displaymode))
end

local function noAutoscroll(self)
  self.displaymode = band(self.displaymode, bnot(M.LCD_ENTRYSHIFTINCREMENT))
  write4bits(self, bor(M.LCD_ENTRYMODESET, self.displaymode))
end

local function pulseEnable(self)
  GPIO.output(self.pin_e, false)
  M.delayMicroseconds(1)        -- 1 microsecond pause - enable pulse must be > 450ns 
  GPIO.output(self.pin_e, true)
  M.delayMicroseconds(1)        -- 1 microsecond pause - enable pulse must be > 450ns 
  GPIO.output(self.pin_e, false)
  M.delayMicroseconds(37)        -- commands need > 37us to settle
end

local function message(self, text)
  text = text:gsub("\n", string.char(0xC0))
  for i = 1, #text do
    local c = text:byte(i)
    write4bits(self, c, (c ~= 0xC0)) -- newline should pass 'false'
  end
end

---
-- This function will sleep for a number om micro (NOT milli!) seconds.
-- On first call it will replace itself with a new implementation based on LuaSocket
-- or the OS. NOTE: the OS version is really slow!
-- @param microseconds number of microseconds to sleep
function M.delayMicroseconds(microseconds)
  local sleep = (package.loaded.socket or {}).sleep
  if sleep then
    -- use LuaSocket sleep function
    M.delayMicroseconds = function(microseconds)
      sleep(microseconds/1000000)
    end
  else
    -- LuaSocket sleep function not found, so go and use OS
    M.delayMicroseconds = function(microseconds)
      os.execute(string.format("sleep %.6f", microseconds/1000000))
    end
  end
  return M.delayMicroseconds(microseconds) -- invoke the new implementation
end

--- Creates a new display object with its pin configuration.
-- @param pin_rs pin number for rs (according to current pin numbering scheme)
-- @param pin_e pin number for e (according to current pin numbering scheme)
-- @param pin_db table/list with 4 pin numbers for data (according to current pin numbering scheme)
-- @return New display object
function M.initialize(pin_rs, pin_e, pin_db)

  local self = {
    pin_rs = pin_rs,
    pin_e = pin_e,
    pin_db = { pin_db[1], pin_db[2], pin_db[3], pin_db[4] }
  }
  GPIO.setup(self.pin_rs, GPIO.OUT)
  GPIO.setup(self.pin_e, GPIO.OUT)
  for i, pin in ipairs(self.pin_db) do
    GPIO.setup(pin, GPIO.OUT)
  end

  self.displaycontrol = bor(M.LCD_DISPLAYON, M.LCD_CURSOROFF, M.LCD_BLINKOFF)
  self.displayfunction = bor(M.LCD_4BITMODE, M.LCD_1LINE, M.LCD_5x8DOTS, M.LCD_2LINE)
  self.displaymode =  bor(M.LCD_ENTRYLEFT, M.LCD_ENTRYSHIFTDECREMENT)

  self.begin = begin
  self.home = home
  self.clear = clear
  self.setCursor = setCursor
  self.noDisplay = noDisplay
  self.display = display
  self.noCursor = noCursor
  self.cursor = cursor
  self.noBlink = noBlink
  self.blink = blink
  self.scrollDisplayLeft = scrollDisplayLeft
  self.scrollDisplayRight = scrollDisplayRight
  self.leftToRight = leftToRight
  self.rightToLeft = rightToLeft
  self.autoScroll = autoScroll
  self.noAutoScroll = noAutoScroll
  self.pulseEnable = pulseEnable
  self.message = message

  write4bits(self, 0x33) -- initialization
  write4bits(self, 0x32) -- initialization
  write4bits(self, 0x28) -- 2 line 5x7 matrix
  write4bits(self, 0x0C) -- turn cursor off 0x0E to enable cursor
  write4bits(self, 0x06) -- shift cursor right

  write4bits(self, bor(M.LCD_ENTRYMODESET, self.displaymode)) --  set the entry mode

  self:clear()
  return self
end

return M
