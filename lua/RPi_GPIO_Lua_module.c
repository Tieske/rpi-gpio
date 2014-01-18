/*
Copyright (c) 2013 Andre Simon

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


#define LUA_MODULE_VERSION "0.5.4 (Lua)"
#define LUA_PUD_CONST_OFFSET 20
#define LUA_EVENT_CONST_OFFSET 30
#define PWM_MT_NAME "RPI-GPIO PWM MT"

#include <errno.h>
#include <string.h>

#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>

#include "c_gpio.h"
#include "event_gpio.h"
#include "cpuinfo.h"
#include "common.h"
#include "soft_pwm.h"

typedef struct
{
    unsigned int gpio;
    float freq;
    float dutycycle;
} PWMObject;

int gpio_mode = MODE_UNKNOWN;
const int pin_to_gpio_rev1[27] = {-1, -1, -1, 0, -1, 1, -1, 4, 14, -1, 15, 17, 18, 21, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7};
const int pin_to_gpio_rev2[27] = {-1, -1, -1, 2, -1, 3, -1, 4, 14, -1, 15, 17, 18, 27, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7};

static int gpio_warnings = 1;

unsigned int lua_get_gpio_number(lua_State *L, int channel)
{
    unsigned int gpio;
    
    // check setmode() has been run
    if (gpio_mode != BOARD && gpio_mode != BCM)
        return (unsigned int)luaL_error(L, "Please set pin numbering mode using GPIO.setmode(GPIO.BOARD) or GPIO.setmode(GPIO.BCM)");

    // check channel number is in range
    if ( (gpio_mode == BCM && (channel < 0 || channel > 53))
      || (gpio_mode == BOARD && (channel < 1 || channel > 26)) )
        return (unsigned int)luaL_error(L, "The channel sent is invalid on a Raspberry Pi");

    // convert channel to gpio
    if (gpio_mode == BOARD)
    {
        if (*(*pin_to_gpio+channel) == -1)
        {
            return (unsigned int)luaL_error(L, "The channel sent is invalid on a Raspberry Pi");
        } else {
            gpio = *(*pin_to_gpio+channel);
        }
    }
    else // gpio_mode == BCM
    {
        gpio = channel;
    }

    return gpio;
}

static int lua_setmode(lua_State *L)
{
   int mode = luaL_checkint(L, 1);
   
   if (mode != BOARD && mode != BCM)
      return luaL_error(L, "An invalid mode was passed to setmode()");
      
   gpio_mode = mode;
   return 0;
}

static int lua_setwarnings(lua_State *L)
{
   gpio_warnings = lua_toboolean(L, 1);
   return 0;
}
  
static int lua_setup_channel(lua_State *L)
{
   unsigned int gpio;
   int channel, direction;
   int pud = PUD_OFF + LUA_PUD_CONST_OFFSET;
   int initial = -1;
   int func;
   
   if (lua_type(L, 1) == LUA_TTABLE){
     
     lua_pushstring(L, "channel");
     lua_gettable(L, -2);
     channel = luaL_checkint(L, -1);

     lua_pop(L, 1);
     lua_pushstring(L, "direction");
     lua_gettable(L, -2);
     direction = luaL_checkint(L, -1);

     lua_pop(L, 1);
     lua_pushstring(L, "pull_up_down");
     lua_gettable(L, -2);
     if(lua_isnumber(L, -1)) {
            pud = lua_tonumber(L, -1);
     } 
     lua_pop(L, 1);
     lua_pushstring(L, "initial");
     lua_gettable(L, -2);
     if(lua_isnumber(L, -1)) {
            initial = lua_tonumber(L, -1);
     }    
     
   } else {
      
      if (lua_gettop(L) <2)
              return luaL_error(L, "An invalid number of args was passed to setup()");
      
      channel = luaL_checkint(L, 1);
      direction = luaL_checkint(L, 2);
      
      if (lua_gettop(L)>=3)
        pud=luaL_checkint(L, 3);
      if (lua_gettop(L)>=4)
        initial=luaL_checkint(L, 4);
   }
   
   gpio = lua_get_gpio_number(L, channel);

   if (direction != INPUT && direction != OUTPUT)
      return luaL_error(L,  "An invalid direction was passed to setup()");

   if (direction == OUTPUT)
      pud = PUD_OFF + LUA_PUD_CONST_OFFSET;

   pud -= LUA_PUD_CONST_OFFSET;
   if (pud != PUD_OFF && pud != PUD_DOWN && pud != PUD_UP)
      return luaL_error(L,  "Invalid value for pull_up_down - should be either PUD_OFF, PUD_UP or PUD_DOWN");

   func = gpio_function(gpio);
   if (gpio_warnings &&                             // warnings enabled and
       ((func != 0 && func != 1) ||                 // (already one of the alt functions or
       (gpio_direction[gpio] == -1 && func == 1)))  // already an output not set from this program)
   {
      fprintf(stderr, "This channel is already in use, continuing anyway.  Use GPIO.setwarnings(False) to disable warnings.");
   }

   if (direction == OUTPUT && (initial == LOW || initial == HIGH))
   {
      output_gpio(gpio, initial);
   }
   setup_gpio(gpio, direction, pud);
   gpio_direction[gpio] = direction;

   return 0;   
}
  

static int lua_output_gpio(lua_State* L)
{
   int channel = luaL_checkint(L, 1);
   int value = luaL_checkint(L, 2);
   unsigned int gpio = lua_get_gpio_number(L, channel);
   
   if (gpio_direction[gpio] != OUTPUT)
     return luaL_error(L,  "The GPIO channel has not been set up as an OUTPUT");

   output_gpio(gpio, value);
   return 0;
}

// python function value = input(channel)
static int lua_input_gpio(lua_State* L)
{
   int channel = luaL_checkint(L, 1);
   unsigned int gpio = lua_get_gpio_number(L, channel);

   // check channel is set up as an input or output
   if (gpio_direction[gpio] != INPUT && gpio_direction[gpio] != OUTPUT)
      return luaL_error(L, "You must setup() the GPIO channel first");

   if (input_gpio(gpio)) {
      lua_pushnumber(L, HIGH);
   } else {
      lua_pushnumber(L, LOW);
   }
   return 1;
}  

static int lua_cleanup(lua_State* L)
{
   int i;
   int found = 0;


    // clean up any /sys/class exports
    //TODO event_cleanup();

    // set everything back to input
    for (i=0; i<54; i++)
    {
      if (gpio_direction[i] != -1)
      {
          setup_gpio(i, INPUT, PUD_OFF);
          gpio_direction[i] = -1;
      found = 1;
      }
    }
   
   // check if any channels set up - if not warn about misuse of GPIO.cleanup()
   if (!found && gpio_warnings)
      fprintf(stderr, "No channels have been set up yet - nothing to clean up!  Try cleaning up at the end of your program instead! Use GPIO.setwarnings(False) to disable warnings.");

   return 0;
}

static int lua_gpio_function(lua_State* L)
{
   int channel = luaL_checkint(L, 1);
   unsigned int gpio;
   int f;
   
   // run init_module if module not set up
   if (init_module() != SETUP_OK)
      luaL_error(L, "Failed initializing module");
   gpio = lua_get_gpio_number(L, channel);

   f = gpio_function(gpio);
   switch (f)
   {
      case 0 : f = INPUT;  break;
      case 1 : f = OUTPUT; break;
      case 4 : switch (gpio)
               {
                  case 0 :
                  case 1 : if (revision == 1) f = I2C; else f = MODE_UNKNOWN;
                           break;

                  case 2 :
                  case 3 : if (revision == 2) f = I2C; else f = MODE_UNKNOWN;
                           break;
                           
                  case 7 :
                  case 8 :
                  case 9 :
                  case 10 :
                  case 11 : f = SPI; break;

                  case 14 :
                  case 15 : f = SERIAL; break;

                  default : f = MODE_UNKNOWN; break;
               }
               break;

      case 5 : if (gpio == 18) f = PWM; else f = MODE_UNKNOWN;
               break;

      default : f = MODE_UNKNOWN; break;

   }
   lua_pushnumber(L, f);
   return 1;
}

// python method PWM.__init__(self, channel, frequency)
static int lua_pwm_init(lua_State* L)
{
    int channel = luaL_checkint(L, 1);
    float frequency = (float)luaL_checknumber(L, 2);
    PWMObject *self = lua_newuserdata(L, sizeof(PWMObject));
    
    if (self == NULL) 
        return luaL_error(L, "Failed allocating userdata, out of memory?");
    
    // convert channel to gpio
    self->gpio = lua_get_gpio_number(L, channel);

    // ensure channel set as output
    if (gpio_direction[self->gpio] != OUTPUT)
        return luaL_error(L, "You must setup() the GPIO channel as an output first");

    if (frequency <= 0.0)
        return luaL_error(L, "frequency must be greater than 0.0");

    self->freq = frequency;

    pwm_set_frequency(self->gpio, self->freq);
    
    // Attach meta table with shutdown method; __GC
    lua_getfield(L, LUA_REGISTRYINDEX, PWM_MT_NAME);
    lua_setmetatable(L, -2);
    return 1;
}

// python method PWM.ChangeDutyCycle(self, dutycycle)
static int lua_pwm_ChangeDutyCycle(lua_State* L)
{
    PWMObject *self = luaL_checkudata(L, 1, PWM_MT_NAME);
    float dutycycle = (float)luaL_checknumber(L, 2);

    if (dutycycle < 0.0 || dutycycle > 100.0)
        return luaL_error(L, "dutycycle must have a value from 0.0 to 100.0");

    self->dutycycle = dutycycle;
    pwm_set_duty_cycle(self->gpio, self->dutycycle);

    lua_settop(L, 1); // only return object itself
    return 1;
}

// python method PWM.start(self, dutycycle)
static int lua_pwm_start(lua_State* L)
{
    PWMObject *self = luaL_checkudata(L, 1, PWM_MT_NAME);
    
    lua_pwm_ChangeDutyCycle(L);
    pwm_start(self->gpio);

    lua_settop(L, 1); // only return object itself
    return 1;
}

// python method PWM. ChangeFrequency(self, frequency)
static int lua_pwm_ChangeFrequency(lua_State* L)
{
    PWMObject *self = luaL_checkudata(L, 1, PWM_MT_NAME);
    float frequency = (float)luaL_checknumber(L, 2);

    if (frequency <= 0.0)
        return luaL_error(L, "frequency must be greater than 0.0");

    self->freq = frequency;

    pwm_set_frequency(self->gpio, self->freq);
    lua_settop(L, 1); // only return object itself
    return 1;
}

// python function PWM.stop(self)
static int lua_pwm_stop(lua_State* L)
{
    PWMObject *self = luaL_checkudata(L, 1, PWM_MT_NAME);
    
    pwm_stop(self->gpio);
    lua_settop(L, 1); // only return object itself
    return 1;
}

// deallocation method
static int lua_pwm_dealloc(lua_State* L)
{
    lua_pwm_stop(L);
    return 0;
}


static int lua_nimpl(lua_State* L)
{
  return luaL_error(L, "Not implemented for Lua");
}

static const struct luaL_Reg gpio_lib[] = {
  { "setup", lua_setup_channel},
  { "cleanup", lua_cleanup},
  { "input", lua_input_gpio},
  { "output", lua_output_gpio},
  { "setmode", lua_setmode},  
  { "gpio_function", lua_gpio_function},
  { "setwarnings", lua_setwarnings},
  
  // interrupts and events
  { "add_event_detect", lua_nimpl},
  { "remove_event_detect", lua_nimpl},
  { "event_detected", lua_nimpl},
  { "add_event_callback", lua_nimpl},
  { "wait_for_edge", lua_nimpl},
  
  // PWM
  { "PWM", lua_pwm_init},
  { "start", lua_pwm_start},
  { "ChangeFrequency", lua_pwm_ChangeFrequency},
  { "ChangeDutyCycle", lua_pwm_ChangeDutyCycle},
  { "stop", lua_pwm_stop},

  {NULL, NULL}
};
  

int luaopen_GPIO (lua_State *L){
  
  
  int i, result;

  for (i=0; i<54; i++)
      gpio_direction[i] = -1;

   result = setup();
   if (result == SETUP_DEVMEM_FAIL)
   {
      return luaL_error(L, "GPIO module has no access to /dev/mem.  Try running as root!");
   } else if (result == SETUP_MALLOC_FAIL) {
      return luaL_error(L, "No memory!");
   } else if (result == SETUP_MMAP_FAIL) {
      return luaL_error(L,  "Mmap of GPIO registers failed");
   } 

  //Metatable for PWM objects
  luaL_newmetatable(L, PWM_MT_NAME);
  lua_pushstring(L, "__gc");
  lua_pushcfunction(L, lua_pwm_dealloc);

  //luaL_newlib(L, gpio_lib);
  luaL_register(L, "GPIO", gpio_lib);
  
  lua_pushnumber(L, HIGH);
  lua_setfield(L, -2, "HIGH");

  lua_pushnumber(L, LOW);
  lua_setfield(L, -2, "LOW");

  lua_pushnumber(L, OUTPUT);
  lua_setfield(L, -2, "OUT");

  lua_pushnumber(L, INPUT);
  lua_setfield(L, -2, "IN");
  
  lua_pushnumber(L, PWM);
  lua_setfield(L, -2, "PWM");

  lua_pushnumber(L, SERIAL);
  lua_setfield(L, -2, "SERIAL");

  lua_pushnumber(L, I2C);
  lua_setfield(L, -2, "I2C");

  lua_pushnumber(L, SPI);
  lua_setfield(L, -2, "SPI");

  lua_pushnumber(L, MODE_UNKNOWN);
  lua_setfield(L, -2, "UNKNOWN");

  lua_pushnumber(L, BOARD);
  lua_setfield(L, -2, "BOARD");

  lua_pushnumber(L, BCM);
  lua_setfield(L, -2, "BCM");

  lua_pushnumber(L, PUD_OFF + LUA_PUD_CONST_OFFSET);
  lua_setfield(L, -2, "PUD_OFF");

  lua_pushnumber(L, PUD_UP + LUA_PUD_CONST_OFFSET);
  lua_setfield(L, -2, "PUD_UP");

  lua_pushnumber(L, PUD_DOWN + LUA_PUD_CONST_OFFSET);
  lua_setfield(L, -2, "PUD_DOWN");

  lua_pushnumber(L, RISING_EDGE + LUA_EVENT_CONST_OFFSET);
  lua_setfield(L, -2, "RISING_EDGE");

  lua_pushnumber(L, FALLING_EDGE + LUA_EVENT_CONST_OFFSET);
  lua_setfield(L, -2, "FALLING_EDGE");

  lua_pushnumber(L, BOTH_EDGE + LUA_EVENT_CONST_OFFSET);
  lua_setfield(L, -2, "BOTH_EDGE");

  lua_pushstring(L, LUA_MODULE_VERSION);
  lua_setfield(L, -2, "VERSION");
  
  
   // detect board revision and set up accordingly
   revision = get_rpi_revision();
   if (revision == -1)
   {
      return luaL_error(L,  "This module can only be run on a Raspberry Pi!");
   } else if (revision == 1) {
      pin_to_gpio = &pin_to_gpio_rev1;
   } else { // assume revision 2
      pin_to_gpio = &pin_to_gpio_rev2;
   }

   lua_pushnumber(L, revision);
   lua_setfield(L, -2, "RPI_REVISION");

// TODO shouldn't there by a UDATA with a __GC method to detect Lua closing and do a cleanup?
// Does exiting Lua while PWM is running crash? it probably does....
  
  return 1;
}