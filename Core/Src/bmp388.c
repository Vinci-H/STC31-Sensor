#include "math.h"
#include "bmp388.h"

#include "sensirion_config.h"
#include "sensirion_common.h"
#include "sensirion_i2c.h"
#include "sensirion_i2c_hal.h"
//#include "stm32f1xx_hal_i2c.h"
#include "i2c.h"

struct bmp3_calib_data  calib_data;
struct bmp3_uncomp_data uncomp_data;
struct bmp3_data        comp_data;


/********************************************************************************/
/**
 * @brief  This function through I2C Write 1 Byte in Reg_Addr.
 * @param  Reg_Addr: Register Address of the Slave device.
 * @param  Data: 8bit
 * @retval 1 or 0
 */
int BMP388_I2C_WriteByte(uint8_t Reg_Addr, uint8_t sData)
{

    uint8_t sendBuffer[2];
    sendBuffer[0]=Reg_Addr;
    sendBuffer[1]=sData;

    sensirion_i2c_hal_write(BMP388_Address, sendBuffer, 2);

    return 1;
}


/**
 * @brief This function through I2C Write some Bytes in Reg_Addr.
 * @param Reg_Addr: Register Address of the Slave device.
 * @param pBuffer: the data to write
 * @param NumByteToWrite
 * @return int
 */
int BMP388_I2C_WriteBuffer(uint8_t Reg_Addr, uint8_t *pBuffer, uint8_t NumByteToWrite)
{

    sensirion_i2c_hal_write(BMP388_Address, 0,1);
    sensirion_i2c_hal_write(Reg_Addr, pBuffer,NumByteToWrite);

	return 1;
}


/**
 * @brief  This function through I2C Read 1 Byte from Reg_Addr.
 * @param  Reg_Addr: Register Address of the Slave device.
 * @param  Data: 8bit
 * @retval 1 or 0
 */
int BMP388_I2C_ReadByte(uint8_t Reg_Addr, uint8_t *Data)
{

    sensirion_i2c_hal_write(BMP388_Address, &Reg_Addr,1);
    sensirion_i2c_hal_read(BMP388_Address,Data,1);

	return 1;
}

/**
 * @brief  This function throu;;gh I2C Read NumByte from Reg_Addr.
 * @param  Reg_Addr: Register Address of the Slave device.
 * @param  pBuffer: the data to read
 * @param  NumByteToRead:
 * @retval None
 */
int BMP388_I2C_ReadBuffer(uint8_t Reg_Addr, uint8_t *pBuffer, uint8_t NumByteToRead){

    sensirion_i2c_hal_write(BMP388_Address, &Reg_Addr,1);
    sensirion_i2c_hal_read(BMP388_Address,pBuffer,NumByteToRead);

	return 1;
}


/////////////////////////////           BMP388 Coding       //////////////////////////
uint32_t BMP388_GetData(char ch)
{

	uint8_t status;
    BMP388_I2C_ReadByte(BMP388_STATUS, &status);

    while(!((status&0x40) && (status&0x20)))  //查询数据是否准备完毕  0 1 1 0 0000
	{
        sensirion_i2c_hal_sleep_usec(1000);
        BMP388_I2C_ReadByte(BMP388_STATUS, &status);
	}

	//注意data的MSB 和 LSB方向是常规相反的
    uint8_t buf1[3];
    BMP388_I2C_ReadBuffer(BMP388_PressAddress, buf1, 3); //16bit for press. but not 16 bit data
    uncomp_data.pressure = buf1[2] << 8;
    uncomp_data.pressure |= buf1[1];
    uncomp_data.pressure <<= 8;
    uncomp_data.pressure |= buf1[0];

    uint8_t buf2[3];
    BMP388_I2C_ReadBuffer(BMP388_TempAddress, buf2, 3); //16bit for temp
    uncomp_data.temperature = buf2[2] << 8;
    uncomp_data.temperature |= buf2[1];
    uncomp_data.temperature <<= 8;
    uncomp_data.temperature |= buf2[0];


	if (ch == 'T')
	{
		return uncomp_data.temperature;
	}
	else if (ch == 'P')
	{
		return uncomp_data.pressure;
	}
	else{
		return 0;
	}

}


//get calib data
void BMP388_Get_CalibData()
{
	uint8_t reg_data[21];
    BMP388_I2C_ReadBuffer(0x31, reg_data, 21); //21个8bit

	//根据数据手册的数据类型转换
    calib_data.par_t1  = Concat_Bytes(reg_data[1], reg_data[0]);
    calib_data.par_t2  = Concat_Bytes(reg_data[3], reg_data[2]);
    calib_data.par_t3  = (int8_t)reg_data[4];
    calib_data.par_p1  = (int16_t)Concat_Bytes(reg_data[6], reg_data[5]);
    calib_data.par_p2  = (int16_t)Concat_Bytes(reg_data[8], reg_data[7]);
    calib_data.par_p3  = (int8_t)reg_data[9];
    calib_data.par_p4  = (int8_t)reg_data[10];
    calib_data.par_p5  = Concat_Bytes(reg_data[12], reg_data[11]);
    calib_data.par_p6  = Concat_Bytes(reg_data[14],  reg_data[13]);
    calib_data.par_p7  = (int8_t)reg_data[15];
    calib_data.par_p8  = (int8_t)reg_data[16];
    calib_data.par_p9  = (int16_t)Concat_Bytes(reg_data[18], reg_data[17]);
    calib_data.par_p10 = (int8_t)reg_data[19];
    calib_data.par_p11 = (int8_t)reg_data[20];

}

//修正温度
void BMP388_Compensate_temperature()
{
    uint64_t partial_data1;
    uint64_t partial_data2;
    uint64_t partial_data3;
    int64_t  partial_data4;
    int64_t  partial_data5;
    int64_t  partial_data6;
    int64_t  comp_temp;

    partial_data1 = uncomp_data.temperature - (256 * calib_data.par_t1);
    partial_data2 = calib_data.par_t2 * partial_data1;
    partial_data3 = partial_data1 * partial_data1;
    partial_data4 = (int64_t)partial_data3 * calib_data.par_t3;
    partial_data5 = ((int64_t)(partial_data2 * 262144) + partial_data4);
    partial_data6 = partial_data5 / 4294967296;
    calib_data.t_lin = partial_data6;               /* 存储这个data6为t_lin因为计算气压要用到 */
    comp_temp = (int64_t)((partial_data6 * 25)  / 16384);
    comp_data.temperature = comp_temp;
}


//修正气压
void BMP388_Compensate_pressure()
{
    int64_t partial_data1;
    int64_t partial_data2;
    int64_t partial_data3;
    int64_t partial_data4;
    int64_t partial_data5;
    int64_t partial_data6;
    int64_t offset;
    int64_t sensitivity;
    uint64_t comp_press;

    partial_data1 = calib_data.t_lin * calib_data.t_lin;
    partial_data2 = partial_data1 / 64;
    partial_data3 = (partial_data2 * calib_data.t_lin) / 256;
    partial_data4 = (calib_data.par_p8 * partial_data3) / 32;
    partial_data5 = (calib_data.par_p7 * partial_data1) * 16;
    partial_data6 = (calib_data.par_p6 * calib_data.t_lin) * 4194304;
    offset = (calib_data.par_p5 * 140737488355328) + partial_data4 + partial_data5 + partial_data6;

    partial_data2 = (calib_data.par_p4 * partial_data3) / 32;
    partial_data4 = (calib_data.par_p3 * partial_data1) * 4;
    partial_data5 = (calib_data.par_p2 - 16384) * calib_data.t_lin * 2097152;
    sensitivity = ((calib_data.par_p1 - 16384) * 70368744177664) + partial_data2 + partial_data4 + partial_data5;

    partial_data1 = (sensitivity / 16777216) * uncomp_data.pressure;
    partial_data2 = calib_data.par_p10 * calib_data.t_lin;
    partial_data3 = partial_data2 + (65536 * calib_data.par_p9);
    partial_data4 = (partial_data3 * uncomp_data.pressure) / 8192;
    partial_data5 = (partial_data4 * uncomp_data.pressure) / 512;
    partial_data6 = (int64_t)((uint64_t)uncomp_data.pressure * (uint64_t)uncomp_data.pressure);
    partial_data2 = (calib_data.par_p11 * partial_data6) / 65536;
    partial_data3 = (partial_data2 * uncomp_data.pressure) / 128;
    partial_data4 = (offset / 4) + partial_data1 + partial_data5 + partial_data3;
    comp_press = (((uint64_t)partial_data4 * 25) / (uint64_t)1099511627776);
    comp_data.pressure = comp_press;
}
