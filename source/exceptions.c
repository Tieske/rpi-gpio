/*
Copyright (c) 2013 Ben Croston

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
#include "exceptions.h"

void define_exceptions(PyObject *module)
{
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
}
