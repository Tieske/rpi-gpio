This package provides a class to control the GPIO on a Raspberry Pi.

Note that this module is unsuitable for real-time or timing critical applications.

Example Usage :

::

    import RPi.GPIO as GPIO

    # set up the GPIO channels - one input and one output
    GPIO.setup(11, GPIO.IN)
    GPIO.setup(12, GPIO.OUT)

    # input from pin 11
    input_value = GPIO.input(11)

    # output to pin 12
    GPIO.output(12, True)

