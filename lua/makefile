
RPI_GPIO_PYTHON_SRC_DIR=../source/

LUA_LIBS=$(shell pkg-config --libs lua5.1)

GPIO_CORE_OBJECTS=c_gpio.o cpuinfo.o
# event_gpio.o soft_pwm.o

ALL_OBJECTS=RPi_GPIO_Lua_module.o ${GPIO_CORE_OBJECTS}

GPIO.so: ${ALL_OBJECTS} 
	ld -shared -soname GPIO.so -o GPIO.so  ${LUA_LIBS} ${ALL_OBJECTS}

RPi_GPIO_Lua_module.o:
	gcc -fPIC -c  RPi_GPIO_Lua_module.c -I ${RPI_GPIO_PYTHON_SRC_DIR}

c_gpio.o:
	gcc -fPIC -c  ${RPI_GPIO_PYTHON_SRC_DIR}c_gpio.c

cpuinfo.o:
	gcc -fPIC -c  ${RPI_GPIO_PYTHON_SRC_DIR}cpuinfo.c

event_gpio.o:
	gcc -fPIC -c  ${RPI_GPIO_PYTHON_SRC_DIR}event_gpio.c


soft_pwm.o:
	gcc -fPIC -c  ${RPI_GPIO_PYTHON_SRC_DIR}soft_pwm.c

clean:
	rm -rf *.o *.so