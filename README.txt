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

    # set up GPIO input with pull-up control
    #   (pull_up_down be PUD_OFF, PUD_UP or PUD_DOWN, default PUD_OFF)
    GPIO.setup(11, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    # input from RPi board pin 11
    input_value = GPIO.input(11)

    # to change to BCM GPIO numbering
    GPIO.setmode(GPIO.BCM)
