/*
Copyright (c) 2012-2013 Ben Croston

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
#include "event_gpio.h"
#include "cpuinfo.h"

static PyObject *AddEventException;
static PyObject *WrongDirectionException;
static PyObject *InvalidModeException;
static PyObject *InvalidDirectionException;
static PyObject *InvalidChannelException;
static PyObject *InvalidPullException;
static PyObject *InvalidEdgeException;
static PyObject *ModeNotSetException;
static PyObject *SetupException;
static PyObject *high;
static PyObject *low;
static PyObject *input;
static PyObject *output;
static PyObject *alt0;
static PyObject *board;
static PyObject *bcm;
static PyObject *pud_off;
static PyObject *pud_up;
static PyObject *pud_down;
static PyObject *rising_edge;
static PyObject *falling_edge;
static PyObject *both_edge;
static PyObject *rpi_revision;
static PyObject *version;

static int gpio_direction[54];
static const int pin_to_gpio_rev1[27] = {-1, -1, -1, 0, -1, 1, -1, 4, 14, -1, 15, 17, 18, 21, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7};
static const int pin_to_gpio_rev2[27] = {-1, -1, -1, 2, -1, 3, -1, 4, 14, -1, 15, 17, 18, 27, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7};
static const int (*pin_to_gpio)[27];
static int gpio_warnings = 1;
static int setup_error = 0;

#define MODE_UNKNOWN -1
#define BOARD        10
#define BCM          11
static int gpio_mode = MODE_UNKNOWN;

struct py_callback
{
   int gpio;
   PyObject *py_cb;
   struct py_callback *next;
};
static struct py_callback *py_callbacks = NULL;

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
      PyErr_SetString(PyExc_RuntimeError, "No access to /dev/mem.  Try running as root!");
      return SETUP_DEVMEM_FAIL;
   } else if (result == SETUP_MALLOC_FAIL) {
      PyErr_NoMemory();
      return SETUP_MALLOC_FAIL;
   } else if (result == SETUP_MMAP_FAIL) {
      PyErr_SetString(PyExc_RuntimeError, "Mmap failed on module import");
      return SETUP_MALLOC_FAIL;
   } else { // result == SETUP_OK
      return SETUP_OK;
   }
}

// python function cleanup()
static PyObject *py_cleanup(PyObject *self, PyObject *args)
{
    int i;

    // clean up any /sys/class exports
    event_cleanup();
    
    // set everything back to input
    for (i=0; i<54; i++)
        if (gpio_direction[i] != -1)
        {
//            printf("GPIO %d --> INPUT\n", i);
            setup_gpio(i, INPUT, PUD_OFF);
            gpio_direction[i] = -1;
        }

   Py_INCREF(Py_None);
   return Py_None;
}

static int verify_input(int channel, int *gpio)
{
    if (gpio_mode != BOARD && gpio_mode != BCM)
    {
        PyErr_SetString(ModeNotSetException, "Please set pin numbering mode using GPIO.setmode(GPIO.BOARD) or GPIO.setmode(GPIO.BCM)");
        return 0;
    }

    if ( (gpio_mode == BCM && (channel < 0 || channel > 53))
      || (gpio_mode == BOARD && (channel < 1 || channel > 26)) )
    {
        PyErr_SetString(InvalidChannelException, "The channel sent is invalid on a Raspberry Pi");
        return 0;
    }

    if (gpio_mode == BOARD)
    {
        *gpio = *(*pin_to_gpio+channel);
        if (*gpio == -1)
        {
            PyErr_SetString(InvalidChannelException, "The channel sent is invalid on a Raspberry Pi");
            return 0;
        }
    }
    else // gpio_mode == BCM
    {
        *gpio = channel;
    }

    if ((gpio_direction[*gpio] != INPUT) && (gpio_direction[*gpio] != OUTPUT))
    {
        PyErr_SetString(WrongDirectionException, "GPIO channel has not been set up");
        return 0;
    }
    return 1;
}

// python function setup(channel, direction, pull_up_down=PUD_OFF, initial=None)
static PyObject *py_setup_channel(PyObject *self, PyObject *args, PyObject *kwargs)
{
   int gpio, channel, direction;
   int pud = PUD_OFF;
   int initial = -1;
   static char *kwlist[] = {"channel", "direction", "pull_up_down", "initial", NULL};
   int func;

   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|ii", kwlist, &channel, &direction, &pud, &initial))
      return NULL;

   if (setup_error)
   {
      PyErr_SetString(SetupException, "Module not imported correctly!");
      return NULL;
   }

   if (direction != INPUT && direction != OUTPUT)
   {
      PyErr_SetString(InvalidDirectionException, "An invalid direction was passed to setup()");
      return NULL;
   }

   if (direction == OUTPUT)
      pud = PUD_OFF;

   if (pud != PUD_OFF && pud != PUD_DOWN && pud != PUD_UP)
   {
      PyErr_SetString(InvalidPullException, "Invalid value for pull_up_down - should be either PUD_OFF, PUD_UP or PUD_DOWN");
      return NULL;
   }

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
      gpio = *(*pin_to_gpio+channel);
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

   func = gpio_function(gpio);
   if (gpio_warnings &&                             // warnings enabled and
       ((func != 0 && func != 1) ||                 // (already one of the alt functions or
       (gpio_direction[gpio] == -1 && func == 1)))  // already an output not set from this program)
   {
      PyErr_WarnEx(NULL, "This channel is already in use, continuing anyway.  Use GPIO.setwarnings(False) to disable warnings.", 1);
   }

//   printf("Setup GPIO %d direction %d pud %d\n", gpio, direction, pud);
   if (direction == OUTPUT && (initial == LOW || initial == HIGH))
   {
//      printf("Writing intial value %d\n",initial);
      output_gpio(gpio, initial);
   }
   setup_gpio(gpio, direction, pud);
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
      gpio = *(*pin_to_gpio+channel);
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
      PyErr_SetString(WrongDirectionException, "The GPIO channel has not been set up as an OUTPUT");
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
   int gpio, channel;
   PyObject *value;

   if (!PyArg_ParseTuple(args, "i", &channel))
      return NULL;

    if (!verify_input(channel, &gpio))
        return NULL;

   //   printf("Input GPIO %d\n", gpio);
   
   if (input_gpio(gpio)) {
      value = Py_BuildValue("i", HIGH);
   } else {
      value = Py_BuildValue("i", LOW);
   }
   return value;
}

// python function setmode(mode)
static PyObject *setmode(PyObject *self, PyObject *args)
{
   if (!PyArg_ParseTuple(args, "i", &gpio_mode))
      return NULL;

   if (setup_error)
   {
      PyErr_SetString(SetupException, "Module not imported correctly!");
      return NULL;
   }

   if (gpio_mode != BOARD && gpio_mode != BCM)
   {
      PyErr_SetString(InvalidModeException, "An invalid mode was passed to setmode()");
      return NULL;
   }

   Py_INCREF(Py_None);
   return Py_None;
}

static void run_py_callbacks(int gpio)
{
   PyObject *result;
   PyGILState_STATE gstate;
   struct py_callback *cb = py_callbacks;
   
   gstate = PyGILState_Ensure();
   while (cb != NULL)
   {
      if (cb->gpio == gpio)
      {
         // run callback
         result = PyObject_CallObject(cb->py_cb, NULL);
         if (result == NULL && PyErr_Occurred())
         {
            PyErr_Print();
            PyErr_Clear();
         }
         Py_XDECREF(result);
      }
      cb = cb->next;
   }
   PyGILState_Release(gstate);
}

static int add_py_callback(unsigned int gpio, PyObject *cb_func)
{
   struct py_callback *new_py_cb;
   struct py_callback *cb = py_callbacks;

   // add callback to py_callbacks list
   new_py_cb = malloc(sizeof(struct py_callback));
   if (new_py_cb == 0)
   {
      PyErr_NoMemory();
      return -1;
   }
   new_py_cb->py_cb = cb_func;
   Py_XINCREF(cb_func);         /* Add a reference to new callback */
   new_py_cb->gpio = gpio;
   new_py_cb->next = NULL;
   if (py_callbacks == NULL) {
      py_callbacks = new_py_cb;
      add_edge_callback(gpio, run_py_callbacks);
   } else {
      // add to end of list
      while (cb->next != NULL)
         cb = cb->next;
      cb->next = new_py_cb;
   }
   return 0;
}

// python function add_event_callback(gpio, callback)
static PyObject *py_add_event_callback(PyObject *self, PyObject *args)
{
   int gpio, channel;
   PyObject *cb_func;
   
   if (!PyArg_ParseTuple(args, "iO:set_callback", &channel, &cb_func))
      return NULL;

   if (!PyCallable_Check(cb_func))
   {
      PyErr_SetString(PyExc_TypeError, "Parameter must be callable");
      return NULL;
   }

   if (setup_error)
   {
      PyErr_SetString(SetupException, "Module not imported correctly!");
      return NULL;
   }

   if (!verify_input(channel, &gpio))
      return NULL;

   if (!gpio_event_added(gpio))
   {
      PyErr_SetString(AddEventException, "Add event detection using add_event_detect first before adding a callback");
      return NULL;
   }

   if (add_py_callback(gpio, cb_func) != 0)
      return NULL;

   Py_INCREF(Py_None);
   return Py_None;
}

// python function add_event_detect(gpio, edge, callback=None)
static PyObject *py_add_event_detect(PyObject *self, PyObject *args, PyObject *kwargs)
{
   int gpio, channel, edge, result;
   PyObject *cb_func = NULL;
   static char *kwlist[] = {"gpio", "edge", "callback", NULL};

   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|O:set_callback", kwlist, &channel, &edge, &cb_func))
      return NULL;

   if (cb_func != NULL && !PyCallable_Check(cb_func))
   {
      PyErr_SetString(PyExc_TypeError, "Parameter must be callable");
      return NULL;
   }

   if (setup_error)
   {
      PyErr_SetString(SetupException, "Module not imported correctly!");
      return NULL;
   }

   if (!verify_input(channel, &gpio))
      return NULL;

   if (gpio_direction[gpio] != INPUT)
   {
      PyErr_SetString(WrongDirectionException, "You must setup() the GPIO channel as an input first");
      return NULL;
   }

   // is edge valid value
   if (edge != RISING_EDGE && edge != FALLING_EDGE && edge != BOTH_EDGE)
   {
      PyErr_SetString(InvalidEdgeException, "The edge must be set to RISING, FALLING or BOTH");
      return NULL;
   }

   if ((result = add_edge_detect(gpio, edge)) != 0)   // starts a thread
   {
      if (result == 1)
      {
         PyErr_SetString(AddEventException, "Edge detection already enabled for this GPIO channel");
         return NULL;
      } else {
         PyErr_SetString(AddEventException, "Failed to add edge detection");
         return NULL;
      }
   }

   if (cb_func != NULL)
      if (add_py_callback(gpio, cb_func) != 0)
         return NULL;

   Py_INCREF(Py_None);
   return Py_None;
}

// python function remove_event_detect(gpio)
static PyObject *py_remove_event_detect(PyObject *self, PyObject *args)
{
   int gpio, channel;
   struct py_callback *cb = py_callbacks;
   struct py_callback *temp;
   struct py_callback *prev = NULL;
   
   if (!PyArg_ParseTuple(args, "i", &channel))
      return NULL;

   if (!verify_input(channel, &gpio))
      return NULL;
   
   // remove all python callbacks for gpio
   while (cb != NULL)
   {
      if (cb->gpio == gpio)
      {
         Py_XDECREF(cb->py_cb);
         if (prev == NULL)
            py_callbacks = cb->next;
         else
            prev->next = cb->next;
         temp = cb;
         cb = cb->next;
         free(temp);
      } else {
         prev = cb;
         cb = cb->next;
      }
   }
   
   remove_edge_detect(gpio);
   
   Py_INCREF(Py_None);
   return Py_None;
}

// python function value = event_detected(channel)
static PyObject *py_event_detected(PyObject *self, PyObject *args)
{
   int gpio, channel;

   if (!PyArg_ParseTuple(args, "i", &channel))
      return NULL;

   if (!verify_input(channel, &gpio))
      return NULL;
   
   // printf("Detect event GPIO %d\n", gpio);
   if (event_detected(gpio))
      Py_RETURN_TRUE;
   else
      Py_RETURN_FALSE;
}

// python function py_wait_for_edge(gpio, edge)
static PyObject *py_wait_for_edge(PyObject *self, PyObject *args)
{
   int channel, gpio, edge, result;
   char error[30];
   
   if (!PyArg_ParseTuple(args, "ii", &channel, &edge))
      return NULL;

   if (setup_error)
   {
      PyErr_SetString(SetupException, "Module not imported correctly!");
      return NULL;
   }

   // is edge a valid value?
   if (edge != RISING_EDGE && edge != FALLING_EDGE && edge != BOTH_EDGE)
   {
      PyErr_SetString(InvalidEdgeException, "The edge must be set to RISING, FALLING or BOTH");
      return NULL;
   }

   if (!verify_input(channel, &gpio))
      return NULL;

   if (gpio_direction[gpio] != INPUT)
   {
      PyErr_SetString(WrongDirectionException, "You must setup() the GPIO channel as an input first");
      return NULL;
   }

   Py_BEGIN_ALLOW_THREADS // disable GIL
   result = blocking_wait_for_edge(gpio, edge);
   Py_END_ALLOW_THREADS   // enable GIL

   if (result == 0) {
      Py_INCREF(Py_None);
      return Py_None;
   } else if (result == 2) {
      PyErr_SetString(AddEventException, "Edge detection events already enabled for this GPIO channel");
      return NULL;
   } else {
      sprintf(error, "Error #%d waiting for edge", result);
      PyErr_SetString(PyExc_RuntimeError, error);
      return NULL;
   }
}

// python function value = gpio_function(gpio)
static PyObject *py_gpio_function(PyObject *self, PyObject *args)
{
   int gpio, f;
   PyObject *func;

   if (!PyArg_ParseTuple(args, "i", &gpio))
      return NULL;

   if (setup_error)
   {
      PyErr_SetString(SetupException, "Module not imported correctly!");
      return NULL;
   }

   f = gpio_function(gpio);
   switch (f)
   {
      case 0 : f = INPUT;  break;
      case 1 : f = OUTPUT; break;
   }
   func = Py_BuildValue("i", f);
   return func;
}

// python function setwarnings(state)
static PyObject *py_setwarnings(PyObject *self, PyObject *args)
{
   if (!PyArg_ParseTuple(args, "i", &gpio_warnings))
      return NULL;

   if (setup_error)
   {
      PyErr_SetString(SetupException, "Module not imported correctly!");
      return NULL;
   }

   Py_INCREF(Py_None);
   return Py_None;
}

PyMethodDef rpi_gpio_methods[] = {
   {"setup", (PyCFunction)py_setup_channel, METH_VARARGS | METH_KEYWORDS, "Set up the GPIO channel, direction and (optional) pull/up down control\nchannel   - Either: RPi board pin number (not BCM GPIO 00..nn number).  Pins start from 1\n            or    : BCM GPIO number\ndirection - INPUT or OUTPUT\n[pull_up_down] - PUD_OFF (default), PUD_UP or PUD_DOWN\n[initial]      - Initial value for an output channel"},
   {"cleanup", py_cleanup, METH_VARARGS, "Clean up by resetting all GPIO channels that have been used by this program to INPUT with no pullup/pulldown and no event detection"},
   {"output", py_output_gpio, METH_VARARGS, "Output to a GPIO channel\ngpio  - gpio channel\nvalue - 0/1 or False/True or LOW/HIGH"},
   {"input", py_input_gpio, METH_VARARGS, "Input from a GPIO channel.  Returns HIGH=1=True or LOW=0=False\ngpio - gpio channel"},
   {"setmode", setmode, METH_VARARGS, "Set up numbering mode to use for channels.\nBOARD - Use Raspberry Pi board numbers\nBCM   - Use Broadcom GPIO 00..nn numbers"},
   {"add_event_detect", (PyCFunction)py_add_event_detect, METH_VARARGS | METH_KEYWORDS, "Enable edge detection events for a particular GPIO channel.\nchannel    - either board pin number or BCM number depending on which mode is set.\nedge       - RISING, FALLING or BOTH\n[callback] - A callback function for the event (optional)"}, 
   {"remove_event_detect", py_remove_event_detect, METH_VARARGS, "Remove edge detection for a particular GPIO channel\ngpio - gpio channel"},
   {"event_detected", py_event_detected, METH_VARARGS, "Returns True if an edge has occured on a given GPIO.  You need to enable edge detection using add_event_detect() first.\ngpio - gpio channel"},
   {"add_event_callback", py_add_event_callback, METH_VARARGS, "Add a callback for an event already defined using add_event_detect()\ngpio     - gpio channel\ncallback - a callback function"},
   {"wait_for_edge", py_wait_for_edge, METH_VARARGS, "Wait for an edge.\ngpio - gpio channel\nedge - RISING, FALLING or BOTH"},
   {"gpio_function", py_gpio_function, METH_VARARGS, "Return the current GPIO function (IN, OUT, ALT0)\ngpio - gpio channel"},
   {"setwarnings", py_setwarnings, METH_VARARGS, "Enable or disable warning messages"},
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
   int revision = -1;

#if PY_MAJOR_VERSION > 2
   if ((module = PyModule_Create(&rpigpiomodule)) == NULL)
      return NULL;
#else
   if ((module = Py_InitModule("RPi.GPIO", rpi_gpio_methods)) == NULL)
      return;
#endif

   AddEventException = PyErr_NewException("RPi.GPIO.AddEventException", NULL, NULL);
   PyModule_AddObject(module, "AddEventException", AddEventException);
   
   WrongDirectionException = PyErr_NewException("RPi.GPIO.WrongDirectionException", NULL, NULL);
   PyModule_AddObject(module, "WrongDirectionException", WrongDirectionException);

   InvalidModeException = PyErr_NewException("RPi.GPIO.InvalidModeException", NULL, NULL);
   PyModule_AddObject(module, "InvalidModeException", InvalidModeException);

   InvalidDirectionException = PyErr_NewException("RPi.GPIO.InvalidDirectionException", NULL, NULL);
   PyModule_AddObject(module, "InvalidDirectionException", InvalidDirectionException);

   InvalidChannelException = PyErr_NewException("RPi.GPIO.InvalidChannelException", NULL, NULL);
   PyModule_AddObject(module, "InvalidChannelException", InvalidChannelException);

   InvalidPullException = PyErr_NewException("RPi.GPIO.InvalidPullException", NULL, NULL);
   PyModule_AddObject(module, "InvalidPullException", InvalidPullException);

   InvalidEdgeException = PyErr_NewException("RPi.GPIO.InvalidEdgeException", NULL, NULL);
   PyModule_AddObject(module, "InvalidEdgeException", InvalidEdgeException);

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

   alt0 = Py_BuildValue("i", ALT0);
   PyModule_AddObject(module, "ALT0", alt0);

   board = Py_BuildValue("i", BOARD);
   PyModule_AddObject(module, "BOARD", board);

   bcm = Py_BuildValue("i", BCM);
   PyModule_AddObject(module, "BCM", bcm);

   pud_off = Py_BuildValue("i", PUD_OFF);
   PyModule_AddObject(module, "PUD_OFF", pud_off);

   pud_up = Py_BuildValue("i", PUD_UP);
   PyModule_AddObject(module, "PUD_UP", pud_up);

   pud_down = Py_BuildValue("i", PUD_DOWN);
   PyModule_AddObject(module, "PUD_DOWN", pud_down);
   
   rising_edge = Py_BuildValue("i", RISING_EDGE);
   PyModule_AddObject(module, "RISING", rising_edge);
   
   falling_edge = Py_BuildValue("i", FALLING_EDGE);
   PyModule_AddObject(module, "FALLING", falling_edge);

   both_edge = Py_BuildValue("i", BOTH_EDGE);
   PyModule_AddObject(module, "BOTH", both_edge);

   // detect board revision and set up accordingly
   revision = get_rpi_revision();
   if (revision == -1)
   {
      PyErr_SetString(PyExc_RuntimeError, "This module can only be run on a Raspberry Pi!");
      setup_error = 1;
#if PY_MAJOR_VERSION > 2
      return NULL;
#else
      return;
#endif
   } else if (revision == 1) {
      pin_to_gpio = &pin_to_gpio_rev1;
   } else { // assume revision 2
      pin_to_gpio = &pin_to_gpio_rev2;
   }
   rpi_revision = Py_BuildValue("i", revision);
   PyModule_AddObject(module, "RPI_REVISION", rpi_revision);

   version = Py_BuildValue("s", "0.5.0a");
   PyModule_AddObject(module, "VERSION", version);

   // set up mmaped areas
   if (module_setup() != SETUP_OK )
   {
      setup_error = 1;
#if PY_MAJOR_VERSION > 2
      return NULL;
#else
      return;
#endif
   }
   
   // initialise events
   event_initialise();

   if (!PyEval_ThreadsInitialized())
      PyEval_InitThreads();

   // register exit functions - last declared is called first
   if (Py_AtExit(cleanup) != 0)
   {
      setup_error = 1;
      cleanup();
#if PY_MAJOR_VERSION > 2
      return NULL;
#else
      return;
#endif
   }

   if (Py_AtExit(event_cleanup) != 0)
   {
      setup_error = 1;
      cleanup();
#if PY_MAJOR_VERSION > 2
      return NULL;
#else
      return;
#endif
   }

#if PY_MAJOR_VERSION > 2
   return module;
#else
   return;
#endif
}
