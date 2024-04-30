/*************************************************************************************************************
* Non-blocking delay function, duration in milliseconds
**************************************************************************************************************/

#include "Wait.h"

void wait(uint16_t duration)
{
  unsigned long time_now = millis();   
  while((unsigned long)(millis() - time_now) < duration)     
  {
    yield();        // do nothing, but allow ESP8266 background processes to continue
  }
}

