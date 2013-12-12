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
_BCM = ('0', '1', '4', '7', '8', '9', '10', '11', '14', '15', '17', '18', '21', '22', '23', '24', '25')
_MODES = (BOARD, BCM) = range(2)
_MODE = BOARD  # default mode

IN = 'in'
OUT = 'out'

_ExportedIds = {}

class InvalidChannelException(Exception):
    """The channel sent is invalid on a Raspberry Pi"""
    pass

class InvalidDirectionException(Exception):
    """An invalid direction was passed to setup()"""
    pass

class WrongDirectionException(Exception):
    """The GPIO channel has not been set up or is set up in the wrong direction"""
    pass

class InvalidModeException(Exception):
    """An invalid mode was passed to setmode()"""
    pass

def setmode(mode):
    """
    Set up numbering mode to use for channels.
    BOARD - Use Raspberry Pi board numbers
    BCM   - Use Broadcom GPIO 00..nn numbers
    """
    if mode not in _MODES:
        raise InvalidModeException
    global _MODE
    _MODE = mode

def _GetValidId(channel):
    if _MODE == BCM:
        # remove any leading zero from channel
        try:
           channel = int(channel)
        except ValueError:
            raise InvalidChannelException
        channel = str(channel)
        if channel not in _BCM:
            raise InvalidChannelException
        return channel

    # default to MODE == BOARD
    try:
        value = _GPIO_PINS[int(channel)]
    except:
        raise InvalidChannelException
    if value is None or channel < 1:
        raise InvalidChannelException
    return value

def setup(channel, direction):
    """
    Set up the GPIO channel and direction
    channel   - Either: RPi board pin number (not BCM GPIO 00..nn number).  Pins start from 1
                or    : BCM GPIO number
    direction - IN or OUT
    """
    id = _GetValidId(channel)
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

def output(channel, value):
    """Write to a GPIO channel"""
    id = _GetValidId(channel)
    if id not in _ExportedIds or _ExportedIds[id] != OUT:
        raise WrongDirectionException
    with open('/sys/class/gpio/gpio%s/value'%id, 'w') as f:
        f.write('1' if value else '0')

def input(channel):
    """Read from a GPIO channel"""
    id = _GetValidId(channel)
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
    # assumes channel 11 INPUT
    #         channel 12 OUTPUT
    setup(11, IN)
    setup(12, OUT)
    print(input(11))
    output(12, True)
