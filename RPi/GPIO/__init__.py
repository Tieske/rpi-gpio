#!/usr/bin/env python

# Copyright (c) 2012 Ben Croston
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import os
import atexit

# pins start from 1, tuple index starts from 0
_GPIO_PINS = (None, None, None, '0', None, '1', None, '4', '14', None, '15', '17', '18', '21', None, '22', '23', None, '24', '10', None, '9', '25', '11', '8', None, '7')

IN = 'in'
OUT = 'out'

_ExportedIds = {}

class InvalidPinException(Exception):
    """The pin sent is invalid on a Raspberry Pi"""
    pass

class InvalidDirectionException(Exception):
    """An invalid direction was passed to setup()"""
    pass

class WrongDirectionException(Exception):
    """The GPIO channel has not been set up or is set up in the wrong direction"""
    pass

def _GetValidId(pin):
    try:
        value = _GPIO_PINS[int(pin)]
    except:
        raise InvalidPinException
    if value is None or pin < 1:
        raise InvalidPinException
    return value

def setup(pin, direction):
    """
    Set up the GPIO channel and direction
    pin - RPi board GPIO pin number (not SOC pin number).  Pins start from 1
    direction - IN or OUT
    """
    id = _GetValidId(pin)
    if direction != IN and direction != OUT:
        raise InvalidDirectionException

    # unexport if it exists
    if os.path.exists('/sys/class/gpio/gpio%s'%id):
        with open('/sys/class/gpio/unexport', 'w') as f:
            f.write(id)

    # export
    with open('/sys/class/gpio/export', 'w') as f:
        f.write(id)

    # set i/o direction
    with open('/sys/class/gpio/gpio%s/direction'%id, 'w') as f:
        f.write(direction)
    _ExportedIds[id] = direction

def output(pin, value):
    """Write to a GPIO channel"""
    id = _GetValidId(pin)
    if id not in _ExportedIds or _ExportedIds[id] != OUT:
        raise WrongDirectionException
    with open('/sys/class/gpio/gpio%s/value'%id, 'w') as f:
        f.write('1' if value else '0')

def input(pin):
    """Read from a GPIO channel"""
    id = _GetValidId(pin)
    if id not in _ExportedIds or _ExportedIds[id] != IN:
        raise WrongDirectionException
    with open('/sys/class/gpio/gpio%s/value'%id, 'r') as f:
        return f.read(1) == '1'

# clean up routine
def _unexport():
    """Clean up by unexporting evey channel that we have set up"""
    for id in _ExportedIds:
        if os.path.exists('/sys/class/gpio/gpio%s'%id):
            with open('/sys/class/gpio/unexport', 'w') as f:
                f.write(id)
atexit.register(_unexport)

if __name__ == '__main__':
    # assumes pin 11 INPUT
    #         pin 12 OUTPUT
    setup(11, IN)
    setup(12, OUT)
    print(input(11))
    output(12, True)
