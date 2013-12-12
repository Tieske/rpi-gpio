This package provides a class to control the GPIO lines on a Raspberry Pi.

Example Usage :

::

    import RPi.GPIO
    gpio = RPi.GPIO.GPIO()

    # set up the GPIO lines
    gpio.setup(0, RPi.GPIO.IN)
    gpio.setup(1, RPi.GPIO.OUT)

    # input from channel 0
    input_value = gpio.input(0)

    # output to channel 1
    gpio.output(1, True)

