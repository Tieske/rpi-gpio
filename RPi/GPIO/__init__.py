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
_GPIO_TABLE = ('0', '1', '4', '7', '8', '9', '10', '11', '14', '15', '17', '18', '21', '22', '23', '24', '25')
_ExportedIds = {}
_filehandle = {}

class InvalidIdException(Exception):
    """The Id sent is invalid on a Raspberry Pi"""
    pass

class InvalidDirectionException(Exception):
    """An invalid direction was passed to setup()"""
    pass

class WrongDirectionException(Exception):
    """The GPIO channel has not been set up or is set up in the wrong direction"""
    pass

def _GetValidId(id):
    try:
        return _GPIO_TABLE[int(id)]
    except:
        raise InvalidIdException

def setup(id, direction):
    """
    Set up the GPIO channel and direction
    id - GPIO channel (0-16)
    direction - IN or OUT
    """
    id = _GetValidId(id)
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
    with open('/sys/class/gpio/gpio%s/direction'%id) as f:
        f.write(direction)

    _ExportedIds[id] = direction
    _filehandle[id] = open('/sys/class/gpio/gpio%s/value'%id, 'w' if direction == OUT else 'r')

def output(id, value):
    """Write to a GPIO channel"""
    id = _GetValidId(id)
    if id not in _ExportedIds or _ExportedIds[id] != OUT:
        raise WrongDirectionException
    _filehandle[id].write('1' if value else '0')

def input(id):
    """Read from a GPIO channel"""
    id = _GetValidId(id)
    if id not in _ExportedIds or _ExportedIds[id] != IN:
        raise WrongDirectionException
    return _filehandle[id].read() == '1'

# clean up routine
def _unexport():
    """Clean up by unexporting evey channel that we have set up"""
    for f in _filehandle:
        close(f)
    for id in _ExportedIds:
        if os.path.exists('/sys/class/gpio/gpio%s'%id):
            with open('/sys/class/gpio/unexport', 'w') as f:
                f.write(id)
atexit.register(_unexport)

if __name__ == '__main__':
    # assumes channel 0 INPUT
    #         channel 1 OUTPUT
    setup(0, IN)
    setup(1, OUT)
    print(input(0))
    output(1, True)

