//
//  bme280.c
//  i2c
//
//  Created by Michael Köhler on 09.10.17.
//
//

#include "bme280.h"
#include <math.h>       // for NAN & pow()
#include <util/delay.h> // needed delay after softreset

/**********************************************
 Public Function: bme280_init
 
 Purpose: Initialise sensor
 
 Input Parameter: sensor
 
 Return Value: uint8_t
 - Value 0x00 means BME280 detected
 - Value 0x01 means BMP280 detected
 - Value 0xff means sensor unknown or argue out of range
 **********************************************/
uint8_t bme280_init(uint8_t sensor){
    if ((sensor > SENSORS-1) && (sensor < 0)) { // argue sensor out of range
        return 0xff;
    }
    
    uint8_t returnValue = 0xff;
    switch (bme280_read1Byte(BME280_REGISTER_CHIPID, sensor)){
        case 0x60:
        // BME280 connected
        returnValue = 0x00;
        
        // init softreset of sensor
        i2c_start(0xec|((1-sensor)<<1));
        i2c_byte(BME280_REGISTER_SOFTRESET);
        i2c_byte(0xB6);
        i2c_stop();
        // wait for finished softreset
        _delay_ms(10);
        
        // start to write config via I2C
        i2c_start(0xec|((1-sensor)<<1));
        
        // write config for humidity
        i2c_byte(BME280_REGISTER_CONTROLHUMID);
        i2c_byte(BME280_HUM_CONFIG);
        break;
        case 0x58:
        // BMP280 connected
        returnValue = 0x01;
        
        // init softreset of sensor
        i2c_start(0xec|((1-sensor)<<1));
        i2c_byte(BME280_REGISTER_SOFTRESET);
        i2c_byte(0xB6);
        i2c_stop();
        
        // wait for finished softreset
        _delay_ms(10);
        
        // start to write config via I2C
        i2c_start(0xec|((1-sensor)<<1));
        break;
        default:
        // wrong chip-id, abort init
        return 0xff;
    }
    
    // write config for filter, standby-time and SPI-Mode (SPI off)
    i2c_byte(BME280_REGISTER_CONFIG);
    i2c_byte(BME280_CONFIG);
    
    // write config for pressure, temperture and sensor-mode
    i2c_byte(BME280_REGISTER_CONTROL);
    i2c_byte((BME280_TEMP_CONFIG << 5)|(BME280_PRESS_CONFIG << 2)|(BME280_MODE_CONFIG));
    
    i2c_stop();
    
    // wait for adjust configs
    _delay_ms(100);
    
    // read coefficients
    bme280_readCoefficients(sensor);
    return returnValue;
}

/**********************************************
 Public Function: bme280_readTemperature
 
 Purpose: Read temperature
 
 Input Parameter: uint8_t sensor: choose sensor on I2C
 
 Return Value: float
 - temperature in celsius
 - Value NAN means measurement disable or argue out of range
 **********************************************/
float bme280_readTemperature(uint8_t sensor){
    if ((sensor > SENSORS-1) && (sensor < 0)) { // argue sensor out of range
        return NAN;
    }
    
    uint32_t adc_T = bme280_read3Byte(BME280_REGISTER_TEMPDATA, sensor);
    
    if (adc_T == 0x800000) // value in case temperature measurement was disabled
        return NAN;
    int32_t var1, var2;
    
    adc_T >>= 4;
    
    var1  = ((((adc_T>>3) - ((int32_t)_bme280_calib[sensor].dig_T1 <<1))) *
             ((int32_t)_bme280_calib[sensor].dig_T2)) >> 11;
    
    var2  = (((((adc_T>>4) - ((int32_t)_bme280_calib[sensor].dig_T1)) *
               ((adc_T>>4) - ((int32_t)_bme280_calib[sensor].dig_T1))) >> 12) *
             ((int32_t)_bme280_calib[sensor].dig_T3)) >> 14;
    
    t_fine[sensor] = var1 + var2;
    
    float T  = (t_fine[sensor] * 5 + 128) >> 8;
    
    return T/100;
}

/**********************************************
 Public Function: bme280_readPressure
 
 Purpose: Read pressure
 
 Input Parameter: uint8_t sensor: choose sensor on I2C
 
 Return Value: float
 - pressure in hPa
 - Value NAN means measurement disable or argue out of range
 **********************************************/
float bme280_readPressure(uint8_t sensor){
    if ((sensor > SENSORS-1) && (sensor < 0)) { // argue sensor out of range
        return NAN;
    }
    
    int64_t var1, var2, p;
    
    bme280_readTemperature(sensor); // must be done first to get t_fine
    
    int32_t adc_P = bme280_read3Byte(BME280_REGISTER_PRESSUREDATA, sensor);
    if (adc_P == 0x800000) // value in case pressure measurement was disabled
        return NAN;
    adc_P >>= 4;
    
    var1 = ((int64_t)t_fine[sensor]) - 128000ul;
    var2 = var1 * var1 * (int64_t)_bme280_calib[sensor].dig_P6;
    var2 = var2 + ((var1*(int64_t)_bme280_calib[sensor].dig_P5)<<17);
    var2 = var2 + (((int64_t)_bme280_calib[sensor].dig_P4)<<35);
    var1 = ((var1 * var1 * (int64_t)_bme280_calib[sensor].dig_P3)>>8) +
    ((var1 * (int64_t)_bme280_calib[sensor].dig_P2)<<12);
    var1 = (((((int64_t)1)<<47)+var1))*((int64_t)_bme280_calib[sensor].dig_P1)>>33;
    
    if (var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576ul - adc_P;
    p = (((p<<31) - var2)*3125ul) / var1;
    var1 = (((int64_t)_bme280_calib[sensor].dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 = (((int64_t)_bme280_calib[sensor].dig_P8) * p) >> 19;
    
    p = ((p + var1 + var2) >> 8) + (((int64_t)_bme280_calib[sensor].dig_P7)<<4);
    return (float)p/256ul;
}

/**********************************************
 Public Function: bme280_readHumidity
 
 Purpose: Read humidity
 
 Input Parameter: uint8_t sensor: choose sensor on I2C
 
 Return Value: float
 - humidity in %
 - Value NAN means measurement disable or argue out of range
 **********************************************/
float bme280_readHumidity(uint8_t sensor){
    if ((sensor > SENSORS-1) && (sensor < 0)) { // argue sensor out of range
        return NAN;
    }
    
    if(bme280_read1Byte(BME280_REGISTER_CHIPID, sensor)!=0x60) // sensor isn't a BME280 with humidity unit
        return NAN;
    
    bme280_readTemperature(sensor); // must be done first to get t_fine
    
    int32_t adc_H = bme280_read2Byte(BME280_REGISTER_HUMIDDATA, sensor);
    if (adc_H == 0x8000) // value in case humidity measurement was disabled
        return NAN;
    int32_t v_x1_u32r;
    
    v_x1_u32r = (t_fine[sensor] - ((int32_t)76800));
    
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)_bme280_calib[sensor].dig_H4) << 20) -
                    (((int32_t)_bme280_calib[sensor].dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)_bme280_calib[sensor].dig_H6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)_bme280_calib[sensor].dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)_bme280_calib[sensor].dig_H2) + 8192) >> 14));
    
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                               ((int32_t)_bme280_calib[sensor].dig_H1)) >> 4));
    
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    float h = (v_x1_u32r>>12);
    return  h / 1024;
}

/**********************************************
 Public Function: bme280_readHumidity
 
 Purpose: Read humidity
 
 Input Parameter: uint8_t sensor: choose sensor on I2C
                  float seaLevel: pressure at sealevel
 
 Return Value: float
 - level of sensor over sealevel in meter
 - Value NAN means measurement disable or argue out of range
 **********************************************/
float bme280_readAltitude(float seaLevel, uint8_t sensor){
    if ((sensor > SENSORS-1) && (sensor < 0)) { // argue sensor out of range
        return NAN;
    }
    
    // seaLevel at hPa (mBar), equation from datasheet BMP180, page 16
    
    float atmospheric = bme280_readPressure(sensor) / 100.0F;
    return 44330.0 * (1.0 - pow(atmospheric / seaLevel, 0.1903));
}

uint8_t bme280_read1Byte(uint8_t addr, uint8_t sensor){
    uint8_t value;
    i2c_start(0xec|((1-sensor)<<1));
    i2c_byte(addr);
    i2c_stop();
    i2c_start((0xec|((1-sensor)<<1))|0x01);
    value = i2c_readNAck();
    i2c_stop();
    return value;
}
uint16_t bme280_read2Byte(uint8_t addr, uint8_t sensor){
    uint16_t value;
    i2c_start(0xec|((1-sensor)<<1));
    i2c_byte(addr);
    i2c_stop();
    i2c_start((0xec|((1-sensor)<<1))|0x01);
    value = i2c_readAck();
    value <<= 8;
    value |= i2c_readNAck();
    i2c_stop();
    return value;
}
uint32_t bme280_read3Byte(uint8_t addr, uint8_t sensor){
    uint32_t value;
    i2c_start(0xec|((1-sensor)<<1));
    i2c_byte(addr);
    i2c_stop();
    i2c_start((0xec|((1-sensor)<<1))|0x01);
    value = i2c_readAck();
    value <<= 8;
    value |= i2c_readAck();
    value <<= 8;
    value |= i2c_readNAck();
    i2c_stop();
    return value;
}
uint16_t read16_LE(uint8_t reg, uint8_t sensor)
{
    uint16_t temp = bme280_read2Byte(reg, sensor);
    return (temp >> 8) | (temp << 8);
    
}

int16_t readS16(uint8_t reg, uint8_t sensor)
{
    return (int16_t)bme280_read2Byte(reg, sensor);
    
}

int16_t readS16_LE(uint8_t reg, uint8_t sensor)
{
    return (int16_t)read16_LE(reg, sensor);
    
}


void bme280_readCoefficients(uint8_t sensor)
{
    _bme280_calib[sensor].dig_T1 = read16_LE(BME280_REGISTER_DIG_T1, sensor);
    _bme280_calib[sensor].dig_T2 = readS16_LE(BME280_REGISTER_DIG_T2, sensor);
    _bme280_calib[sensor].dig_T3 = readS16_LE(BME280_REGISTER_DIG_T3, sensor);
    
    _bme280_calib[sensor].dig_P1 = read16_LE(BME280_REGISTER_DIG_P1, sensor);
    _bme280_calib[sensor].dig_P2 = readS16_LE(BME280_REGISTER_DIG_P2, sensor);
    _bme280_calib[sensor].dig_P3 = readS16_LE(BME280_REGISTER_DIG_P3, sensor);
    _bme280_calib[sensor].dig_P4 = readS16_LE(BME280_REGISTER_DIG_P4, sensor);
    _bme280_calib[sensor].dig_P5 = readS16_LE(BME280_REGISTER_DIG_P5, sensor);
    _bme280_calib[sensor].dig_P6 = readS16_LE(BME280_REGISTER_DIG_P6, sensor);
    _bme280_calib[sensor].dig_P7 = readS16_LE(BME280_REGISTER_DIG_P7, sensor);
    _bme280_calib[sensor].dig_P8 = readS16_LE(BME280_REGISTER_DIG_P8, sensor);
    _bme280_calib[sensor].dig_P9 = readS16_LE(BME280_REGISTER_DIG_P9, sensor);
    if(bme280_read1Byte(BME280_REGISTER_CHIPID, sensor)==0x60){
        // sensor is a BME280 with humidity unit
        _bme280_calib[sensor].dig_H1 = bme280_read1Byte(BME280_REGISTER_DIG_H1, sensor);
        _bme280_calib[sensor].dig_H2 = readS16_LE(BME280_REGISTER_DIG_H2, sensor);
        _bme280_calib[sensor].dig_H3 = bme280_read1Byte(BME280_REGISTER_DIG_H3, sensor);
        _bme280_calib[sensor].dig_H4 = (bme280_read1Byte(BME280_REGISTER_DIG_H4, sensor) << 4) | (bme280_read1Byte(BME280_REGISTER_DIG_H4+1, sensor) & 0xF);
        _bme280_calib[sensor].dig_H5 = (bme280_read1Byte(BME280_REGISTER_DIG_H5+1, sensor) << 4) | (bme280_read1Byte(BME280_REGISTER_DIG_H5, sensor) >> 4);
        _bme280_calib[sensor].dig_H6 = (int8_t)bme280_read1Byte(BME280_REGISTER_DIG_H6, sensor);
    }
}
