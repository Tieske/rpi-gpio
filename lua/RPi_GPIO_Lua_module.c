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
// Name for PWM objects metatable
#define PWM_MT_NAME "RPI-GPIO PWM MT"
// Name for callback table
#define RPI_CBT_NAME "RPI-GPIO CBT"


#include <errno.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "darksidesync_api.h"
#include "darksidesync_aux.h"

#include "c_gpio.h"
#include "event_gpio.h"
#include "cpuinfo.h"
#include "common.h"
#include "soft_pwm.h"
#include "sys/time.h"
#include "stdlib.h"

typedef struct
{
    unsigned int gpio;
    float freq;
    float dutycycle;
} PWMObject;

typedef struct
{
    unsigned int gpio;
    int cb_ref;
} dss_data;

struct lua_callback
{
   unsigned int gpio;
   int cb_ref;  // int value, key in the table named RPI_CB_NAME in registry
   unsigned long long lastcall;
   unsigned int bouncetime;
   struct lua_callback *next;
};

// start of linked list with callbacks
// TODO: static; hence lib can be used from only 1 Lua state!!!!!
static struct lua_callback *lua_callbacks = NULL;
static void* lua_dss_utilid = NULL;

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
  
// check a HIGH/LOW value on the position, using Lua boolean check
// as True, but ALSO accepts 0 as false
// ALso checks number of parameters
static int lua_get_high_low(lua_State* L, int index)
{
   int value;
   if (lua_gettop(L) < index)
      return luaL_error(L, "to little arguments, missing HIGH/LOW parameter");
   if (lua_isnumber(L, index)
   {
      value = luaL_checkint(L, index);
      if (value != 0) value = 1;
   }
   else
   {
      value = lua_toboolean(L, index);
   }
   return value
}

static int lua_setup_channel(lua_State *L)
{
   unsigned int gpio;
   int channel, direction;
   int pud = PUD_OFF + LUA_PUD_CONST_OFFSET;
   int initial = -1;
   int func;

//TODO check below must also check at least 1 param is on the stack
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
        initial=lua_get_high_low(L,4);
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
   int value = lua_get_high_low(L, 2);
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
      lua_pushboolean(L, 1);
   } else {
      lua_pushboolean(L, 0);
   }
   return 1;
}  

// DSS requests gpio to cancel
void dss_cancel(void* utilid)
{
   if (lua_dss_utilid != NULL)
   {
      DSS_shutdown(NULL, utilid);
      lua_dss_utilid = NULL;
   }
}

// removes all callbacks for the given gpio number
void remove_lua_callbacks(lua_State* L, unsigned int gpio)
{
   struct lua_callback *cb = lua_callbacks;
   struct lua_callback *temp;
   struct lua_callback *prev = NULL;

   lua_getfield(L, LUA_REGISTRYINDEX, RPI_CBT_NAME);
   // remove all lua callbacks for gpio
   while (cb != NULL)
   {
      if (cb->gpio == gpio)
      {
         luaL_unref(L, -2, cb->cb_ref);  // release cb function from table         
         if (prev == NULL)
            lua_callbacks = cb->next;
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
   lua_pop(L, 1);
}

static int lua_cleanup(lua_State* L)
{
   int i;
   int found = 0;

    // clean up any /sys/class exports
    event_cleanup();

    // set everything back to input
    for (i=0; i<54; i++)
    {
      if (gpio_direction[i] != -1)
      {
          setup_gpio(i, INPUT, PUD_OFF);
          gpio_direction[i] = -1;
          remove_lua_callbacks(L, (unsigned int)i);
          found = 1;
      }
    }
   
   // stop DSS
   dss_cancel(lua_dss_utilid);
   
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

// DSS decode function
static int dss_decode(lua_State *L, void* TheData, void* utilid)
{
   int result;
   dss_data *pData = TheData;
   if (L == NULL)
   {
      //discard, we're exiting
      result = 0;
   }
   else
   {
      // push our data to Lua
      lua_getfield(L, LUA_REGISTRYINDEX, RPI_CBT_NAME);   // get callback table
      lua_rawgeti(L, -1, pData->cb_ref);
      lua_pushinteger(L, (int)(pData->gpio));
      result = 2;  // 1 = lua CB function, 2 = channel
   }
   free(pData);
   return result;
}

// callback function execution
static void run_lua_callbacks(unsigned int gpio)
{
   struct lua_callback *cb = lua_callbacks;
   struct timeval tv_timenow;
   unsigned long long timenow;
   dss_data *pData;

   while (cb != NULL)
   {
      if (cb->gpio == gpio)
      {
         gettimeofday(&tv_timenow, NULL);
         timenow = tv_timenow.tv_sec*1E6 + tv_timenow.tv_usec;
         if (cb->bouncetime == 0 || timenow - cb->lastcall > cb->bouncetime*1000 || cb->lastcall == 0 || cb->lastcall > timenow) {
            if (lua_dss_utilid != NULL)
            {
               // create a copy of the data
               pData = malloc(sizeof(dss_data));
               if (pData != NULL)
               {
                  pData->cb_ref = cb->cb_ref;
                  pData->gpio = gpio;
                  DSS_deliver(lua_dss_utilid, &dss_decode, NULL, pData);
               }
               else
               {
                  // TODO malloc failed, stay silent or some error?
               }
            }
            else
            {
               // TODO we can't deliver, stay silent? or some error?
            }
         }
         cb->lastcall = timenow;
      }
      cb = cb->next;
   }
}

void add_lua_callback(lua_State* L, unsigned int gpio, unsigned int bouncetime, int cb_index)  //NOTE: params will not be checked!
{
   struct lua_callback *new_lua_cb;
   struct lua_callback *cb = lua_callbacks;

   if (lua_dss_utilid == NULL)  // check if DarkSideSync is available
   {
      DSS_initialize(L, &dss_cancel);     // will not return on error
      lua_dss_utilid = DSS_getutilid(L);   // get our id
   }
   
   // start by inserting the callback function in our callback table
   lua_getfield(L, LUA_REGISTRYINDEX, RPI_CBT_NAME);   // get callback table

   // add callback to py_callbacks list
   new_lua_cb = malloc(sizeof(struct lua_callback));
   if (new_lua_cb == NULL)
      luaL_error(L, "Cannot allocate memory for callback storage");

   lua_pushvalue(L, cb_index);                         // copy callback to top of stack
   new_lua_cb->cb_ref = luaL_ref(L, -2);               // store it and get its unique index
   new_lua_cb->gpio = gpio;
   new_lua_cb->lastcall = 0;
   new_lua_cb->bouncetime = bouncetime;
   new_lua_cb->next = NULL;
   if (lua_callbacks == NULL) {
      lua_callbacks = new_lua_cb;
   } else {
      // add to end of list
      while (cb->next != NULL)
         cb = cb->next;
      cb->next = new_lua_cb;
   }
   add_edge_callback(gpio, run_lua_callbacks);
   lua_pop(L, 1);   
}

// python function add_event_callback(gpio, callback, bouncetime=0)
static int lua_add_event_callback(lua_State* L)
{
//TODO  allow for named arguments (a table)
   unsigned int gpio = lua_get_gpio_number(L, luaL_checkint(L, 1));
   unsigned int bouncetime = 0;

   luaL_checktype(L, 2, LUA_TFUNCTION);

   if (lua_gettop(L) > 2) 
   {
      bouncetime = (unsigned int)luaL_checkint(L, 3);
      if (bouncetime < 0 || bouncetime > 60000)
         luaL_error(L, "Bouncetime must be a value from 0 to 60000");
   }

   // check channel is set up as an input
   if (gpio_direction[gpio] != INPUT)
      return luaL_error(L, "You must setup() the GPIO channel as an input first");

   if (!gpio_event_added(gpio))
      return luaL_error(L, "Add event detection using add_event_detect first before adding a callback");

   add_lua_callback(L, gpio, (unsigned int)bouncetime, 2);
   return 0;
}

// python function add_event_detect(gpio, edge, callback=None, bouncetime=0
static int lua_add_event_detect(lua_State* L)
{
//TODO  allow for named arguments (a table)
   unsigned int gpio = lua_get_gpio_number(L, luaL_checkint(L, 1));
   int edge = luaL_checkint(L, 2);
   int result;
   unsigned int bouncetime = 0;

   if (lua_gettop(L) > 2) 
   {
      if (!lua_isnil(L, 3))
         luaL_checktype(L, 3, LUA_TFUNCTION);
   }
   
   if (lua_gettop(L) > 3) 
   {
      bouncetime = (unsigned int)luaL_checkint(L, 4);
      if (bouncetime < 0 || bouncetime > 60000)
         luaL_error(L, "Bouncetime must be a value from 0 to 60000");
   }

   // check channel is set up as an input
   if (gpio_direction[gpio] != INPUT)
      return luaL_error(L, "You must setup() the GPIO channel as an input first");

   // is edge valid value
   edge -= LUA_EVENT_CONST_OFFSET;
   if (edge != RISING_EDGE && edge != FALLING_EDGE && edge != BOTH_EDGE)
      return luaL_error(L, "The edge must be set to RISING, FALLING or BOTH");

   if ((result = add_edge_detect(gpio, edge)) != 0)   // starts a thread
   {
      if (result == 1)
      {
         return luaL_error(L, "Edge detection already enabled for this GPIO channel");
      } else {
         return luaL_error(L, "Failed to add edge detection");
      }
   }

   if (!lua_isnil(L, 3))
      add_lua_callback(L, gpio, bouncetime, 3);

   return 0;
}

// python function remove_event_detect(gpio)
int lua_remove_event_detect(lua_State* L)
{
   unsigned int gpio = lua_get_gpio_number(L, luaL_checkint(L, 1));

   remove_lua_callbacks(L, gpio);
   remove_edge_detect(gpio);
   return 0;
}

// python function value = event_detected(channel)
static int lua_event_detected(lua_State* L)
{
   unsigned int gpio = lua_get_gpio_number(L, luaL_checkint(L, 1));

   if (event_detected(gpio))
      lua_pushboolean(L, 1);
   else
      lua_pushboolean(L, 0);
   return 1;
}

// python function py_wait_for_edge(gpio, edge)
static int lua_wait_for_edge(lua_State* L)
{
   unsigned int gpio = lua_get_gpio_number(L, luaL_checkint(L, 1));
   int edge = luaL_checkint(L, 2);
   int result;
   char error[30];

   // check channel is setup as an input
   if (gpio_direction[gpio] != INPUT)
      return luaL_error(L, "You must setup() the GPIO channel as an input first");

   // is edge a valid value?
   edge -= LUA_EVENT_CONST_OFFSET;
   if (edge != RISING_EDGE && edge != FALLING_EDGE && edge != BOTH_EDGE)
      return luaL_error(L, "The edge must be set to RISING, FALLING or BOTH");

   result = blocking_wait_for_edge(gpio, edge);

   if (result == 0) {
      return 0;
   } else if (result == 2) {
      return luaL_error(L, "Edge detection events already enabled for this GPIO channel");
   } else {
      sprintf(error, "Error #%d waiting for edge", result);
      return luaL_error(L, error);
   }
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
  { "wait_for_edge", lua_wait_for_edge},
  { "event_detected", lua_event_detected},
  { "add_event_detect", lua_add_event_detect},
  { "remove_event_detect", lua_remove_event_detect},
  { "add_event_callback", lua_add_event_callback},
  
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
  
  lua_pushboolean(L, 1);
  lua_setfield(L, -2, "HIGH");

  lua_pushboolean(L, 0);
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
  lua_setfield(L, -2, "RISING");

  lua_pushnumber(L, FALLING_EDGE + LUA_EVENT_CONST_OFFSET);
  lua_setfield(L, -2, "FALLING");

  lua_pushnumber(L, BOTH_EDGE + LUA_EVENT_CONST_OFFSET);
  lua_setfield(L, -2, "BOTH");

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

   lua_newtable(L);
   lua_setfield(L, LUA_REGISTRYINDEX, RPI_CBT_NAME); // create empty table for callback storage
   
// TODO shouldn't there by a UDATA with a __GC method to detect Lua closing and do a cleanup?
// Does exiting Lua while PWM or other threading is running crash? it probably does....
  
  return 1;
}