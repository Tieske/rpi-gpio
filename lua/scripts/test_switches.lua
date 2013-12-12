#!/usr/bin/lua

local GPIO=require "GPIO"

-- based on the sample at http://blog.oscarliang.net/use-gpio-pins-on-raspberry-pi/

GPIO.setmode(GPIO.BOARD)

GPIO.setwarnings(0)

GPIO.setup(16, GPIO.IN)
GPIO.setup(18, GPIO.IN)

GPIO.setup(11, GPIO.OUT)
GPIO.setup(13, GPIO.OUT)
GPIO.setup(15, GPIO.OUT)
GPIO.output(11, GPIO.LOW)
GPIO.output(13, GPIO.LOW)
GPIO.output(15, GPIO.LOW)

-- state - decides what LED should be on and off
state = 0

-- increment - the direction of states
inc = 1

print ("Press PB1 to cycle thru the LEDs, Press PB2 to quit")

while GPIO.input(18) == 0 do

	-- state toggle button is pressed
        if  GPIO.input(16) == 1  then

		if inc == 1 then
			state = state + 1;
		else
			state = state - 1;
		end
		
		-- reached the max state, time to go back (decrement)
		if state == 3 then
		  inc = 0
		-- reached the min state, go back up (increment)
		elseif state == 0 then
		  inc = 1
		end
		
		if state == 1 then
			GPIO.output(11, GPIO.HIGH)
			GPIO.output(13, GPIO.LOW)
			GPIO.output(15, GPIO.LOW)
		elseif state == 2 then
			GPIO.output(11, GPIO.HIGH)
			GPIO.output(13, GPIO.HIGH)
			GPIO.output(15, GPIO.LOW)
		elseif state == 3 then
			GPIO.output(11, GPIO.HIGH)
			GPIO.output(13, GPIO.HIGH)
			GPIO.output(15, GPIO.HIGH)
		else
			GPIO.output(11, GPIO.LOW)
			GPIO.output(13, GPIO.LOW)
			GPIO.output(15, GPIO.LOW)
		end
		print("pressed PB1 ".. state)
	end
        os.execute("sleep 0.1")
end

GPIO.cleanup()