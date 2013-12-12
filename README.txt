This package provides a class to control the GPIO on a Raspberry Pi.

Note that this module is unsuitable for real-time or timing critical applications.

Example Usage :

::

    import RPi.GPIO as GPIO

    # set up the GPIO channels - one input and one output
    GPIO.setup(0, GPIO.IN)
    GPIO.setup(1, GPIO.OUT)

    # input from channel 0
    input_value = GPIO.input(0)

    # output to channel 1
    GPIO.output(1, True)

