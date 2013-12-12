/*
Copyright (c) 2012 Ben Croston

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
*/

#include "Python.h"
#include "c_gpio.h"

static PyObject *WrongDirectionException;
static PyObject *InvalidModeException;
static PyObject *InvalidDirectionException;
static PyObject *InvalidChannelException;
static PyObject *ModeNotSetException;
static PyObject *SetupException;
static PyObject *high;
static PyObject *low;
static PyObject *input;
static PyObject *output;
static PyObject *board;
static PyObject *bcm;

static int gpio_direction[54];
static const int pin_to_gpio[27] = {-1, -1, -1, 0, -1, 1, -1, 4, 14, -1, 15, 17, 18, 21, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7};

#define MODE_UNKNOWN -1
#define BOARD        10
#define BCM          11
static int gpio_mode = MODE_UNKNOWN;

// setup function run on import of the RPi.GPIO module
static int module_setup(void)
{
   int i, result;

//   printf("Setup module (mmap)\n");
   for (i=0; i<54; i++)
      gpio_direction[i] = -1;

   result = setup();
   if (result == SETUP_DEVMEM_FAIL)
   {
      PyErr_SetString(SetupException, "No access to /dev/mem.  Try running as root!");
      return SETUP_DEVMEM_FAIL;
   } else if (result == SETUP_MALLOC_FAIL) {
      PyErr_NoMemory();
      return SETUP_MALLOC_FAIL;
   } else if (result == SETUP_MMAP_FAIL) {
      PyErr_SetString(SetupException, "Mmap failed on module import");
      return SETUP_MALLOC_FAIL;
   } else { // result == SETUP_OK
      return SETUP_OK;
   }
}

// function run on exit of python
static void gpio_cleanup(void)
{
   int i;

//   printf("GPIO cleanup\n");
   for (i=0; i<54; i++)
      if (gpio_direction[i] != -1)
      {
//         printf("GPIO %d --> INPUT\n", i);
         setup_gpio(i, INPUT);
      }

    cleanup();
}

// python function setup(channel, direction)
static PyObject *py_setup_channel(PyObject *self, PyObject *args)
{
   int gpio, channel, direction;

   if (!PyArg_ParseTuple(args, "ii", &channel, &direction))
      return NULL;

   if (direction != INPUT && direction != OUTPUT)
   {
      PyErr_SetString(InvalidDirectionException, "An invalid direction was passed to setup()");
      return NULL;
   }

   if (gpio_mode != BCM && gpio_mode != BOARD)
   {
      PyErr_SetString(ModeNotSetException, "Please set mode using GPIO.setmode(GPIO.BOARD) or GPIO.setmode(GPIO.BCM)");
      return NULL;
   }

   if ( (gpio_mode == BCM && (channel < 0 || channel > 53))
     || (gpio_mode == BOARD && (channel < 1 || channel > 26)) )
   {
      PyErr_SetString(InvalidChannelException, "The channel sent is invalid on a Raspberry Pi");
      return NULL;
   }

   if (gpio_mode == BOARD)
   {
      gpio = pin_to_gpio[channel];
      if (gpio == -1)
      {
         PyErr_SetString(InvalidChannelException, "The channel sent is invalid on a Raspberry Pi");
         return NULL;
      }
   }
   else // gpio_mode == BCM
   {
      gpio = channel;
   }

//   printf("Setup GPIO %d direction %d\n", gpio, direction);
   setup_gpio(gpio, direction);
   gpio_direction[gpio] = direction;

   Py_INCREF(Py_None);
   return Py_None;
}

// python function output(channel, value)
static PyObject *py_output_gpio(PyObject *self, PyObject *args)
{
   int gpio, channel, value;

   if (!PyArg_ParseTuple(args, "ii", &channel, &value)) 
      return NULL;

   if (gpio_mode != BOARD && gpio_mode != BCM)
   {
      PyErr_SetString(ModeNotSetException, "Please set mode using GPIO.setmode(GPIO.BOARD) or GPIO.setmode(GPIO.BCM)");
      return NULL;
   }

   if ( (gpio_mode == BCM && (channel < 0 || channel > 53))
     || (gpio_mode == BOARD && (channel < 1 || channel > 26)) )
   {
      PyErr_SetString(InvalidChannelException, "The channel sent is invalid on a Raspberry Pi");
      return NULL;
   }

   if (gpio_mode == BOARD)
   {
      gpio = pin_to_gpio[channel];
      if (gpio == -1)
      {
         PyErr_SetString(InvalidChannelException, "The channel sent is invalid on a Raspberry Pi");
         return NULL;
      }
   }
   else // gpio_mode == BCM
   {
      gpio = channel;
   }

   if (gpio_direction[gpio] != OUTPUT)
   {
      PyErr_SetString(WrongDirectionException, "The GPIO channel has not been set up or is set up in the wrong direction");
      return NULL;
   }

//   printf("Output GPIO %d value %d\n", gpio, value);
   output_gpio(gpio, value);

   Py_INCREF(Py_None);
   return Py_None;
}

// python function value = input(channel)
static PyObject *py_input_gpio(PyObject *self, PyObject *args)
{
   int gpio, channel, value;

   if (!PyArg_ParseTuple(args, "i", &channel))
      return NULL;

   if (gpio_mode != BOARD && gpio_mode != BCM)
   {
      PyErr_SetString(ModeNotSetException, "Please set mode using GPIO.setmode(GPIO.BOARD) or GPIO.setmode(GPIO.BCM)");
      return NULL;
   }

   if ( (gpio_mode == BCM && (channel < 0 || channel > 53))
     || (gpio_mode == BOARD && (channel < 1 || channel > 26)) )
   {
      PyErr_SetString(InvalidChannelException, "The channel sent is invalid on a Raspberry Pi");
      return NULL;
   }

   if (gpio_mode == BOARD)
   {
      gpio = pin_to_gpio[channel];
      if (gpio == -1)
      {
         PyErr_SetString(InvalidChannelException, "The channel sent is invalid on a Raspberry Pi");
         return NULL;
      }
   }
   else // gpio_mode == BCM
   {
      gpio = channel;
   }

   if (gpio_direction[gpio] != INPUT)
   {
      PyErr_SetString(WrongDirectionException, "The GPIO channel has not been set up or is set up in the wrong direction");
      return NULL;
   }

//   printf("Input GPIO %d\n", gpio);
   value = input_gpio(gpio);
   if (value)
      Py_RETURN_TRUE;
   else
      Py_RETURN_FALSE;
}

// python function setmode(mode)
static PyObject *setmode(PyObject *self, PyObject *args)
{
   if (!PyArg_ParseTuple(args, "i", &gpio_mode))
      return NULL;

   if (gpio_mode != BOARD && gpio_mode != BCM)
   {
      PyErr_SetString(InvalidModeException, "An invalid mode was passed to setmode()");
      return NULL;
   }

   Py_INCREF(Py_None);
   return Py_None;
}

PyMethodDef rpi_gpio_methods[] = {
   {"setup", py_setup_channel, METH_VARARGS, "Set up the GPIO channel and direction\nchannel   - Either: RPi board pin number (not BCM GPIO 00..nn number).  Pins start from 1\n            or    : BCM GPIO number\ndirection - INPUT or OUTPUT"},
   {"output", py_output_gpio, METH_VARARGS, "Output to a GPIO channel"},
   {"input", py_input_gpio, METH_VARARGS, "Input from a GPIO channel"},
   {"setmode", setmode, METH_VARARGS, "Set up numbering mode to use for channels.\nBOARD - Use Raspberry Pi board numbers\nBCM   - Use Broadcom GPIO 00..nn numbers"},
   {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION > 2
static struct PyModuleDef rpigpiomodule = {
   PyModuleDef_HEAD_INIT,
   "RPi.GPIO", /* name of module */
   NULL,       /* module documentation, may be NULL */
   -1,         /* size of per-interpreter state of the module,
                  or -1 if the module keeps state in global variables. */
   rpi_gpio_methods
};
#endif

#if PY_MAJOR_VERSION > 2
PyMODINIT_FUNC PyInit_GPIO(void)
#else
PyMODINIT_FUNC initGPIO(void)
#endif
{
   PyObject *module = NULL;

#if PY_MAJOR_VERSION > 2
   if ((module = PyModule_Create(&rpigpiomodule)) == NULL)
      goto exit;
#else
   if ((module = Py_InitModule("RPi.GPIO", rpi_gpio_methods)) == NULL)
      goto exit;
#endif

   WrongDirectionException = PyErr_NewException("RPi.GPIO.WrongDirectionException", NULL, NULL);
   PyModule_AddObject(module, "WrongDirectionException", WrongDirectionException);

   InvalidModeException = PyErr_NewException("RPi.GPIO.InvalidModeException", NULL, NULL);
   PyModule_AddObject(module, "InvalidModeException", InvalidModeException);

   InvalidDirectionException = PyErr_NewException("RPi.GPIO.InvalidDirectionException", NULL, NULL);
   PyModule_AddObject(module, "InvalidDirectionException", InvalidDirectionException);

   InvalidChannelException = PyErr_NewException("RPi.GPIO.InvalidChannelException", NULL, NULL);
   PyModule_AddObject(module, "InvalidChannelException", InvalidChannelException);

   ModeNotSetException = PyErr_NewException("RPi.GPIO.ModeNotSetException", NULL, NULL);
   PyModule_AddObject(module, "ModeNotSetException", ModeNotSetException);

   SetupException = PyErr_NewException("RPi.GPIO.SetupException", NULL, NULL);
   PyModule_AddObject(module, "SetupException", SetupException);

   high = Py_BuildValue("i", HIGH);
   PyModule_AddObject(module, "HIGH", high);

   low = Py_BuildValue("i", LOW);
   PyModule_AddObject(module, "LOW", low);

   output = Py_BuildValue("i", OUTPUT);
   PyModule_AddObject(module, "OUT", output);

   input = Py_BuildValue("i", INPUT);
   PyModule_AddObject(module, "IN", input);

   board = Py_BuildValue("i", BOARD);
   PyModule_AddObject(module, "BOARD", board);

   bcm = Py_BuildValue("i", BCM);
   PyModule_AddObject(module, "BCM", bcm);

   if (module_setup() != SETUP_OK)
   {
#if PY_MAJOR_VERSION > 2
      return NULL;
#else
      return;
#endif
   }
      
   if (Py_AtExit(gpio_cleanup) != 0)
      gpio_cleanup();
      goto exit;

exit:
#if PY_MAJOR_VERSION > 2
   return module;
#else
   return;
#endif
}
