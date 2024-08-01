#ifndef _W25QXX_H
#define _W25QXX_H

#ifndef HAL_SPI_MODULE_DISABLED

#include "stm32_def.h"

//#define _W25QXX_DEBUG 1

#include <inttypes.h>
#include <stdbool.h>

typedef enum {
  W25Q10 = 1,
  W25Q20,
  W25Q40,
  W25Q80,
  W25Q16,
  W25Q32,
  W25Q64,
  W25Q128,
  W25Q256,
  W25Q512,

} W25QXX_ID_t;

typedef struct {
  W25QXX_ID_t ID;
  uint8_t UniqID[8];
  uint16_t PageSize;
  uint32_t PageCount;
  uint32_t SectorSize;
  uint32_t SectorCount;
  uint32_t BlockSize;
  uint32_t BlockCount;
  uint32_t CapacityInKiloByte;
  uint8_t StatusRegister1;
  uint8_t StatusRegister2;
  uint8_t StatusRegister3;
  uint8_t Lock;

} w25qxx_t;

extern w25qxx_t w25qxx;
//############################################################################
// in Page,Sector and block read/write functions, can put 0 to read maximum
// bytes
//############################################################################
bool W25qxxInit(SPI_HandleTypeDef *_spi, GPIO_TypeDef *_port, uint16_t _pin);

void W25qxxEraseChip(void);
void W25qxxEraseSector(uint32_t SectorAddr);
void W25qxxEraseBlock(uint32_t BlockAddr);

uint32_t W25qxxPageToSector(uint32_t PageAddress);
uint32_t W25qxxPageToBlock(uint32_t PageAddress);
uint32_t W25qxxSectorToBlock(uint32_t SectorAddress);
uint32_t W25qxxSectorToPage(uint32_t SectorAddress);
uint32_t W25qxxBlockToPage(uint32_t BlockAddress);

bool W25qxxIsEmptyPage(uint32_t Page_Address, uint32_t OffsetInByte,
                       uint32_t NumByteToCheck_up_to_PageSize);
bool W25qxxIsEmptySector(uint32_t Sector_Address, uint32_t OffsetInByte,
                         uint32_t NumByteToCheck_up_to_SectorSize);
bool W25qxxIsEmptyBlock(uint32_t Block_Address, uint32_t OffsetInByte,
                        uint32_t NumByteToCheck_up_to_BlockSize);

void W25qxxWriteByte(uint8_t pBuffer, uint32_t Bytes_Address);
void W25qxxWriteBytes(uint8_t *pBuffer, uint32_t WriteAddr_inBytes,
                      uint32_t NumByteToRead);
void W25qxxWritePage(uint8_t *pBuffer, uint32_t Page_Address,
                     uint32_t OffsetInByte,
                     uint32_t NumByteToWrite_up_to_PageSize);
void W25qxxWriteSector(uint8_t *pBuffer, uint32_t Sector_Address,
                       uint32_t OffsetInByte,
                       uint32_t NumByteToWrite_up_to_SectorSize);
void W25qxxWriteBlock(uint8_t *pBuffer, uint32_t Block_Address,
                      uint32_t OffsetInByte,
                      uint32_t NumByteToWrite_up_to_BlockSize);

void W25qxxReadByte(uint8_t *pBuffer, uint32_t Bytes_Address);
void W25qxxReadBytes(uint8_t *pBuffer, uint32_t ReadAddr,
                     uint32_t NumByteToRead);
void W25qxxReadPage(uint8_t *pBuffer, uint32_t Page_Address,
                    uint32_t OffsetInByte,
                    uint32_t NumByteToRead_up_to_PageSize);
void W25qxxReadSector(uint8_t *pBuffer, uint32_t Sector_Address,
                      uint32_t OffsetInByte,
                      uint32_t NumByteToRead_up_to_SectorSize);
void W25qxxReadBlock(uint8_t *pBuffer, uint32_t Block_Address,
                     uint32_t OffsetInByte,
                     uint32_t NumByteToRead_up_to_BlockSize);
//############################################################################

#endif

#endif