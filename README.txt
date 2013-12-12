This package provides a class to control the GPIO on a Raspberry Pi.

Note that this module is unsuitable for real-time or timing critical applications.  This is because you can not predict when Python will be busy garbage collecting.  It also runs under the Linux kernel which is not suitable for real time applications - it is multitasking O/S and another process may be given priority over the CPU, causing jitter in your program.  If you are after true real-time performance and predictability, buy yourself an Arduino http://www.arduino.cc !

Note that the current release does not support SPI, I2C, PWM or serial functionality on the RPi yet.
This is planned for the near future - watch this space!  One-wire functionality is also planned.

Example Usage :

::

    import RPi.GPIO as GPIO

    # to use Raspberry Pi board pin numbers
    GPIO.setmode(GPIO.BOARD)

    # set up the GPIO channels - one input and one output
    GPIO.setup(11, GPIO.IN)
    GPIO.setup(12, GPIO.OUT)

    # input from pin 11
    input_value = GPIO.input(11)

    # output to pin 12
    GPIO.output(12, True)

    # the same script as above but using BCM GPIO 00..nn numbers
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(17, GPIO.IN)
    GPIO.setup(18, GPIO.OUT)
    input_value = GPIO.input(17)
    GPIO.output(18, True)

