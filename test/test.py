import time
import RPi.GPIO as GPIO

LED_PIN = 12
SWITCH_PIN = 18

def test_output():
    print('OUTPUT test')
    for i in range(10):
            time.sleep(0.1)
            GPIO.output(LED_PIN, GPIO.HIGH)
            if GPIO.input(LED_PIN) != GPIO.HIGH:
                    print('Read back of output failed.')
            time.sleep(0.1)
            GPIO.output(LED_PIN, GPIO.LOW)
            if GPIO.input(LED_PIN) != GPIO.LOW:
                    print('Read back of output failed.')

def test_input():
    print('INPUT test (Ctrl-C to stop)')
    try:
        while 1:
            GPIO.output(LED_PIN, GPIO.input(SWITCH_PIN))
            time.sleep(0.02)  # 20 ms
    except KeyboardInterrupt:
        return

def test_rising():
    def cb(chan):
        xprint('Callback 1 - this should produce an exception')
    def cb2(chan):
        print('Callback 2 called - channel %s'%chan)
        
    print('Rising edge test')
    print('5 second sample for event_detected function')
    
    try:
        GPIO.add_event_detect(LED_PIN, GPIO.RISING)
        print('Fail - added event to an output, not produced AddEventException')
    except GPIO.WrongDirectionException:
        pass
    GPIO.add_event_detect(SWITCH_PIN, GPIO.RISING)
    time.sleep(5)
    if GPIO.event_detected(SWITCH_PIN):
        print('Event detected')
    else:
        print('Event not detected')
    print('5 seconds for callback function (which should produce exceptions)')
    input('Press return to start: ')
    GPIO.add_event_callback(SWITCH_PIN, cb)
    GPIO.add_event_callback(SWITCH_PIN, cb2)
    time.sleep(5)
    GPIO.remove_event_detect(SWITCH_PIN);
    print('Blocking wait for rising edge...')
    GPIO.wait_for_edge(SWITCH_PIN, GPIO.RISING)
    
def test_falling():
    def cb(chan):
        print('Callback called - channel %s'%chan)
        
    print('Falling edge test')
    print('5 second sample for event_detected function')
    GPIO.add_event_detect(SWITCH_PIN, GPIO.FALLING)
    time.sleep(5)
    if GPIO.event_detected(SWITCH_PIN):
        print('Event detected')
    else:
        print('Event not detected')
    print('5 seconds for callback function')
    input('Press return to start: ')
    GPIO.remove_event_detect(SWITCH_PIN);
    GPIO.add_event_detect(SWITCH_PIN, GPIO.FALLING, callback=cb)
    time.sleep(5)
    GPIO.remove_event_detect(SWITCH_PIN);

def test_switchbounce():
    global count 
    count = 0

    def cb(chan):
        global count
        count += 1
        print('Switch on channel %s pressed %s!'%(chan,count))
        
    print('Switch bounce test - Ctrl-C to stop...')
    GPIO.add_event_detect(SWITCH_PIN, GPIO.FALLING, callback=cb, bouncetime=200)
    try:
        while 1:
            time.sleep(3600)
    except KeyboardInterrupt:
        pass
    GPIO.remove_event_detect(SWITCH_PIN);

def test_gpio_function():
    for chan in range(54):
        f = GPIO.gpio_function(chan)
        if f == GPIO.IN:
            func = 'INPUT'
        elif f == GPIO.OUT:
            func = 'OUTPUT'
        elif f == GPIO.ALT0:
            func = 'ALT0'
        else:
            func = 'UNKNOWN'
        print('chan=%s func=%s'%(chan,func))

def test_warnings():
    GPIO.setwarnings(False)
    print('No warning should be produced vvv')
    GPIO.setup(8, GPIO.OUT)   # is ALT0 serial TXD by default
    print('Done!')
    GPIO.setwarnings(True)
    print('Warning should be produced vvv')
    GPIO.setup(10, GPIO.OUT)  # is ALT0 serial RXD by default
    print('Done!')

def test_setup():
    print('Running setup tests...')
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(26, GPIO.OUT, initial=GPIO.HIGH) # or True
    if not GPIO.input(26):
        print('Initial state test failed')
    GPIO.setup(LED_PIN, GPIO.OUT, initial=GPIO.LOW) # or False
    if GPIO.input(LED_PIN):
        print('Initial state test failed')
    GPIO.setup(LED_PIN, GPIO.OUT)
    GPIO.setup(SWITCH_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

def test_soft_pwm():
    print('Running software PWM tests (ctrl-c to stop)...')
    pwm = GPIO.PWM(LED_PIN, 50)   # frequency  - 50Hz
    pwm.start(0)                  # duty cycle - 0 (off)
    try:
        while 1:   # make the LED fade in and out
            for x in range(0,101,5):
                pwm.ChangeDutyCycle(x)
                time.sleep(0.1)
            for x in range(100,-1,-5):
                pwm.ChangeDutyCycle(x)
                time.sleep(0.1)
    except KeyboardInterrupt:
        pass
    pwm.stop()

def test_hard_pwm():
    print('Hardware PWM - not yet implemented')

# main program starts here
while 1:
    print('1 - Setup')
    print('O - Output')
    print('I - Input')
    print('R - Rising edge')
    print('F - Falling edge')
    print('P - Software PWM')
    print('H - Hardware PWM')
    print('S - Switchbounce')
    print('G - gpio_function')
    print('B - Board revision')
    print('W - Test warnings')
    print('V - Version')
    print('X - eXit')
    command = input('Enter your choice: ').upper()

    if command.startswith('1'):
        test_setup()
    elif command.startswith('O'):
        test_output()
    elif command.startswith('I'):
        test_input()
    elif command.startswith('R'):
        test_rising()
    elif command.startswith('F'):
        test_falling()
    elif command.startswith('P'):
        test_soft_pwm()
    elif command.startswith('H'):
        test_hard_pwm()
    elif command.startswith('S'):
        test_switchbounce()
    elif command.startswith('G'):
        test_gpio_function()
    elif command.startswith('W'):
        test_warnings()
    elif command.startswith('B'):
        print('Board revision -', GPIO.RPI_REVISION)
    elif command.startswith('V'):
        print('RPi.GPIO Version',GPIO.VERSION)
    elif command.startswith('X'):
        break

GPIO.cleanup()
