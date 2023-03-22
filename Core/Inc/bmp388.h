#ifndef __BMP388_H__
#define __BMP388_H__

#include "sensirion_config.h"
#include "sensirion_i2c.h"

//Code from CSDN
#define Concat_Bytes(msb, lsb) (((uint16_t)msb << 8) | (uint16_t)lsb)

//相关结构体定义，分别存放修正系数，未修正数据，已经修正的数据
struct bmp3_calib_data{
    uint16_t    par_t1;
    uint16_t    par_t2;
    int8_t      par_t3;
    int16_t     par_p1;
    int16_t     par_p2;
    int8_t      par_p3;
    int8_t      par_p4;
    uint16_t    par_p5;
    uint16_t    par_p6;
    int8_t      par_p7;
    int8_t      par_p8;
    int16_t     par_p9;
    int8_t      par_p10;
    int8_t      par_p11;
    int64_t     t_lin;
};

struct bmp3_uncomp_data {
    uint32_t pressure;
    uint32_t temperature;
};

struct bmp3_data {
    int64_t temperature;
    uint64_t pressure;
};


// BMP388 sensor port: PB3 - PB4
#define BMP388_PORT	GPIOB	

#define BMP388_SCL		GPIO_Pin_3
#define BMP388_SDA		GPIO_Pin_4	

#define BMP388_SCL_H          BMP388_PORT->BSRR = BMP388_SCL //set to 1
#define BMP388_SCL_L          BMP388_PORT->BRR  = BMP388_SCL //reset to 0
#define BMP388_SDA_H          BMP388_PORT->BSRR = BMP388_SDA
#define BMP388_SDA_L          BMP388_PORT->BRR  = BMP388_SDA

#define BMP388_SCL_Status     BMP388_PORT->IDR  & BMP388_SCL //IDR是查看引脚电平状态
#define BMP388_SDA_Status     BMP388_PORT->IDR  & BMP388_SDA


// The Chip parameters we use.
#define BMP388_Write_Address	0xEC
#define BMP388_Read_Address     0xED
#define BMP388_Address          0x76

#define BMP388_CHIP_ID          0x00
#define BMP388_ERR_REG          0x02
#define BMP388_STATUS           0x03
#define BMP388_PressAddress     0x04 //MSB->XLSB: 0x06 0x05 0x04
#define BMP388_TempAddress      0x07 //MSB->XLSB: 0x09 0x08 0x07
#define BMP388_Sensor_Time      0x0C //MSB->XLSB: 0x0E 0x0D 0x0C
#define BMP388_INT_STATUS       0x11
#define BMP388_INT_CTRL         0x19
#define BMP388_PWR_CTRL         0x1B
#define BMP388_OSR              0x1C
#define BMP388_ODR              0x1D
#define BMP388_CONFIG           0x1F
#define BMP388_CMD              0x7E









// I2C initial 
void BMP388_I2C_GPIO_Configuration(void);

void I2C5_delay(void);

int  I2C5_Start(void);
void I2C5_Stop(void);

void I2C5_SDA_IN(void);
void I2C5_SDA_OUT(void);

static void I2C5_Ack(void);
static void I2C5_NoAck(void);
uint8_t I2C5_GetAck(void);

void    I2C5_WriteByte(uint8_t Data);
uint8_t I2C5_ReadByte(uint8_t ack);

int BMP388_I2C_WriteByte(uint8_t Reg_Addr, uint8_t Data);
int BMP388_I2C_WriteBuffer(uint8_t Reg_Addr, uint8_t *pBuffer, uint8_t NumByteToWrite);

int BMP388_I2C_ReadByte(uint8_t Reg_Addr, uint8_t *Data);
int BMP388_I2C_ReadBuffer(uint8_t Reg_Addr, uint8_t *pBuffer, uint8_t NumByteToRead);


void BMP388_Start(void);
uint32_t BMP388_GetData(char ch);
void BMP388_Caculate(void);

void BMP388_Get_CalibData(void);
void BMP388_Compensate_temperature(void);
void BMP388_Compensate_pressure(void);


#endif


