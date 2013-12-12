#include <stdio.h>
#include "c_gpio.h"

int main()
{
   int i;

   setup();
   for (i=0; i<53; i++) {
      printf("i=%d func=%d\n", i, gpio_function(i));
   }
   return 0;
}
