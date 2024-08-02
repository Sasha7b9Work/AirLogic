#ifndef HAL_SPI_MODULE_DISABLED
#include "w25qxx.h"
#include "stm32_def.h"

#if (_W25QXX_DEBUG == 1)
#include <debug.h>
extern DebugSerial debug;
#endif

#define W25QXX_DUMMY_BYTE 0xEE

w25qxx_t w25qxx;
GPIO_TypeDef *_W25QXX_CS_GPIO;
uint16_t _W25QXX_CS_PIN;

#ifdef MF_RTOS
//#define W25qxxDelay(delay) DELAY(delay)
#define W25qxxDelay(delay)
// YIELD()
#else
//#define W25qxxDelay(delay) HAL_Delay(delay)
#define W25qxxDelay(delay)
#endif

SPI_HandleTypeDef *w25xx_spi;

//###################################################################################################################
void W25qxxSpiSend(uint8_t *Data, uint8_t *Recv, uint32_t Size) {
  HAL_SPI_TransmitReceive(w25xx_spi, Data, Recv, Size, 100);
}
void W25qxxSpiRecv(uint8_t *Data, uint32_t Size) {
  HAL_SPI_Receive(w25xx_spi, Data, Size, 100);
}
uint8_t W25qxxSpi(uint8_t Data) {
  uint8_t ret;
  W25qxxSpiSend(&Data, &ret, 1);
  return ret;
}
//###################################################################################################################
uint32_t W25qxxReadID(void) {
  uint32_t Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x9F);
  Temp0 = W25qxxSpi(W25QXX_DUMMY_BYTE);
  Temp1 = W25qxxSpi(W25QXX_DUMMY_BYTE);
  Temp2 = W25qxxSpi(W25QXX_DUMMY_BYTE);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
  Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;
  return Temp;
}
//###################################################################################################################
void W25qxxReadUniqID(void) {
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x4B);
  for (uint8_t i = 0; i < 4; i++)
    W25qxxSpi(W25QXX_DUMMY_BYTE);
  for (uint8_t i = 0; i < 8; i++)
    w25qxx.UniqID[i] = W25qxxSpi(W25QXX_DUMMY_BYTE);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
}
//###################################################################################################################
void W25qxxWriteEnable(void) {
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x06);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
  W25qxxDelay(1);
}
//###################################################################################################################
void W25qxxWriteDisable(void) {
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x04);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
  W25qxxDelay(1);
}
//###################################################################################################################
uint8_t W25qxxReadStatusRegister(uint8_t SelectStatusRegister_1_2_3) {
  uint8_t status = 0;
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  if (SelectStatusRegister_1_2_3 == 1) {
    W25qxxSpi(0x05);
    status = W25qxxSpi(W25QXX_DUMMY_BYTE);
    w25qxx.StatusRegister1 = status;
  } else if (SelectStatusRegister_1_2_3 == 2) {
    W25qxxSpi(0x35);
    status = W25qxxSpi(W25QXX_DUMMY_BYTE);
    w25qxx.StatusRegister2 = status;
  } else {
    W25qxxSpi(0x15);
    status = W25qxxSpi(W25QXX_DUMMY_BYTE);
    w25qxx.StatusRegister3 = status;
  }
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
  return status;
}
//###################################################################################################################
void W25qxxWriteStatusRegister(uint8_t SelectStatusRegister_1_2_3,
                               uint8_t Data) {
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  if (SelectStatusRegister_1_2_3 == 1) {
    W25qxxSpi(0x01);
    w25qxx.StatusRegister1 = Data;
  } else if (SelectStatusRegister_1_2_3 == 2) {
    W25qxxSpi(0x31);
    w25qxx.StatusRegister2 = Data;
  } else {
    W25qxxSpi(0x11);
    w25qxx.StatusRegister3 = Data;
  }
  W25qxxSpi(Data);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
}
//###################################################################################################################
void W25qxxWaitForWriteEnd(void) {
  W25qxxDelay(1);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x05);
  do {
    w25qxx.StatusRegister1 = W25qxxSpi(W25QXX_DUMMY_BYTE);
    W25qxxDelay(1);
  } while ((w25qxx.StatusRegister1 & 0x01) == 0x01);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
}
//###################################################################################################################
bool W25qxxInit(SPI_HandleTypeDef *_spi, GPIO_TypeDef *_port, uint16_t _pin) {
  _W25QXX_CS_GPIO = _port;
  _W25QXX_CS_PIN = _pin;
  w25xx_spi = _spi;
  w25qxx.Lock = 1;
  while (HAL_GetTick() < 100)
    W25qxxDelay(1);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
  W25qxxDelay(100);
  uint32_t id;
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx Init...\r\n");
#endif
  id = W25qxxReadID();

#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx ID:0x%X\r\n", id);
#endif
  switch (id & 0x0000FFFF) {
  case 0x401A: // 	w25q512
    w25qxx.ID = W25Q512;
    w25qxx.BlockCount = 1024;
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Chip: w25q512\r\n");
#endif
    break;
  case 0x4019: // 	w25q256
    w25qxx.ID = W25Q256;
    w25qxx.BlockCount = 512;
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Chip: w25q256\r\n");
#endif
    break;
  case 0x4018: // 	w25q128
    w25qxx.ID = W25Q128;
    w25qxx.BlockCount = 256;
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Chip: w25q128\r\n");
#endif
    break;
  case 0x4017: //	w25q64
    w25qxx.ID = W25Q64;
    w25qxx.BlockCount = 128;
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Chip: w25q64\r\n");
#endif
    break;
  case 0x4016: //	w25q32
    w25qxx.ID = W25Q32;
    w25qxx.BlockCount = 64;
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Chip: w25q32\r\n");
#endif
    break;
  case 0x4015: //	w25q16
    w25qxx.ID = W25Q16;
    w25qxx.BlockCount = 32;
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Chip: w25q16\r\n");
#endif
    break;
  case 0x4014: //	w25q80
    w25qxx.ID = W25Q80;
    w25qxx.BlockCount = 16;
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Chip: w25q80\r\n");
#endif
    break;
  case 0x4013: //	w25q40
    w25qxx.ID = W25Q40;
    w25qxx.BlockCount = 8;
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Chip: w25q40\r\n");
#endif
    break;
  case 0x4012: //	w25q20
    w25qxx.ID = W25Q20;
    w25qxx.BlockCount = 4;
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Chip: w25q20\r\n");
#endif
    break;
  case 0x4011: //	w25q10
    w25qxx.ID = W25Q10;
    w25qxx.BlockCount = 2;
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Chip: w25q10\r\n");
#endif
    break;
  default:
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx Unknown ID\r\n");
#endif
    w25qxx.Lock = 0;
    return false;
  }
  w25qxx.PageSize = 256;
  w25qxx.SectorSize = 0x1000;
  w25qxx.SectorCount = w25qxx.BlockCount * 16;
  w25qxx.PageCount = (w25qxx.SectorCount * w25qxx.SectorSize) / w25qxx.PageSize;
  w25qxx.BlockSize = w25qxx.SectorSize * 16;
  w25qxx.CapacityInKiloByte = (w25qxx.SectorCount * w25qxx.SectorSize) / 1024;
  W25qxxReadUniqID();
  W25qxxReadStatusRegister(1);
  W25qxxReadStatusRegister(2);
  W25qxxReadStatusRegister(3);
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx Page Size: %d Bytes\r\n", w25qxx.PageSize);
  debug.printf("w25qxx Page Count: %d\r\n", w25qxx.PageCount);
  debug.printf("w25qxx Sector Size: %d Bytes\r\n", w25qxx.SectorSize);
  debug.printf("w25qxx Sector Count: %d\r\n", w25qxx.SectorCount);
  debug.printf("w25qxx Block Size: %d Bytes\r\n", w25qxx.BlockSize);
  debug.printf("w25qxx Block Count: %d\r\n", w25qxx.BlockCount);
  debug.printf("w25qxx Capacity: %d KiloBytes\r\n", w25qxx.CapacityInKiloByte);
#endif
  w25qxx.Lock = 0;
  return true;
}
//###################################################################################################################
void W25qxxEraseChip(void) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
#if (_W25QXX_DEBUG == 1)
  uint32_t StartTime = HAL_GetTick();
  debug.printf("w25qxx EraseChip Begin...\r\n");
#endif
  W25qxxWriteEnable();
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0xC7);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
  W25qxxWaitForWriteEnd();
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx EraseBlock done after %d ms!\r\n",
               HAL_GetTick() - StartTime);
#endif
  W25qxxDelay(10);
  w25qxx.Lock = 0;
}
//###################################################################################################################
void W25qxxEraseSector(uint32_t SectorAddr) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
#if (_W25QXX_DEBUG == 1)
  uint32_t StartTime = HAL_GetTick();
  debug.printf("w25qxx EraseSector %d Begin...\r\n", SectorAddr);
#endif
  W25qxxWaitForWriteEnd();
  SectorAddr = SectorAddr * w25qxx.SectorSize;
  W25qxxWriteEnable();
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x20);
  if (w25qxx.ID >= W25Q256)
    W25qxxSpi((SectorAddr & 0xFF000000) >> 24);
  W25qxxSpi((SectorAddr & 0xFF0000) >> 16);
  W25qxxSpi((SectorAddr & 0xFF00) >> 8);
  W25qxxSpi(SectorAddr & 0xFF);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
  W25qxxWaitForWriteEnd();
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx EraseSector done after %d ms\r\n",
               HAL_GetTick() - StartTime);
#endif
  W25qxxDelay(1);
  w25qxx.Lock = 0;
}
//###################################################################################################################
void W25qxxEraseBlock(uint32_t BlockAddr) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx EraseBlock %d Begin...\r\n", BlockAddr);
  W25qxxDelay(100);
  uint32_t StartTime = HAL_GetTick();
#endif
  W25qxxWaitForWriteEnd();
  BlockAddr = BlockAddr * w25qxx.SectorSize * 16;
  W25qxxWriteEnable();
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0xD8);
  if (w25qxx.ID >= W25Q256)
    W25qxxSpi((BlockAddr & 0xFF000000) >> 24);
  W25qxxSpi((BlockAddr & 0xFF0000) >> 16);
  W25qxxSpi((BlockAddr & 0xFF00) >> 8);
  W25qxxSpi(BlockAddr & 0xFF);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
  W25qxxWaitForWriteEnd();
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx EraseBlock done after %d ms\r\n",
               HAL_GetTick() - StartTime);
  W25qxxDelay(100);
#endif
  W25qxxDelay(1);
  w25qxx.Lock = 0;
}
//###################################################################################################################
uint32_t W25qxxPageToSector(uint32_t PageAddress) {
  return ((PageAddress * w25qxx.PageSize) / w25qxx.SectorSize);
}
//###################################################################################################################
uint32_t W25qxxPageToBlock(uint32_t PageAddress) {
  return ((PageAddress * w25qxx.PageSize) / w25qxx.BlockSize);
}
//###################################################################################################################
uint32_t W25qxxSectorToBlock(uint32_t SectorAddress) {
  return ((SectorAddress * w25qxx.SectorSize) / w25qxx.BlockSize);
}
//###################################################################################################################
uint32_t W25qxxSectorToPage(uint32_t SectorAddress) {
  return (SectorAddress * w25qxx.SectorSize) / w25qxx.PageSize;
}
//###################################################################################################################
uint32_t W25qxxBlockToPage(uint32_t BlockAddress) {
  return (BlockAddress * w25qxx.BlockSize) / w25qxx.PageSize;
}
//###################################################################################################################
bool W25qxxIsEmptyPage(uint32_t Page_Address, uint32_t OffsetInByte,
                       uint32_t NumByteToCheck_up_to_PageSize) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
  if (((NumByteToCheck_up_to_PageSize + OffsetInByte) > w25qxx.PageSize) ||
      (NumByteToCheck_up_to_PageSize == 0))
    NumByteToCheck_up_to_PageSize = w25qxx.PageSize - OffsetInByte;
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx CheckPage:%d, Offset:%d, Bytes:%d begin...\r\n",
               Page_Address, OffsetInByte, NumByteToCheck_up_to_PageSize);
  W25qxxDelay(100);
  uint32_t StartTime = HAL_GetTick();
#endif
  uint8_t pBuffer[32];
  uint32_t WorkAddress;
  uint32_t i;
  for (i = OffsetInByte; i < w25qxx.PageSize; i += sizeof(pBuffer)) {
    HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
    WorkAddress = (i + Page_Address * w25qxx.PageSize);
    W25qxxSpi(0x0B);
    if (w25qxx.ID >= W25Q256)
      W25qxxSpi((WorkAddress & 0xFF000000) >> 24);
    W25qxxSpi((WorkAddress & 0xFF0000) >> 16);
    W25qxxSpi((WorkAddress & 0xFF00) >> 8);
    W25qxxSpi(WorkAddress & 0xFF);
    W25qxxSpi(0);
    W25qxxSpiRecv(pBuffer, sizeof(pBuffer));
    HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
    for (uint8_t x = 0; x < sizeof(pBuffer); x++) {
      if (pBuffer[x] != 0xFF)
        goto NOT_EMPTY;
    }
  }
  if ((w25qxx.PageSize + OffsetInByte) % sizeof(pBuffer) != 0) {
    i -= sizeof(pBuffer);
    for (; i < w25qxx.PageSize; i++) {
      HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
      WorkAddress = (i + Page_Address * w25qxx.PageSize);
      W25qxxSpi(0x0B);
      if (w25qxx.ID >= W25Q256)
        W25qxxSpi((WorkAddress & 0xFF000000) >> 24);
      W25qxxSpi((WorkAddress & 0xFF0000) >> 16);
      W25qxxSpi((WorkAddress & 0xFF00) >> 8);
      W25qxxSpi(WorkAddress & 0xFF);
      W25qxxSpi(0);
      W25qxxSpiRecv(pBuffer, 1);
      HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
      if (pBuffer[0] != 0xFF)
        goto NOT_EMPTY;
    }
  }
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx CheckPage is Empty in %d ms\r\n",
               HAL_GetTick() - StartTime);
  W25qxxDelay(100);
#endif
  w25qxx.Lock = 0;
  return true;
NOT_EMPTY:
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx CheckPage is Not Empty in %d ms\r\n",
               HAL_GetTick() - StartTime);
  W25qxxDelay(100);
#endif
  w25qxx.Lock = 0;
  return false;
}
//###################################################################################################################
bool W25qxxIsEmptySector(uint32_t Sector_Address, uint32_t OffsetInByte,
                         uint32_t NumByteToCheck_up_to_SectorSize) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
  if ((NumByteToCheck_up_to_SectorSize > w25qxx.SectorSize) ||
      (NumByteToCheck_up_to_SectorSize == 0))
    NumByteToCheck_up_to_SectorSize = w25qxx.SectorSize;
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx CheckSector:%d, Offset:%d, Bytes:%d begin...\r\n",
               Sector_Address, OffsetInByte, NumByteToCheck_up_to_SectorSize);
  W25qxxDelay(100);
  uint32_t StartTime = HAL_GetTick();
#endif
  uint8_t pBuffer[32];
  uint32_t WorkAddress;
  uint32_t i;
  for (i = OffsetInByte; i < w25qxx.SectorSize; i += sizeof(pBuffer)) {
    HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
    WorkAddress = (i + Sector_Address * w25qxx.SectorSize);
    W25qxxSpi(0x0B);
    if (w25qxx.ID >= W25Q256)
      W25qxxSpi((WorkAddress & 0xFF000000) >> 24);
    W25qxxSpi((WorkAddress & 0xFF0000) >> 16);
    W25qxxSpi((WorkAddress & 0xFF00) >> 8);
    W25qxxSpi(WorkAddress & 0xFF);
    W25qxxSpi(0);
    W25qxxSpiRecv(pBuffer, sizeof(pBuffer));
    HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
    for (uint8_t x = 0; x < sizeof(pBuffer); x++) {
      if (pBuffer[x] != 0xFF)
        goto NOT_EMPTY;
    }
  }
  if ((w25qxx.SectorSize + OffsetInByte) % sizeof(pBuffer) != 0) {
    i -= sizeof(pBuffer);
    for (; i < w25qxx.SectorSize; i++) {
      HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
      WorkAddress = (i + Sector_Address * w25qxx.SectorSize);
      W25qxxSpi(0x0B);
      if (w25qxx.ID >= W25Q256)
        W25qxxSpi((WorkAddress & 0xFF000000) >> 24);
      W25qxxSpi((WorkAddress & 0xFF0000) >> 16);
      W25qxxSpi((WorkAddress & 0xFF00) >> 8);
      W25qxxSpi(WorkAddress & 0xFF);
      W25qxxSpi(0);
      W25qxxSpiRecv(pBuffer, 1);
      HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
      if (pBuffer[0] != 0xFF)
        goto NOT_EMPTY;
    }
  }
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx CheckSector is Empty in %d ms\r\n",
               HAL_GetTick() - StartTime);
  W25qxxDelay(100);
#endif
  w25qxx.Lock = 0;
  return true;
NOT_EMPTY:
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx CheckSector is Not Empty in %d ms\r\n",
               HAL_GetTick() - StartTime);
  W25qxxDelay(100);
#endif
  w25qxx.Lock = 0;
  return false;
}
//###################################################################################################################
bool W25qxxIsEmptyBlock(uint32_t Block_Address, uint32_t OffsetInByte,
                        uint32_t NumByteToCheck_up_to_BlockSize) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
  if ((NumByteToCheck_up_to_BlockSize > w25qxx.BlockSize) ||
      (NumByteToCheck_up_to_BlockSize == 0))
    NumByteToCheck_up_to_BlockSize = w25qxx.BlockSize;
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx CheckBlock:%d, Offset:%d, Bytes:%d begin...\r\n",
               Block_Address, OffsetInByte, NumByteToCheck_up_to_BlockSize);
  W25qxxDelay(100);
  uint32_t StartTime = HAL_GetTick();
#endif
  uint8_t pBuffer[32];
  uint32_t WorkAddress;
  uint32_t i;
  for (i = OffsetInByte; i < w25qxx.BlockSize; i += sizeof(pBuffer)) {
    HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
    WorkAddress = (i + Block_Address * w25qxx.BlockSize);
    W25qxxSpi(0x0B);
    if (w25qxx.ID >= W25Q256)
      W25qxxSpi((WorkAddress & 0xFF000000) >> 24);
    W25qxxSpi((WorkAddress & 0xFF0000) >> 16);
    W25qxxSpi((WorkAddress & 0xFF00) >> 8);
    W25qxxSpi(WorkAddress & 0xFF);
    W25qxxSpi(0);
    W25qxxSpiRecv(pBuffer, sizeof(pBuffer));
    HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
    for (uint8_t x = 0; x < sizeof(pBuffer); x++) {
      if (pBuffer[x] != 0xFF)
        goto NOT_EMPTY;
    }
  }
  if ((w25qxx.BlockSize + OffsetInByte) % sizeof(pBuffer) != 0) {
    i -= sizeof(pBuffer);
    for (; i < w25qxx.BlockSize; i++) {
      HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
      WorkAddress = (i + Block_Address * w25qxx.BlockSize);
      W25qxxSpi(0x0B);
      if (w25qxx.ID >= W25Q256)
        W25qxxSpi((WorkAddress & 0xFF000000) >> 24);
      W25qxxSpi((WorkAddress & 0xFF0000) >> 16);
      W25qxxSpi((WorkAddress & 0xFF00) >> 8);
      W25qxxSpi(WorkAddress & 0xFF);
      W25qxxSpi(0);
      W25qxxSpiRecv(pBuffer, 1);
      HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
      if (pBuffer[0] != 0xFF)
        goto NOT_EMPTY;
    }
  }
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx CheckBlock is Empty in %d ms\r\n",
               HAL_GetTick() - StartTime);
  W25qxxDelay(100);
#endif
  w25qxx.Lock = 0;
  return true;
NOT_EMPTY:
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx CheckBlock is Not Empty in %d ms\r\n",
               HAL_GetTick() - StartTime);
  W25qxxDelay(100);
#endif
  w25qxx.Lock = 0;
  return false;
}
//###################################################################################################################
void W25qxxWriteByte(uint8_t pBuffer, uint32_t WriteAddr_inBytes) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
#if (_W25QXX_DEBUG == 1)
  uint32_t StartTime = HAL_GetTick();
  debug.printf("w25qxx WriteByte 0x%02X at address %d begin...", pBuffer,
               WriteAddr_inBytes);
#endif
  W25qxxWaitForWriteEnd();
  W25qxxWriteEnable();
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x02);
  if (w25qxx.ID >= W25Q256)
    W25qxxSpi((WriteAddr_inBytes & 0xFF000000) >> 24);
  W25qxxSpi((WriteAddr_inBytes & 0xFF0000) >> 16);
  W25qxxSpi((WriteAddr_inBytes & 0xFF00) >> 8);
  W25qxxSpi(WriteAddr_inBytes & 0xFF);
  W25qxxSpi(pBuffer);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
  W25qxxWaitForWriteEnd();
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx WriteByte done after %d ms\r\n",
               HAL_GetTick() - StartTime);
#endif
  w25qxx.Lock = 0;
}
//###################################################################################################################
void W25qxxWriteBytes(uint8_t *pBuffer, uint32_t WriteAddr_inBytes,
                      uint32_t NumByteToRead) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
#if (_W25QXX_DEBUG == 1)
  uint32_t StartTime = HAL_GetTick();
  debug.printf("w25qxx WriteBytes (%u) at address %d begin...", NumByteToRead,
               WriteAddr_inBytes);
#endif
  W25qxxWaitForWriteEnd();
  uint16_t n;
  uint16_t maxBytes = 256 - (WriteAddr_inBytes % 256);
  // force the first set of bytes to stay within the first page
  uint16_t offset = 0;
  while (NumByteToRead > 0) {
    W25qxxWriteEnable();
    HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
    n = (NumByteToRead <= maxBytes) ? NumByteToRead : maxBytes;
    W25qxxSpi(0x02);
    if (w25qxx.ID >= W25Q256)
      W25qxxSpi((WriteAddr_inBytes & 0xFF000000) >> 24);
    W25qxxSpi((WriteAddr_inBytes & 0xFF0000) >> 16);
    W25qxxSpi((WriteAddr_inBytes & 0xFF00) >> 8);
    W25qxxSpi(WriteAddr_inBytes & 0xFF);
    W25qxxSpiSend(&pBuffer[offset], &pBuffer[offset], n);
    WriteAddr_inBytes += n; // adjust the addresses and remaining bytes by what
                            // we've just transferred.
    offset += n;
    NumByteToRead -= n;
    maxBytes = 256; // now we can do up to 256 bytes per loop
    HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
    W25qxxWaitForWriteEnd();
  }
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx WriteByte done after %d ms\r\n",
               HAL_GetTick() - StartTime);
#endif
  w25qxx.Lock = 0;
}
//###################################################################################################################
void W25qxxWritePage(uint8_t *pBuffer, uint32_t Page_Address,
                     uint32_t OffsetInByte,
                     uint32_t NumByteToWrite_up_to_PageSize) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
  if (((NumByteToWrite_up_to_PageSize + OffsetInByte) > w25qxx.PageSize) ||
      (NumByteToWrite_up_to_PageSize == 0))
    NumByteToWrite_up_to_PageSize = w25qxx.PageSize - OffsetInByte;
  if ((OffsetInByte + NumByteToWrite_up_to_PageSize) > w25qxx.PageSize)
    NumByteToWrite_up_to_PageSize = w25qxx.PageSize - OffsetInByte;
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx WritePage:%d, Offset:%d ,Writes %d Bytes, begin...\r\n",
               Page_Address, OffsetInByte, NumByteToWrite_up_to_PageSize);
  W25qxxDelay(100);
  uint32_t StartTime = HAL_GetTick();
#endif
  W25qxxWaitForWriteEnd();
  W25qxxWriteEnable();
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x02);
  Page_Address = (Page_Address * w25qxx.PageSize) + OffsetInByte;
  if (w25qxx.ID >= W25Q256)
    W25qxxSpi((Page_Address & 0xFF000000) >> 24);
  W25qxxSpi((Page_Address & 0xFF0000) >> 16);
  W25qxxSpi((Page_Address & 0xFF00) >> 8);
  W25qxxSpi(Page_Address & 0xFF);
  HAL_SPI_Transmit(w25xx_spi, pBuffer, NumByteToWrite_up_to_PageSize, 100);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
  W25qxxWaitForWriteEnd();
#if (_W25QXX_DEBUG == 1)
  StartTime = HAL_GetTick() - StartTime;
  for (uint32_t i = 0; i < NumByteToWrite_up_to_PageSize; i++) {
    if ((i % 8 == 0) && (i > 2)) {
      debug.printf("\r\n");
      W25qxxDelay(10);
    }
    debug.printf("0x%02X,", pBuffer[i]);
  }
  debug.printf("\r\n");
  debug.printf("w25qxx WritePage done after %d ms\r\n", StartTime);
  W25qxxDelay(100);
#endif
  W25qxxDelay(1);
  w25qxx.Lock = 0;
}
//###################################################################################################################
void W25qxxWriteSector(uint8_t *pBuffer, uint32_t Sector_Address,
                       uint32_t OffsetInByte,
                       uint32_t NumByteToWrite_up_to_SectorSize) {
  if ((NumByteToWrite_up_to_SectorSize > w25qxx.SectorSize) ||
      (NumByteToWrite_up_to_SectorSize == 0))
    NumByteToWrite_up_to_SectorSize = w25qxx.SectorSize;
#if (_W25QXX_DEBUG == 1)
  debug.printf(
      "+++w25qxx WriteSector:%d, Offset:%d ,Write %d Bytes, begin...\r\n",
      Sector_Address, OffsetInByte, NumByteToWrite_up_to_SectorSize);
  W25qxxDelay(100);
#endif
  if (OffsetInByte >= w25qxx.SectorSize) {
#if (_W25QXX_DEBUG == 1)
    debug.printf("---w25qxx WriteSector Failed!\r\n");
    W25qxxDelay(100);
#endif
    return;
  }
  uint32_t StartPage;
  int32_t BytesToWrite;
  uint32_t LocalOffset;
  if ((OffsetInByte + NumByteToWrite_up_to_SectorSize) > w25qxx.SectorSize)
    BytesToWrite = w25qxx.SectorSize - OffsetInByte;
  else
    BytesToWrite = NumByteToWrite_up_to_SectorSize;
  StartPage =
      W25qxxSectorToPage(Sector_Address) + (OffsetInByte / w25qxx.PageSize);
  LocalOffset = OffsetInByte % w25qxx.PageSize;
  do {
    W25qxxWritePage(pBuffer, StartPage, LocalOffset, BytesToWrite);
    StartPage++;
    BytesToWrite -= w25qxx.PageSize - LocalOffset;
    pBuffer += w25qxx.PageSize - LocalOffset;
    LocalOffset = 0;
  } while (BytesToWrite > 0);
#if (_W25QXX_DEBUG == 1)
  debug.printf("---w25qxx WriteSector Done\r\n");
  W25qxxDelay(100);
#endif
}
//###################################################################################################################
void W25qxxWriteBlock(uint8_t *pBuffer, uint32_t Block_Address,
                      uint32_t OffsetInByte,
                      uint32_t NumByteToWrite_up_to_BlockSize) {
  if ((NumByteToWrite_up_to_BlockSize > w25qxx.BlockSize) ||
      (NumByteToWrite_up_to_BlockSize == 0))
    NumByteToWrite_up_to_BlockSize = w25qxx.BlockSize;
#if (_W25QXX_DEBUG == 1)
  debug.printf(
      "+++w25qxx WriteBlock:%d, Offset:%d ,Write %d Bytes, begin...\r\n",
      Block_Address, OffsetInByte, NumByteToWrite_up_to_BlockSize);
  W25qxxDelay(100);
#endif
  if (OffsetInByte >= w25qxx.BlockSize) {
#if (_W25QXX_DEBUG == 1)
    debug.printf("---w25qxx WriteBlock Failed!\r\n");
    W25qxxDelay(100);
#endif
    return;
  }
  uint32_t StartPage;
  int32_t BytesToWrite;
  uint32_t LocalOffset;
  if ((OffsetInByte + NumByteToWrite_up_to_BlockSize) > w25qxx.BlockSize)
    BytesToWrite = w25qxx.BlockSize - OffsetInByte;
  else
    BytesToWrite = NumByteToWrite_up_to_BlockSize;
  StartPage =
      W25qxxBlockToPage(Block_Address) + (OffsetInByte / w25qxx.PageSize);
  LocalOffset = OffsetInByte % w25qxx.PageSize;
  do {
    W25qxxWritePage(pBuffer, StartPage, LocalOffset, BytesToWrite);
    StartPage++;
    BytesToWrite -= w25qxx.PageSize - LocalOffset;
    pBuffer += w25qxx.PageSize - LocalOffset;
    LocalOffset = 0;
  } while (BytesToWrite > 0);
#if (_W25QXX_DEBUG == 1)
  debug.printf("---w25qxx WriteBlock Done\r\n");
  W25qxxDelay(100);
#endif
}
//###################################################################################################################
void W25qxxReadByte(uint8_t *pBuffer, uint32_t Bytes_Address) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
#if (_W25QXX_DEBUG == 1)
  uint32_t StartTime = HAL_GetTick();
  debug.printf("w25qxx ReadByte at address %d begin...\r\n", Bytes_Address);
#endif
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x0B);
  if (w25qxx.ID >= W25Q256)
    W25qxxSpi((Bytes_Address & 0xFF000000) >> 24);
  W25qxxSpi((Bytes_Address & 0xFF0000) >> 16);
  W25qxxSpi((Bytes_Address & 0xFF00) >> 8);
  W25qxxSpi(Bytes_Address & 0xFF);
  W25qxxSpi(0);
  *pBuffer = W25qxxSpi(W25QXX_DUMMY_BYTE);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx ReadByte 0x%02X done after %d ms\r\n", *pBuffer,
               HAL_GetTick() - StartTime);
#endif
  w25qxx.Lock = 0;
}
//###################################################################################################################
void W25qxxReadBytes(uint8_t *pBuffer, uint32_t ReadAddr,
                     uint32_t NumByteToRead) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
#if (_W25QXX_DEBUG == 1)
  uint32_t StartTime = HAL_GetTick();
  debug.printf("w25qxx ReadBytes at Address:%d, %d Bytes  begin...\r\n",
               ReadAddr, NumByteToRead);
#endif
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x0B);
  if (w25qxx.ID >= W25Q256)
    W25qxxSpi((ReadAddr & 0xFF000000) >> 24);
  W25qxxSpi((ReadAddr & 0xFF0000) >> 16);
  W25qxxSpi((ReadAddr & 0xFF00) >> 8);
  W25qxxSpi(ReadAddr & 0xFF);
  W25qxxSpi(0);
  W25qxxSpiRecv(pBuffer, NumByteToRead);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
#if (_W25QXX_DEBUG == 1)
  StartTime = HAL_GetTick() - StartTime;
  for (uint32_t i = 0; i < NumByteToRead; i++) {
    if ((i % 8 == 0) && (i > 2)) {
      debug.printf("\r\n");
      W25qxxDelay(10);
    }
    debug.printf("0x%02X,", pBuffer[i]);
  }
  debug.printf("\r\n");
  debug.printf("w25qxx ReadBytes done after %d ms\r\n", StartTime);
  W25qxxDelay(100);
#endif
  W25qxxDelay(1);
  w25qxx.Lock = 0;
}
//###################################################################################################################
void W25qxxReadPage(uint8_t *pBuffer, uint32_t Page_Address,
                    uint32_t OffsetInByte,
                    uint32_t NumByteToRead_up_to_PageSize) {
  while (w25qxx.Lock == 1)
    W25qxxDelay(1);
  w25qxx.Lock = 1;
  if ((NumByteToRead_up_to_PageSize > w25qxx.PageSize) ||
      (NumByteToRead_up_to_PageSize == 0))
    NumByteToRead_up_to_PageSize = w25qxx.PageSize;
  if ((OffsetInByte + NumByteToRead_up_to_PageSize) > w25qxx.PageSize)
    NumByteToRead_up_to_PageSize = w25qxx.PageSize - OffsetInByte;
#if (_W25QXX_DEBUG == 1)
  debug.printf("w25qxx ReadPage:%d, Offset:%d ,Read %d Bytes, begin...\r\n",
               Page_Address, OffsetInByte, NumByteToRead_up_to_PageSize);
  W25qxxDelay(100);
  uint32_t StartTime = HAL_GetTick();
#endif
  Page_Address = Page_Address * w25qxx.PageSize + OffsetInByte;
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_RESET);
  W25qxxSpi(0x0B);
  if (w25qxx.ID >= W25Q256)
    W25qxxSpi((Page_Address & 0xFF000000) >> 24);
  W25qxxSpi((Page_Address & 0xFF0000) >> 16);
  W25qxxSpi((Page_Address & 0xFF00) >> 8);
  W25qxxSpi(Page_Address & 0xFF);
  W25qxxSpi(0);
  W25qxxSpiRecv(pBuffer, NumByteToRead_up_to_PageSize);
  HAL_GPIO_WritePin(_W25QXX_CS_GPIO, _W25QXX_CS_PIN, GPIO_PIN_SET);
#if (_W25QXX_DEBUG == 1)
  StartTime = HAL_GetTick() - StartTime;
  for (uint32_t i = 0; i < NumByteToRead_up_to_PageSize; i++) {
    if ((i % 8 == 0) && (i > 2)) {
      debug.printf("\r\n");
      W25qxxDelay(10);
    }
    debug.printf("0x%02X,", pBuffer[i]);
  }
  debug.printf("\r\n");
  debug.printf("w25qxx ReadPage done after %d ms\r\n", StartTime);
  W25qxxDelay(100);
#endif
  W25qxxDelay(1);
  w25qxx.Lock = 0;
}
//###################################################################################################################
void W25qxxReadSector(uint8_t *pBuffer, uint32_t Sector_Address,
                      uint32_t OffsetInByte,
                      uint32_t NumByteToRead_up_to_SectorSize) {
  if ((NumByteToRead_up_to_SectorSize > w25qxx.SectorSize) ||
      (NumByteToRead_up_to_SectorSize == 0))
    NumByteToRead_up_to_SectorSize = w25qxx.SectorSize;
#if (_W25QXX_DEBUG == 1)
  debug.printf(
      "+++w25qxx ReadSector:%d, Offset:%d ,Read %d Bytes, begin...\r\n",
      Sector_Address, OffsetInByte, NumByteToRead_up_to_SectorSize);
  W25qxxDelay(100);
#endif
  if (OffsetInByte >= w25qxx.SectorSize) {
#if (_W25QXX_DEBUG == 1)
    debug.printf("---w25qxx ReadSector Failed!\r\n");
    W25qxxDelay(100);
#endif
    return;
  }
  uint32_t StartPage;
  int32_t BytesToRead;
  uint32_t LocalOffset;
  if ((OffsetInByte + NumByteToRead_up_to_SectorSize) > w25qxx.SectorSize)
    BytesToRead = w25qxx.SectorSize - OffsetInByte;
  else
    BytesToRead = NumByteToRead_up_to_SectorSize;
  StartPage =
      W25qxxSectorToPage(Sector_Address) + (OffsetInByte / w25qxx.PageSize);
  LocalOffset = OffsetInByte % w25qxx.PageSize;
  do {
    W25qxxReadPage(pBuffer, StartPage, LocalOffset, BytesToRead);
    StartPage++;
    BytesToRead -= w25qxx.PageSize - LocalOffset;
    pBuffer += w25qxx.PageSize - LocalOffset;
    LocalOffset = 0;
  } while (BytesToRead > 0);
#if (_W25QXX_DEBUG == 1)
  debug.printf("---w25qxx ReadSector Done\r\n");
  W25qxxDelay(100);
#endif
}
//###################################################################################################################
void W25qxxReadBlock(uint8_t *pBuffer, uint32_t Block_Address,
                     uint32_t OffsetInByte,
                     uint32_t NumByteToRead_up_to_BlockSize) {
  if ((NumByteToRead_up_to_BlockSize > w25qxx.BlockSize) ||
      (NumByteToRead_up_to_BlockSize == 0))
    NumByteToRead_up_to_BlockSize = w25qxx.BlockSize;
#if (_W25QXX_DEBUG == 1)
  debug.printf("+++w25qxx ReadBlock:%d, Offset:%d ,Read %d Bytes, begin...\r\n",
               Block_Address, OffsetInByte, NumByteToRead_up_to_BlockSize);
  W25qxxDelay(100);
#endif
  if (OffsetInByte >= w25qxx.BlockSize) {
#if (_W25QXX_DEBUG == 1)
    debug.printf("w25qxx ReadBlock Failed!\r\n");
    W25qxxDelay(100);
#endif
    return;
  }
  uint32_t StartPage;
  int32_t BytesToRead;
  uint32_t LocalOffset;
  if ((OffsetInByte + NumByteToRead_up_to_BlockSize) > w25qxx.BlockSize)
    BytesToRead = w25qxx.BlockSize - OffsetInByte;
  else
    BytesToRead = NumByteToRead_up_to_BlockSize;
  StartPage =
      W25qxxBlockToPage(Block_Address) + (OffsetInByte / w25qxx.PageSize);
  LocalOffset = OffsetInByte % w25qxx.PageSize;
  do {
    W25qxxReadPage(pBuffer, StartPage, LocalOffset, BytesToRead);
    StartPage++;
    BytesToRead -= w25qxx.PageSize - LocalOffset;
    pBuffer += w25qxx.PageSize - LocalOffset;
    LocalOffset = 0;
  } while (BytesToRead > 0);
#if (_W25QXX_DEBUG == 1)
  debug.printf("---w25qxx ReadBlock Done\r\n");
  W25qxxDelay(100);
#endif
}
//###################################################################################################################
#endif