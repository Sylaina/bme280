//
//  bme280.h
//  i2c
//
//  Created by Michael Köhler on 09.10.17.
//
//

#ifndef bme280_h
#define bme280_h


#define OVER_0x		0x00 //skipped, output set to 0x80000
#define OVER_1x		0x01
#define OVER_2x		0x02
#define OVER_4x		0x03
#define OVER_8x		0x04
#define OVER_16x	0x05

#define BME280_FORCED_MODE 0x01
#define BME280_NORMAL_MODE 0x03
#define BME280_SLEEP_MODE 0x00

#define BME280_STANDBY_500us	0x00
#define BME280_STANDBY_62500us	0x01
#define BME280_STANDBY_125ms	0x02
#define BME280_STANDBY_250ms	0x03
#define BME280_STANDBY_500ms	0x04
#define BME280_STANDBY_1000ms	0x05
#define BME280_STANDBY_10ms		0x06
#define BME280_STANDBY_20ms		0x07

#define BME280_IIR_OFF	0x00
#define BME280_IIR_2x	0x01
#define BME280_IIR_4x	0x02
#define BME280_IIR_8x	0x03
#define BME280_IIR_16x	0x04

#define BME280_SPI_OFF	0x00
#define BME280_SPI_ON	0x01

/* TODO: configure Sensor */

// i2c adress
#define BME_ADDR 0xEE
// for BME280 define
#define BME280
// for BMP280 (same as BME280 without humitity sensor) uncomment #define above

/****** settings *******/
// default: Standby-Time = 250ms, IIR-Filter = 16x, SPI disable, Oversampling for all Sensors = 16x, Normal Mode

// Standby-Time, IIR-Filter, SPI Disable
#define BME280_CONFIG		(BME280_STANDBY_250ms << 5)|(BME280_IIR_8x << 2)|(BME280_SPI_OFF)
// Temperatur-Sensor
#define BME280_TEMP_CONFIG	OVER_16x
// Pressure-Sensor
#define BME280_PRESS_CONFIG	OVER_16x

#ifdef BME280
// Humitity-Sensor
#define BME280_HUM_CONFIG	OVER_16x
#elif
#define BME280_HUM_CONFIG	OVER_0x	//no humitity-sensor, skipped
#endif
// Mode
#define BME280_MODE_CONFIG	BME280_NORMAL_MODE

#include <stdio.h>
#include "i2c.h"

typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
#ifdef BME280
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
#endif
} bme280_calib_data;

enum
{
    BME280_REGISTER_DIG_T1              = 0x88,
    BME280_REGISTER_DIG_T2              = 0x8A,
    BME280_REGISTER_DIG_T3              = 0x8C,
    
    BME280_REGISTER_DIG_P1              = 0x8E,
    BME280_REGISTER_DIG_P2              = 0x90,
    BME280_REGISTER_DIG_P3              = 0x92,
    BME280_REGISTER_DIG_P4              = 0x94,
    BME280_REGISTER_DIG_P5              = 0x96,
    BME280_REGISTER_DIG_P6              = 0x98,
    BME280_REGISTER_DIG_P7              = 0x9A,
    BME280_REGISTER_DIG_P8              = 0x9C,
    BME280_REGISTER_DIG_P9              = 0x9E,
#ifdef BME280
    BME280_REGISTER_DIG_H1              = 0xA1,
    BME280_REGISTER_DIG_H2              = 0xE1,
    BME280_REGISTER_DIG_H3              = 0xE3,
    BME280_REGISTER_DIG_H4              = 0xE4,
    BME280_REGISTER_DIG_H5              = 0xE5,
    BME280_REGISTER_DIG_H6              = 0xE7,
#endif
    BME280_REGISTER_CHIPID             = 0xD0,
    BME280_REGISTER_VERSION            = 0xD1,
    BME280_REGISTER_SOFTRESET          = 0xE0,
    
    BME280_REGISTER_CAL26              = 0xE1,  // R calibration stored in 0xE1-0xF0
    
    BME280_REGISTER_CONTROLHUMID       = 0xF2,
    BME280_REGISTER_CONTROL            = 0xF4,
    BME280_REGISTER_CONFIG             = 0xF5,
    BME280_REGISTER_PRESSUREDATA       = 0xF7,
    BME280_REGISTER_TEMPDATA           = 0xFA,
#ifdef BME280
    BME280_REGISTER_HUMIDDATA          = 0xFD,
#endif
};

void bme280_init(void);

float bme280_readTemperature(void);
float bme280_readPressure(void);
#ifdef BME280
float bme280_readHumiditiy(void);
#endif
float bme280_readAltitude(float seaLevel);

uint8_t bme280_read1Byte(uint8_t addr);
uint16_t bme280_read2Byte(uint8_t addr);
uint32_t bme280_read3Byte(uint8_t addr);

void bme280_readCoefficients(void);

uint16_t read16_LE(uint8_t reg);
int16_t readS16(uint8_t reg);
int16_t readS16_LE(uint8_t reg);

volatile uint32_t t_fine;
volatile bme280_calib_data _bme280_calib;
#endif /* bme280_h */
