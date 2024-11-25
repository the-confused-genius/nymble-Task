/*
 * W25Qxx.c
 *
 *  Created on: Nov 22, 2024
 *      Author: adnan
 */


#include "main.h"
#include "W25Qxx.h"

extern SPI_HandleTypeDef hspi1;
#define W25Q_SPI hspi1
#define numBLOCK 256

//W25W64FW  -> 64 Mbit -> 16MB
//has 32768 pages and each page of 256 bytes
//erase happens on group of 16 pages (minimum)
//4096 sectors totally, sector is of 4KB each
//256 blocks, each block has 16 sectors
//JEDEC -> 0xef6017h

void csLOW (void){
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
}

void csHIGH (void){
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
}

void SPI_Write (uint8_t *data, uint16_t len){
	HAL_SPI_Transmit(&W25Q_SPI, data, len, 2000);
}

void SPI_Read (uint8_t *data, uint16_t len){
	HAL_SPI_Receive(&W25Q_SPI, data, len, 5000);
}

void W25Q_Reset (void)
{
	uint8_t tData[2];
	tData[1] = 0x66;	//enable Reset
	tData[2] = 0x99;	//Reset

	csLOW();
	SPI_Write(tData, 2);
	csHIGH();

	HAL_Delay(100);
}

uint32_t W25Q_ReadID (void)
{
	uint8_t tData[] = {0x9F};	//Read JEDEC ID
	uint8_t rData[3];

	csLOW();
	SPI_Write(tData, 1);
	SPI_Read(rData, 3);
	csHIGH();

	return ((rData[0]<<16) | (rData[1]<<8) | (rData[2]));
}

void W25Q_Read (uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData)
{
	uint8_t tData[5] = {0};
	uint32_t memAddr = (startPage*256) + offset;

	if (numBLOCK<512)
	{
		tData[0] = 0x03;	//enable Read
		tData[1] = (memAddr>>16)&0xFF;		//MSB of address
		tData[2] = (memAddr>>8)&0xFF;
		tData[3] = (memAddr)&0xFF;			//LSB of address
		csLOW();
		SPI_Write(tData, 4);
	}
	else{
		tData[0] = 0x13;	//enable Read with 4-Byte address
		tData[1] = (memAddr>>24)&0xFF;
		tData[2] = (memAddr>>16)&0xFF;
		tData[3] = (memAddr>>8)&0xFF;
		tData[4] = (memAddr)&0xFF;
		csLOW();
		SPI_Write(tData, 5);
	}

	SPI_Read(rData, size);
	csHIGH();
}

void W25Q_FastRead (uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData)
{
	uint8_t tData[6] = {0};
	uint32_t memAddr = (startPage*256) + offset;

	if (numBLOCK<512)
	{
		tData[0] = 0x0B;	//enable Fast Read
		tData[1] = (memAddr>>16)&0xFF;
		tData[2] = (memAddr>>8)&0xFF;
		tData[3] = (memAddr)&0xFF;
		tData[4] = 0;					//Dummy clock
		csLOW();
		SPI_Write(tData, 5);
	}
	else{
		tData[0] = 0x0C;	//Fast Read with 4-Byte address
		tData[1] = (memAddr>>24)&0xFF;
		tData[2] = (memAddr>>16)&0xFF;
		tData[3] = (memAddr>>8)&0xFF;
		tData[4] = (memAddr)&0xFF;
		tData[5] = 0;					//Dummy clock
		csLOW();
		SPI_Write(tData, 6);
	}

	SPI_Read(rData, size);
	csHIGH();
}

void write_enable (void)
{
	uint8_t tData[] = {0x06};		//enable write

	csLOW();
	SPI_Write(tData, 1);
	csHIGH();

	HAL_Delay(5);
}

void write_disable (void)
{
	uint8_t tData[] = {0x04};		//disable write

	csLOW();
	SPI_Write(tData, 1);
	csHIGH();

	HAL_Delay(5);
}

uint32_t bytestowrite (uint32_t size, uint16_t offset)
{
	if ((size+offset)<256)	return size;
	return 256 - offset;
}

void W25Q_Erase_Sector (uint16_t sector_number)
{
	uint8_t tData[6] = {0};
	uint32_t memAddr = sector_number * 16 * 256;	//each sector contains 16 pages * 256 bytes

	write_enable();

	if (numBLOCK<512)
	{
		tData[0] = 0x20;		//Erase sector
		tData[1] = (memAddr>>16)&0xFF;
		tData[2] = (memAddr>>8)&0xFF;
		tData[3] = (memAddr)&0xFF;
		csLOW();
		SPI_Write(tData, 4);
	}
	else{
		tData[0] = 0x21;		//Erase sector with 4-Byte address
		tData[1] = (memAddr>>24)&0xFF;
		tData[2] = (memAddr>>16)&0xFF;
		tData[3] = (memAddr>>8)&0xFF;
		tData[4] = (memAddr)&0xFF;
		csLOW();
		SPI_Write(tData, 5);
	}

	csHIGH();
	HAL_Delay(450);
	write_disable();
}

void W25Q_Erase_Sectors (uint16_t page, uint16_t offset, uint32_t size)
{
	uint32_t startPage = page;
	uint32_t endPage = startPage + ((size+offset-1)/256);

	uint32_t startSector = startPage / 16;
	uint32_t endSector = endPage / 16;
	uint32_t numSectors = endSector - startSector + 1;

	for(uint16_t i=0; i<numSectors; i++)
		W25Q_Erase_Sector(startSector++);
}

void W25Q_Write_Page (uint32_t page, uint16_t offset, uint32_t size, uint8_t *data)
{
	uint8_t tData[266] = {0};		//instruction x 1, address x 4, data x 256
	uint32_t startPage = page;
	uint32_t endPage = startPage + ((size+offset-1)/256);
	uint32_t numPages = endPage - startPage + 1;

	uint32_t dataPostion = 0;
	for(uint32_t i=0; i<numPages; i++)
	{
		uint32_t memAddr = (startPage*256) + offset;
		uint16_t bytesremaining = bytestowrite(size, offset);
		uint32_t indx = 0;

		write_enable();
		if (numBLOCK<512)
		{
			tData[0] = 0x02;		//page program
			tData[1] = (memAddr>>16)&0xFF;
			tData[2] = (memAddr>>8)&0xFF;
			tData[3] = (memAddr)&0xFF;

			indx = 4;
		}
		else{
			tData[0] = 0x12;		//page program with 4-Byte address
			tData[1] = (memAddr>>24)&0xFF;
			tData[2] = (memAddr>>16)&0xFF;
			tData[3] = (memAddr>>8)&0xFF;
			tData[4] = (memAddr)&0xFF;

			indx = 5;
		}

		uint16_t bytestosend = bytesremaining + indx;

		for (uint16_t i=0; i<bytesremaining; i++)
			tData[indx++] = data[i+dataPostion];

		if (bytestosend > 200)
		{
			csLOW();
			SPI_Write(tData, 100);
			SPI_Write(tData+100, bytestosend-100);
			csHIGH();
		}
		else{
			csLOW();
			SPI_Write(tData, bytestosend);
			csHIGH();
		}

		startPage++;
		offset = 0;
		size = size - bytesremaining;
		dataPostion = dataPostion + bytesremaining;

		HAL_Delay(5);
		write_disable();
	}

}



















