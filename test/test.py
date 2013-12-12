import time
import RPi.GPIO as GPIO

def test_output():
    print('OUTPUT test')
    for i in range(10):
            time.sleep(0.1)
            GPIO.output(16, GPIO.HIGH)
            if GPIO.input(16) != GPIO.HIGH:
                    print('Read back of output failed.')
            time.sleep(0.1)
            GPIO.output(16, GPIO.LOW)
            if GPIO.input(16) != GPIO.LOW:
                    print('Read back of output failed.')

def test_input():
    print('INPUT test (Ctrl-C to stop)')
    try:
        while 1:
            GPIO.output(16, GPIO.input(18))
            time.sleep(0.02)  # 20 ms
    except KeyboardInterrupt:
        return

def test_rising():
    def cb():
        xprint('Callback 1 - this should produce an exception')
    def cb2():
        print('Callback 2 called')
        
    print('Rising edge test')
    print('5 second sample for event_detected function')
    
    try:
        GPIO.add_event_detect(16, GPIO.RISING)
        print('Fail - added event to an output, not produced AddEventException')
    except GPIO.WrongDirectionException:
        pass
    GPIO.add_event_detect(18, GPIO.RISING)
    time.sleep(5)
    if GPIO.event_detected(18):
        print('Event detected')
    else:
        print('Event not detected')
    print('5 seconds for callback function (which should produce exceptions)')
    input('Press return to start: ')
    GPIO.add_event_callback(18, cb)
    GPIO.add_event_callback(18, cb2)
    time.sleep(5)
    GPIO.remove_event_detect(18);
    print('Blocking wait for rising edge...')
    GPIO.wait_for_edge(18, GPIO.RISING)
    
def test_falling():
    def cb():
        print('Callback called!')
        
    print('Falling edge test')
    print('5 second sample for event_detected function')
    GPIO.add_event_detect(18, GPIO.FALLING)
    time.sleep(5)
    if GPIO.event_detected(18):
        print('Event detected')
    else:
        print('Event not detected')
    print('5 seconds for callback function')
    input('Press return to start: ')
    GPIO.remove_event_detect(18);
    GPIO.add_event_detect(18, GPIO.FALLING, callback=cb)
    time.sleep(5)
    GPIO.remove_event_detect(18);

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
    GPIO.setup(16, GPIO.OUT, initial=GPIO.LOW) # or False
    if GPIO.input(16):
        print('Initial state test failed')
    GPIO.setup(16, GPIO.OUT)
    GPIO.setup(18, GPIO.IN, pull_up_down=GPIO.PUD_UP)

# main program starts here
while 1:
    print('1 - Setup')
    print('O - Output')
    print('I - Input')
    print('R - Rising edge')
    print('F - Falling edge')
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
