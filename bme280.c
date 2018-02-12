//
//  bme280.c
//  i2c
//
//  Created by Michael Köhler on 09.10.17.
//
//

#include "bme280.h"
#include <math.h>       // for NAN

void bme280_init(void){
    if (bme280_read1Byte(BME280_REGISTER_CHIPID) == 0x60){
        // configure sensor
        i2c_start(BME_ADDR);
#ifdef BMP280
        i2c_byte(BME280_REGISTER_CONTROLHUMID);
        i2c_byte(BME280_HUM_CONFIG);
#endif
        i2c_byte(BME280_REGISTER_CONFIG);
        i2c_byte(BME280_CONFIG);
        
        i2c_byte(BME280_REGISTER_CONTROL);
        i2c_byte((BME280_TEMP_CONFIG << 5)|(BME280_PRESS_CONFIG << 2)|(BME280_MODE_CONFIG));
        
        i2c_stop();
        
		// read coefficients
        bme280_readCoefficients();
    }
}

float bme280_readTemperature(void){
    uint32_t adc_T = bme280_read3Byte(BME280_REGISTER_TEMPDATA);
    if (adc_T == 0x800000) // value in case pressure measurement was disabled
        return NAN;
    int32_t var1, var2;
    
    adc_T >>= 4;
    
    var1  = ((((adc_T>>3) - ((int32_t)_bme280_calib.dig_T1 <<1))) *
             ((int32_t)_bme280_calib.dig_T2)) >> 11;
    
    var2  = (((((adc_T>>4) - ((int32_t)_bme280_calib.dig_T1)) *
               ((adc_T>>4) - ((int32_t)_bme280_calib.dig_T1))) >> 12) *
             ((int32_t)_bme280_calib.dig_T3)) >> 14;
    
    t_fine = var1 + var2;
    
    float T  = (t_fine * 5 + 128) >> 8;
    
    return T/100;
}

float bme280_readPressure(void){
    int64_t var1, var2, p;
    
    bme280_readTemperature(); // must be done first to get t_fine
    
    int32_t adc_P = bme280_read3Byte(BME280_REGISTER_PRESSUREDATA);
    if (adc_P == 0x800000) // value in case pressure measurement was disabled
        return NAN;
    adc_P >>= 4;
    
    var1 = ((int64_t)t_fine) - 128000ul;
    var2 = var1 * var1 * (int64_t)_bme280_calib.dig_P6;
    var2 = var2 + ((var1*(int64_t)_bme280_calib.dig_P5)<<17);
    var2 = var2 + (((int64_t)_bme280_calib.dig_P4)<<35);
    var1 = ((var1 * var1 * (int64_t)_bme280_calib.dig_P3)>>8) +
    ((var1 * (int64_t)_bme280_calib.dig_P2)<<12);
    var1 = (((((int64_t)1)<<47)+var1))*((int64_t)_bme280_calib.dig_P1)>>33;
    
    if (var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576ul - adc_P;
    p = (((p<<31) - var2)*3125ul) / var1;
    var1 = (((int64_t)_bme280_calib.dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 = (((int64_t)_bme280_calib.dig_P8) * p) >> 19;
    
    p = ((p + var1 + var2) >> 8) + (((int64_t)_bme280_calib.dig_P7)<<4);
    return (float)p/256ul;
}
#ifdef BME280
float bme280_readHumidity(void){
    bme280_readTemperature(); // must be done first to get t_fine
    
    int32_t adc_H = bme280_read2Byte(BME280_REGISTER_HUMIDDATA);
    if (adc_H == 0x8000) // value in case humidity measurement was disabled
        return NAN;
    int32_t v_x1_u32r;
    
    v_x1_u32r = (t_fine - ((int32_t)76800));
    
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)_bme280_calib.dig_H4) << 20) -
                    (((int32_t)_bme280_calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)_bme280_calib.dig_H6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)_bme280_calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)_bme280_calib.dig_H2) + 8192) >> 14));
    
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                               ((int32_t)_bme280_calib.dig_H1)) >> 4));
    
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    float h = (v_x1_u32r>>12);
    return  h / 1024.0;
}
#endif
float bme280_readAltitude(float seaLevel){
    // seaLevel at hPa (mBar), equation from datasheet BMP180, page 16
    
    float atmospheric = bme280_readPressure() / 100.0F;
    return 44330.0 * (1.0 - pow(atmospheric / seaLevel, 0.1903));
}

uint8_t bme280_read1Byte(uint8_t addr){
    uint8_t value;
    i2c_start(BME_ADDR);
    i2c_byte(addr);
    i2c_stop();
    i2c_start(BME_ADDR|0x01);
    value = i2c_readNAck();
    i2c_stop();
    return value;
}
uint16_t bme280_read2Byte(uint8_t addr){
    uint16_t value;
    i2c_start(BME_ADDR);
    i2c_byte(addr);
    i2c_stop();
    i2c_start(BME_ADDR|0x01);
    value = i2c_readAck();
    value <<= 8;
    value |= i2c_readNAck();
    i2c_stop();
    return value;
}
uint32_t bme280_read3Byte(uint8_t addr){
    uint32_t value;
    i2c_start(BME_ADDR);
    i2c_byte(addr);
    i2c_stop();
    i2c_start(BME_ADDR|0x01);
    value = i2c_readAck();
    value <<= 8;
    value |= i2c_readAck();
    value <<= 8;
    value |= i2c_readNAck();
    i2c_stop();
    return value;
}
uint16_t read16_LE(uint8_t reg)
{
    uint16_t temp = bme280_read2Byte(reg);
    return (temp >> 8) | (temp << 8);
    
}

int16_t readS16(uint8_t reg)
{
    return (int16_t)bme280_read2Byte(reg);
    
}

int16_t readS16_LE(uint8_t reg)
{
    return (int16_t)read16_LE(reg);
    
}


void bme280_readCoefficients(void)
{
    _bme280_calib.dig_T1 = read16_LE(BME280_REGISTER_DIG_T1);
    _bme280_calib.dig_T2 = readS16_LE(BME280_REGISTER_DIG_T2);
    _bme280_calib.dig_T3 = readS16_LE(BME280_REGISTER_DIG_T3);
    
    _bme280_calib.dig_P1 = read16_LE(BME280_REGISTER_DIG_P1);
    _bme280_calib.dig_P2 = readS16_LE(BME280_REGISTER_DIG_P2);
    _bme280_calib.dig_P3 = readS16_LE(BME280_REGISTER_DIG_P3);
    _bme280_calib.dig_P4 = readS16_LE(BME280_REGISTER_DIG_P4);
    _bme280_calib.dig_P5 = readS16_LE(BME280_REGISTER_DIG_P5);
    _bme280_calib.dig_P6 = readS16_LE(BME280_REGISTER_DIG_P6);
    _bme280_calib.dig_P7 = readS16_LE(BME280_REGISTER_DIG_P7);
    _bme280_calib.dig_P8 = readS16_LE(BME280_REGISTER_DIG_P8);
    _bme280_calib.dig_P9 = readS16_LE(BME280_REGISTER_DIG_P9);
#ifdef BME280
    _bme280_calib.dig_H1 = bme280_read1Byte(BME280_REGISTER_DIG_H1);
    _bme280_calib.dig_H2 = readS16_LE(BME280_REGISTER_DIG_H2);
    _bme280_calib.dig_H3 = bme280_read1Byte(BME280_REGISTER_DIG_H3);
    _bme280_calib.dig_H4 = (bme280_read1Byte(BME280_REGISTER_DIG_H4) << 4) | (bme280_read1Byte(BME280_REGISTER_DIG_H4+1) & 0xF);
    _bme280_calib.dig_H5 = (bme280_read1Byte(BME280_REGISTER_DIG_H5+1) << 4) | (bme280_read1Byte(BME280_REGISTER_DIG_H5) >> 4);
    _bme280_calib.dig_H6 = (int8_t)bme280_read1Byte(BME280_REGISTER_DIG_H6);
#endif
}
