This package provides a class to control the GPIO on a Raspberry Pi.

Note that this module is unsuitable for real-time or timing critical applications.  This is because you
can not predict when Python will be busy garbage collecting.  It also runs under the Linux kernel which
is not suitable for real time applications - it is multitasking O/S and another process may be given
priority over the CPU, causing jitter in your program.  If you are after true real-time performance and
predictability, buy yourself an Arduino http://www.arduino.cc !

Note that the current release does not support SPI, I2C, PWM or serial functionality on the RPi yet.
This is planned for the near future - watch this space!  One-wire functionality is also planned.

Example Usage :

::

    import RPi.GPIO as GPIO

    # to use Raspberry Pi board pin numbers
    GPIO.setmode(GPIO.BOARD)

    # set up GPIO output channel
    GPIO.setup(12, GPIO.OUT)

    # set RPi board pin 12 high
    GPIO.output(12, GPIO.HIGH)

    # set up GPIO output channel with an initial state
    GPIO.setup(26, GPIO.OUT, initial=GPIO.LOW)

    # set up GPIO input with pull-up control
    #   (pull_up_down be PUD_OFF, PUD_UP or PUD_DOWN, default PUD_OFF)
    GPIO.setup(11, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    # input from RPi board pin 11.  Will return GPIO.HIGH==1 or GPIO.LOW==0
    input_value = GPIO.input(11)

    # Enable edge detection events
    #   can be RISING, FALLING or BOTH
    GPIO.add_event_detect(11, GPIO.RISING)

    # Check to see if an event has occurred since the last time we checked (poll)
    if GPIO.event_detected(11):
        print('Rising edge has occurred!')

    # Add a threaded callback for an edge detection event. Note that event detection must be enabled first using add_event_detect()
    def my_event_callback_function():
        print('Callback function called!')
    GPIO.add_event_callback(11, my_event_callback_function)

    # Remove edge detection events for a channel
    GPIO.remove_event_detect(11)

    # Another way of adding edge detection events with a threaded callback
    def my_event_callback_function():
        print('Callback function called!')
    GPIO.add_event_detect(11, GPIO.RISING, callback=my_event_callback_function)
    
    # wait for a button press without polling (uses negligable CPU)
    GPIO.wait_for_edge(11, GPIO.RISING)

    # to change to BCM GPIO numbering
    GPIO.setmode(GPIO.BCM)

    # to reset every channel that has been set up by this program to INPUT with no pullup/pulldown and no event detection.
    GPIO.cleanup()

