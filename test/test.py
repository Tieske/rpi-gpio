#!/usr/bin/env python
"""
Copyright (c) 2013-2014 Ben Croston

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

"""This test suite assumes the following circuit is connected:
GND_PIN = 6
LED_PIN = 12 (with resistor to 0v)
SWITCH_PIN = 18 (with 0.1 uF capacitor around switch) to 0v
LOOP_IN = 16 connected with 1K resistor to LOOP_OUT
LOOP_OUT = 22
"""

import sys
import warnings
import time
from threading import Timer
import RPi.GPIO as GPIO
if sys.version[:3] == '2.6':
    import unittest2 as unittest
else:
    import unittest

GND_PIN = 6
LED_PIN = 12
LED_PIN_BCM = 18
SWITCH_PIN = 18
LOOP_IN = 16
LOOP_OUT = 22

# Test starts with 'AAA' so that it is run first
class TestAAASetup(unittest.TestCase):
    def runTest(self):
        # Test mode not set (BOARD or BCM) exception
        with self.assertRaises(RuntimeError) as e:
            GPIO.setup(LED_PIN, GPIO.OUT)
        self.assertEqual(str(e.exception), 'Please set pin numbering mode using GPIO.setmode(GPIO.BOARD) or GPIO.setmode(GPIO.BCM)')

        GPIO.setmode(GPIO.BOARD)

        # Test not set as OUTPUT message
        with self.assertRaises(RuntimeError) as e:
            GPIO.output(LED_PIN, GPIO.HIGH)
        self.assertEqual(str(e.exception), 'The GPIO channel has not been set up as an OUTPUT')

        GPIO.setup(LED_PIN, GPIO.IN)

        # Test setup(..., pull_up_down=GPIO.HIGH) raises exception
        with self.assertRaises(ValueError):
            GPIO.setup(LED_PIN, GPIO.IN, pull_up_down=GPIO.HIGH)

        # Test 'already in use' warning
        GPIO.cleanup()
        with open('/sys/class/gpio/export','wb') as f:
            f.write(str(LED_PIN_BCM).encode())
        with open('/sys/class/gpio/gpio%s/direction'%LED_PIN_BCM,'wb') as f:
            f.write(b'out')
        with open('/sys/class/gpio/gpio%s/value'%LED_PIN_BCM,'wb') as f:
            f.write(b'1')
        with warnings.catch_warnings(record=True) as w:
            GPIO.setup(LED_PIN, GPIO.OUT)    # generate 'already in use' warning
            self.assertEqual(w[0].category, RuntimeWarning)
        with open('/sys/class/gpio/unexport','wb') as f:
            f.write(str(LED_PIN_BCM).encode())
        GPIO.cleanup()

        # test initial value of high reads back as high
        GPIO.setup(LED_PIN, GPIO.OUT, initial=GPIO.HIGH)
        self.assertEqual(GPIO.input(LED_PIN), GPIO.HIGH)
        GPIO.cleanup()

        # test initial value of low reads back as low
        GPIO.setup(LED_PIN, GPIO.OUT, initial=GPIO.LOW)
        self.assertEqual(GPIO.input(LED_PIN), GPIO.LOW)
        GPIO.cleanup()

class TestInputOutput(unittest.TestCase):
    def test_outputread(self):
        """Test that an output() can be input()"""
        GPIO.setup(LED_PIN, GPIO.OUT)
        GPIO.output(LED_PIN, GPIO.HIGH)
        self.assertEqual(GPIO.input(LED_PIN), GPIO.HIGH)
        GPIO.output(LED_PIN, GPIO.LOW)
        self.assertEqual(GPIO.input(LED_PIN), GPIO.LOW)
        GPIO.cleanup()

    def test_loopback(self):
        """Test output loops back to another input"""
        GPIO.setup(LOOP_IN, GPIO.IN, pull_up_down=GPIO.PUD_OFF)
        GPIO.setup(LOOP_OUT, GPIO.OUT, initial=GPIO.LOW)
        self.assertEqual(GPIO.input(LOOP_IN), GPIO.LOW)
        GPIO.output(LOOP_OUT, GPIO.HIGH)
        self.assertEqual(GPIO.input(LOOP_IN), GPIO.HIGH)
        GPIO.cleanup()

    def test_output_on_input(self):
        """Test output() can not be done on input"""
        GPIO.setup(SWITCH_PIN, GPIO.IN)
        with self.assertRaises(RuntimeError):
            GPIO.output(SWITCH_PIN, GPIO.LOW)
        GPIO.cleanup()

class TestSoftPWM(unittest.TestCase):
    def runTest(self):
        GPIO.setup(LED_PIN, GPIO.OUT)
        pwm = GPIO.PWM(LED_PIN, 50)
        pwm.start(100)
        print "\nPWM tests"
        response = raw_input('Is the LED on (y/n) ? ').upper()
        self.assertEqual(response,'Y')
        pwm.start(0)
        response = raw_input('Is the LED off (y/n) ? ').upper()
        self.assertEqual(response,'Y')
        print "LED Brighten/fade test..."
        for i in range(0,3):
            for x in range(0,101,5):
                pwm.ChangeDutyCycle(x)
                time.sleep(0.1)
            for x in range(100,-1,-5):
                pwm.ChangeDutyCycle(x)
                time.sleep(0.1)
        pwm.stop()
        response = raw_input('Did it work (y/n) ? ').upper()
        self.assertEqual(response,'Y')
        GPIO.cleanup()

class TestSetWarnings(unittest.TestCase):
    def test_alreadyinuse(self):
        """Test 'already in use' warning"""
        GPIO.setwarnings(False)
        with open('/sys/class/gpio/export','wb') as f:
            f.write(str(LED_PIN_BCM).encode())
        with open('/sys/class/gpio/gpio%s/direction'%LED_PIN_BCM,'wb') as f:
            f.write(b'out')
        with open('/sys/class/gpio/gpio%s/value'%LED_PIN_BCM,'wb') as f:
            f.write(b'1')
        with warnings.catch_warnings(record=True) as w:
            GPIO.setup(LED_PIN, GPIO.OUT)    # generate 'already in use' warning
            self.assertEqual(len(w),0)       # should be no warnings
        with open('/sys/class/gpio/unexport','wb') as f:
            f.write(str(LED_PIN_BCM).encode())
        GPIO.cleanup()

        GPIO.setwarnings(True)
        with open('/sys/class/gpio/export','wb') as f:
            f.write(str(LED_PIN_BCM).encode())
        with open('/sys/class/gpio/gpio%s/direction'%LED_PIN_BCM,'wb') as f:
            f.write(b'out')
        with open('/sys/class/gpio/gpio%s/value'%LED_PIN_BCM,'wb') as f:
            f.write(b'1')
        with warnings.catch_warnings(record=True) as w:
            GPIO.setup(LED_PIN, GPIO.OUT)    # generate 'already in use' warning
            self.assertEqual(w[0].category, RuntimeWarning)
        with open('/sys/class/gpio/unexport','wb') as f:
            f.write(str(LED_PIN_BCM).encode())
        GPIO.cleanup()

    def test_cleanupwarning(self):
        """Test initial GPIO.cleanup() produces warning"""
        GPIO.setwarnings(False)
        GPIO.setup(SWITCH_PIN, GPIO.IN)
        with warnings.catch_warnings(record=True) as w:
            GPIO.cleanup()
            self.assertEqual(len(w),0) # no warnings
            GPIO.cleanup()
            self.assertEqual(len(w),0) # no warnings

        GPIO.setwarnings(True)
        GPIO.setup(SWITCH_PIN, GPIO.IN)
        with warnings.catch_warnings(record=True) as w:
            GPIO.cleanup()
            self.assertEqual(len(w),0) # no warnings
            GPIO.cleanup()
            self.assertEqual(w[0].category, RuntimeWarning) # a warning

class TestVersions(unittest.TestCase):
    def test_rpi_revision(self):
        if GPIO.RPI_REVISION == 0:
            revision = 'Compute Module'
        elif GPIO.RPI_REVISION == 1:
            revision = 'revision 1'
        elif GPIO.RPI_REVISION == 2:
            revision = 'revision 2'
        elif GPIO.RPI_REVISION == 3:
            revision = 'Model B+'
        else:
            revision = '**undetected**'
        response = raw_input('\nThis board appears to be a %s - is this correct (y/n) ? '%revision).upper()
        self.assertEqual(response, 'Y')

    def test_gpio_version(self):
        response = raw_input('\nRPi.GPIO version %s - is this correct (y/n) ? '%GPIO.VERSION).upper()
        self.assertEqual(response, 'Y')

class TestGPIOFunction(unittest.TestCase):
    def runTest(self):
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(LED_PIN_BCM, GPIO.IN)
        self.assertEqual(GPIO.gpio_function(LED_PIN_BCM), GPIO.IN)
        GPIO.setup(LED_PIN_BCM, GPIO.OUT)
        self.assertEqual(GPIO.gpio_function(LED_PIN_BCM), GPIO.OUT)

        GPIO.setmode(GPIO.BOARD)
        GPIO.setup(LED_PIN, GPIO.IN)
        self.assertEqual(GPIO.gpio_function(LED_PIN), GPIO.IN)
        GPIO.setup(LED_PIN, GPIO.OUT)
        self.assertEqual(GPIO.gpio_function(LED_PIN), GPIO.OUT)

    def tearDown(self):
        GPIO.cleanup()

class TestSwitchBounce(unittest.TestCase):
    def __init__(self, *a, **k):
        unittest.TestCase.__init__(self, *a, **k)
        self.switchcount = 0

    def cb(self,chan):
        self.switchcount += 1
        print 'Button press',self.switchcount

    def setUp(self):
        GPIO.setup(SWITCH_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    def test_switchbounce(self):
        self.switchcount = 0
        print "\nSwitch bounce test.  Press switch at least 10 times and count..."
        GPIO.add_event_detect(SWITCH_PIN, GPIO.FALLING, callback=self.cb, bouncetime=200)
        while self.switchcount < 10:
            time.sleep(1)
        GPIO.remove_event_detect(SWITCH_PIN)

    def test_event_detected(self):
        self.switchcount = 0
        print "\nGPIO.event_detected() switch bounce test.  Press switch at least 10 times and count..."
        GPIO.add_event_detect(SWITCH_PIN, GPIO.FALLING, bouncetime=200)
        while self.switchcount < 10:
            if GPIO.event_detected(SWITCH_PIN):
                self.switchcount += 1
                print 'Button press',self.switchcount
        GPIO.remove_event_detect(SWITCH_PIN)

    def tearDown(self):
        GPIO.cleanup()

class TestEdgeDetection(unittest.TestCase):
    def setUp(self):
        GPIO.setup(LOOP_IN, GPIO.IN)
        GPIO.setup(LOOP_OUT, GPIO.OUT)

    def testHighLowEvent(self):
        with self.assertRaises(ValueError):
            GPIO.add_event_detect(LOOP_IN, GPIO.LOW)
        with self.assertRaises(ValueError):
            GPIO.add_event_detect(LOOP_IN, GPIO.HIGH)

    def testFallingEventDetected(self):
        GPIO.output(LOOP_OUT, GPIO.HIGH)
        GPIO.add_event_detect(LOOP_IN, GPIO.FALLING)
        time.sleep(0.001)
        self.assertEqual(GPIO.event_detected(LOOP_IN), False)
        GPIO.output(LOOP_OUT, GPIO.LOW)
        time.sleep(0.001)
        self.assertEqual(GPIO.event_detected(LOOP_IN), True)
        GPIO.output(LOOP_OUT, GPIO.HIGH)
        time.sleep(0.001)
        self.assertEqual(GPIO.event_detected(LOOP_IN), False)
        GPIO.remove_event_detect(LOOP_IN)

    def testRisingEventDetected(self):
        GPIO.output(LOOP_OUT, GPIO.LOW)
        GPIO.add_event_detect(LOOP_IN, GPIO.RISING)
        time.sleep(0.001)
        self.assertEqual(GPIO.event_detected(LOOP_IN), False)
        GPIO.output(LOOP_OUT, GPIO.HIGH)
        time.sleep(0.001)
        self.assertEqual(GPIO.event_detected(LOOP_IN), True)
        GPIO.output(LOOP_OUT, GPIO.LOW)
        time.sleep(0.001)
        self.assertEqual(GPIO.event_detected(LOOP_IN), False)
        GPIO.remove_event_detect(LOOP_IN)

    def testBothEventDetected(self):
        GPIO.output(LOOP_OUT, GPIO.LOW)
        GPIO.add_event_detect(LOOP_IN, GPIO.BOTH)
        time.sleep(0.001)
        self.assertEqual(GPIO.event_detected(LOOP_IN), False)
        GPIO.output(LOOP_OUT, GPIO.HIGH)
        time.sleep(0.001)
        self.assertEqual(GPIO.event_detected(LOOP_IN), True)
        self.assertEqual(GPIO.event_detected(LOOP_IN), False)
        GPIO.output(LOOP_OUT, GPIO.LOW)
        time.sleep(0.001)
        self.assertEqual(GPIO.event_detected(LOOP_IN), True)
        GPIO.remove_event_detect(LOOP_IN)

    def testWaitForRising(self):
        def makehigh():
            GPIO.output(LOOP_OUT, GPIO.HIGH)
        GPIO.output(LOOP_OUT, GPIO.LOW)
        t = Timer(0.1, makehigh)
        t.start()
        GPIO.wait_for_edge(LOOP_IN, GPIO.RISING)

    def testWaitForFalling(self):
        def makelow():
            GPIO.output(LOOP_OUT, GPIO.LOW)
        GPIO.output(LOOP_OUT, GPIO.HIGH)
        t = Timer(0.1, makelow)
        t.start()
        GPIO.wait_for_edge(LOOP_IN, GPIO.FALLING)

    def testExceptionInCallback(self):
        self.run_cb = False
        def cb(channel):
            with self.assertRaises(ZeroDivisionError):
                self.run_cb = True
                a = 1/0
        GPIO.output(LOOP_OUT, GPIO.LOW)
        GPIO.add_event_detect(LOOP_IN, GPIO.RISING, callback=cb)
        time.sleep(0.001)
        GPIO.output(LOOP_OUT, GPIO.HIGH)
        time.sleep(0.001)
        self.assertEqual(self.run_cb, True)
        GPIO.remove_event_detect(LOOP_IN)

    def testAddEventCallback(self):
        def cb(channel):
            self.callback_count += 1

        # falling test
        self.callback_count = 0
        GPIO.output(LOOP_OUT, GPIO.HIGH)
        GPIO.add_event_detect(LOOP_IN, GPIO.FALLING)
        GPIO.add_event_callback(LOOP_IN, cb)
        time.sleep(0.01)
        for i in range(2048):
            GPIO.output(LOOP_OUT, GPIO.LOW)
            time.sleep(0.001)
            GPIO.output(LOOP_OUT, GPIO.HIGH)
            time.sleep(0.001)
        GPIO.remove_event_detect(LOOP_IN)
        self.assertEqual(self.callback_count, 2048)

        # rising test
        self.callback_count = 0
        GPIO.output(LOOP_OUT, GPIO.LOW)
        GPIO.add_event_detect(LOOP_IN, GPIO.RISING, callback=cb)
        time.sleep(0.001)
        for i in range(2048):
            GPIO.output(LOOP_OUT, GPIO.HIGH)
            time.sleep(0.001)
            GPIO.output(LOOP_OUT, GPIO.LOW)
            time.sleep(0.001)
        GPIO.remove_event_detect(LOOP_IN)
        self.assertEqual(self.callback_count, 2048)

        # both test
        self.callback_count = 0
        GPIO.output(LOOP_OUT, GPIO.LOW)
        GPIO.add_event_detect(LOOP_IN, GPIO.BOTH, callback=cb)
        time.sleep(0.001)
        for i in range(2048):
            GPIO.output(LOOP_OUT, GPIO.HIGH)
            time.sleep(0.001)
            GPIO.output(LOOP_OUT, GPIO.LOW)
            time.sleep(0.001)
        GPIO.remove_event_detect(LOOP_IN)
        self.assertEqual(self.callback_count, 4096)

    def testEventOnOutput(self):
        with self.assertRaises(RuntimeError):
            GPIO.add_event_detect(LOOP_OUT, GPIO.FALLING)

    def tearDown(self):
        GPIO.cleanup()

class TestCleanup(unittest.TestCase):
    def test_cleanall(self):
        GPIO.setup(LOOP_OUT, GPIO.OUT)
        GPIO.setup(LED_PIN, GPIO.OUT)
        self.assertEqual(GPIO.gpio_function(LOOP_OUT), GPIO.OUT)
        self.assertEqual(GPIO.gpio_function(LED_PIN), GPIO.OUT)
        GPIO.cleanup()
        self.assertEqual(GPIO.gpio_function(LOOP_OUT), GPIO.IN)
        self.assertEqual(GPIO.gpio_function(LED_PIN), GPIO.IN)

    def test_cleanone(self):
        GPIO.setup(LOOP_OUT, GPIO.OUT)
        GPIO.setup(LED_PIN, GPIO.OUT)
        self.assertEqual(GPIO.gpio_function(LOOP_OUT), GPIO.OUT)
        self.assertEqual(GPIO.gpio_function(LED_PIN), GPIO.OUT)
        GPIO.cleanup(LOOP_OUT)
        self.assertEqual(GPIO.gpio_function(LOOP_OUT), GPIO.IN)
        self.assertEqual(GPIO.gpio_function(LED_PIN), GPIO.OUT)
        GPIO.cleanup(LED_PIN)
        self.assertEqual(GPIO.gpio_function(LOOP_OUT), GPIO.IN)
        self.assertEqual(GPIO.gpio_function(LED_PIN), GPIO.IN)

#def test_suite():
#    suite = unittest.TestLoader().loadTestsFromModule()
#    return suite

if __name__ == '__main__':
    unittest.main()
