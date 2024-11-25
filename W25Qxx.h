/*
 * W25Qxx.h
 *
 *  Created on: Nov 22, 2024
 *      Author: adnan
 */

#ifndef INC_W25QXX_H_
#define INC_W25QXX_H_


void W25Q_Reset(void);
uint32_t W25Q_ReadID(void);

void W25Q_Read (uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData);
void W25Q_FastRead (uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData);

void W25Q_Erase_Sector (uint16_t sector_number);
void W25Q_Erase_Sectors (uint16_t page, uint16_t offset, uint32_t size);
void W25Q_Write_Page (uint32_t page, uint16_t offset, uint32_t size, uint8_t *data);

#endif /* INC_W25QXX_H_ */
