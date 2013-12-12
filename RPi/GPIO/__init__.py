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

IN = 'in'
OUT = 'out'
_validids = (0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)
_ExportedIds = {}
_filehandle = {}
atexit.register(_unexport)

class InvalidIdException(Exception):
    """The Id sent is invalid on a Raspberry Pi"""
    pass

class InvalidDirectionException(Exception):
    """An invalid direction was passed to setup()"""
    pass

class WrongDirectionException(Exception):
    """The GPIO channel has not been set up or is set up in the wrong direction"""
    pass

class GPIO(object):
    """
    A class to control the GPIO on a Raspberry Pi
    """
    def _CheckValidId(self, id):
        if (type(id) == str and not isdigit(id)) and type(id) != int:
            raise InvalidIdException
        if id not in _validids:
            raise InvalidIdException

    def setup(self, id, direction):
        """Set up the GPIO channel and direction"""
        self._CheckValidId(id)
        id = str(id)
        if direction != IN and direction != OUT:
            raise InvalidDirectionException

        # unexport if it exists
        if os.path.exists('/sys/class/gpio/gpio%s'%id:
            with f as open('/sys/class/gpio/unexport', 'w'):
                f.write(id)

        # export
        with f as open('/sys/class/gpio/export', 'w'):
            f.write(id)

        # set i/o direction
        with f as open('/sys/class/gpio/gpio%s/direction'%id):
            f.write(direction)

        _ExportedIds[id] = direction
        _filehandle[id] = open('/sys/class/gpio/gpio%s/value'%id, 'w' if direction == OUT else 'r')

    def output(self, id, value):
        """Write to a GPIO channel"""
        self._CheckValidId(id)
        if id not in _ExportedIds or _ExportedIds[id] != OUT:
            raise WrongDirectionException
        _filehandle[id].write('1' if value else '0')

    def input(self, id):
        """Read from a GPIO channel"""
        self._CheckValidId(id)
        if id not in _ExportedIds or _ExportedIds[id] != IN:
            raise WrongDirectionException
        return _filehandle[id].read() == '1'

def _unexport():
    """Clean up by unexporting eveything that we have set up"""
    for f in _filehandle:
        close(f)
    for id in _ExportedIds:
        if os.path.exists('/sys/class/gpio/gpio%s'%id):
            with f as open('/sys/class/gpio/unexport', 'w'):
                f.write(str(id))

if __name__ == '__main__':
    # assumes channel 0 INPUT
    #         channel 1 OUTPUT
    gpio = GPIO()
    gpio.setup(0, IN)
    gpio.setup(1, OUT)
    print(gpio.input(0))
    gpio.output(1, True)

