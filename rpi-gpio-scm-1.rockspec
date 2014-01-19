package = "rpi-gpio"
version = "scm-1"

local tag = "master"

source = {
    url = "http://github.com/Tieske/rpi-gpio/archive/"..tag..".zip",
    dir = "rpi-gpio-master",
}
description = {
   summary = "Lua module to control the GPIO on a Raspberry Pi",
   detailed = [[
      This package provides a Lua module to control the GPIO on a Raspberry Pi.
      The main functionality is provided by the RPi.GPIO Python Module of Ben 
      Croston, and the Lua binding was continued upon the work of Andre Simon.
   ]],
   homepage = "https://github.com/Tieske/rpi-gpio",
   license = "MIT"
}
dependencies = {
   "lua >= 5.1, < 5.2"
}
build = {
  type = "builtin",
  modules = {
    ["GPIO"] = {
      sources = {
        "lua/RPi_GPIO_Lua_module.c",
        "source/c_gpio.c",
        "source/cpuinfo.c",
        "source/event_gpio.c",
        "source/soft_pwm.c",
      },
      libraries = {
        "pthread"
      },
      incdirs = {
        "source",
      },
    },
  },
}
