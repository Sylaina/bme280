# bme280
Using BME280 sensor from Bosch at AVR mikrocontrollers like Atmega328p

This Library allowed you to use BME280 to sense temperature, pressure and humidity with AVR microcontroller like Atmega328.
For communication with the BME280 I'll use my own I2C library.

examble source-code:

/*************BME280 examble*************/
#include <stdlib.h>
#include "bme280.h"

int main(void){
  
  float temperature = 0.0;
  float pressure = 0.0;
  float humidity = 0.0;
  
  bme280_init();
  
  temperature = bme280_readTemperature(); // in °C
  pressure = bme280_readPressure(); // in hPa
  humidity = bme280_humidity; // in %
  
  for(;;){
    // main-loop
  }
  return 0; // never reached
}
