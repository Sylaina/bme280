// examble for use BME280 with Atmega328P

#include <stdlib.h>
#include "bme280.h"

int main(void){
  // variables for sensor values
  float temperature = 0.0;
  float pressure = 0.0;
  float humidity = 0.0;
  
  // init sensor
  bme280_init();
  // read values
  temperature = bme280_readTemperature(); // in °C
  pressure = bme280_readPressure()/100.0; // in mbar
  humidity = bme280_readHumidity(); // in %
  
  for(;;){
    // main-loop
  }
  return 0; // never reached
}
