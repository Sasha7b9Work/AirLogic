#define WIN32_LEAN_AND_MEAN
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

#include <fstream>

#include "libusb.h"

#include "mfbus.h"
#include "mfcrc.h"
#include "mfproto.h"
#include "mfprotophy.h"
#include "mfprotophyuart.h"

#include "serial/serial.h"
#include <iostream>
#include <string>
#include <vector>

#define headerSize sizeof(MFBUS_FLASH_PACKET_HEADER)

typedef struct {
    string port = "", desc = "", hw = "", vid = "", pid = "";
} serialInfo;

#ifdef AES_DECRYPT
#include <AES.h>
#include <CBC.h>
#include <Crypto.h>
CBC<AES128> cbc;
#include "aeskey.h"
#endif

// OS Specific sleep
#ifdef _WIN32
#include <windows.h>
#define _sleep(a) Sleep((a))
#else
#define _sleep(a) usleep((a)*1000)
#endif

#define STM_VID "0483"
#define STM_PID "5740"
//#define STLINK_PID "374b"

uint8_t print_devs(libusb_device **devs) {
    libusb_device *dev;
    int i = 0;
    uint8_t path[8];

    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor");
            return 0;
        }
        if ((desc.idVendor == strtol(STM_VID, NULL, 16)) &&
            ((desc.idProduct ==
            strtol(STM_PID, NULL, 16)) /*||
  (desc.idProduct == strtol(STLINK_PID, NULL,
  16))*/)) {
  /*printf("%04x:%04x (bus %d, device %d)", desc.idVendor, desc.idProduct,
         libusb_get_bus_number(dev), libusb_get_device_address(dev));

  r = libusb_get_port_numbers(dev, path, sizeof(path));
  if (r > 0) {
    printf(" path: %d", path[0]);
    for (int j = 1; j < r; j++)
      printf(".%d", path[j]);
  }
  printf("\n");*/
            return 1;
        }
    }
    return 0;
}

serialInfo enumerate_ports() {
    vector<serial::PortInfo> devices_found = serial::list_ports();
    vector<serial::PortInfo>::iterator iter = devices_found.begin();
    serialInfo found;
    found.port = "";
    found.vid = "";
    found.pid = "";
    found.desc = "";

    while (iter != devices_found.end()) {
        serial::PortInfo device = *iter++;

        /*printf("(%s, %s, %s) %d\n", device.port.c_str(),
           device.description.c_str(), device.hardware_id.c_str(),
               ((device.hardware_id.find("0483") != std::string::npos) &&
                (device.hardware_id.find("5740") != std::string::npos)));*/
        if ((device.hardware_id.find(STM_VID) != std::string::npos)) {
            found.vid = STM_VID;
            if (device.hardware_id.find(STM_PID) != std::string::npos) {
                found.pid = STM_PID;
            }
            /*
            if (device.hardware_id.find(STLINK_PID) >= 0) {
              found.pid = STLINK_PID;
            }
            */
            /*printf("Found device: port: %s, desc: %s\n", device.port.c_str(),
                   device.description.c_str());*/
            found.port = device.port;
            found.hw = device.hardware_id;
            found.desc = device.description;
            return found;
        }
    }
    return found;
}

int usbmain(void) {
    libusb_device **devs;
    int r;
    ssize_t cnt;

    r = libusb_init(NULL);
    if (r < 0)
        return r;

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0) {
        libusb_exit(NULL);
        return (int)cnt;
    }

    uint8_t result = print_devs(devs);
    libusb_free_device_list(devs, 1);

    libusb_exit(NULL);

    return result;
}
void dump_messages(mfProto *dev) {
    printf("==========%s===========\n", dev->name);
    for (uint8_t i = 0; i < MF_QUEUE_LEN; i++) {
        MF_MESSAGE m;
        dev->_readMessageIndex(m, i);
        char *_M = (char *)(&m); // get the signature first letter (should be M)
        char *_F = _M + 1;       // get the signature second letter (should be F)
        if (m.head.id > 0) {
            printf("%d: ", i);
            printf("%c%c %3u>%-3u [%-3u] #%-3u (%02X) Timeout: %u, =%-4u\n", *_M, *_F,
                m.head.from, m.head.to, m.head.type, m.head.id, m.foot.crc,
                TICKS() - m.time, m.head.size);
            printf("[");
            for (uint8_t c = 0; c < m.head.size; c++)
                printf("%02X ", m.data[c]);
            printf("]\n");

        }
        else {
        }
    }
}
void bl_DumpDebug(mfBus *bus, uint8_t device) {
    uint32_t bytesRead = 0;
    // return;
    do {
        uint32_t id = bus->sendMessageWait(device, MFBUS_DEBUG, NULL, 0, 3, 100);
        if (id) {
            bytesRead = 0;
            MF_MESSAGE msg;
            bus->readMessageId(msg, id);
            if (msg.head.type == MFBUS_DEBUG) {
                bytesRead = msg.head.size;
                if (bytesRead > 0) {
                    uint32_t counter = 0;
                    string message = "";
                    while (counter < bytesRead) {
                        char *c = (char *)(msg.data + counter);
                        // printf("[%c]", *c);
                        message += (*c);
                        counter++;
                    }
                    printf("%s", message.c_str());
                }
            }
            // bus->deleteMessageId(id);
        }
        else {
            printf("DEBUG: transfer Error!\n");
        }
    } while (bytesRead);
}

#define ERR(a)                                                                 \
  do {                                                                         \
    printf("%s\n", (a));                                                       \
    exit(1);                                                                   \
  } while (0)

void bl_CheckState(mfBus *bus, uint8_t dev, mfProtoPhyUart *UART,
    uint8_t &blCheck, std::string &name) {
    uint32_t id = bus->sendMessageWait(dev, MFBUS_BL_CHECK, NULL, 0);
    if (!id)
        printf("...\n");
    if (id) {
        // printf("id:%u\n", id);
        MF_MESSAGE msg;
        bus->readMessageId(msg, id);
        if (msg.head.size != 1) {
            ERR("Error checking device state\n");
        }
        blCheck = (uint8_t)*msg.data;
        // bus->deleteMessageId(id);
        printf("Device [%s] is in the %s mode", name.c_str(),
            (blCheck) ? "bootloader" : "normal");
        if (!blCheck) {
            printf(", reboot request sent...");
            id = bus->sendMessageWait(dev, MFBUS_REBOOT_BL, NULL, 0);
            if (!id) {
                printf("\nNo reboot reply.");
            }
            bus->deleteMessageId(id);
        }
        else {
            printf(".");
        }
        printf("\n");
    }
}
void bl_GetVars(mfBus *bus, uint8_t dev, MFBUS_BL_VARS &blData) {
    uint32_t id = bus->sendMessageWait(dev, MFBUS_BL_GETVARS, NULL, 0);
    if (!id)
        ERR("Error getting bootloader vars...");
    // dump_messages(proto);
    MF_MESSAGE msg;
    bus->readMessageId(msg, id);
    if (msg.head.size != sizeof(MFBUS_BL_VARS))
        printf("Wrong bootloader vars size...");
    else {
        memcpy(&blData, msg.data, sizeof(MFBUS_BL_VARS));
#if 0
        printf("START:0x%x END:0x%x OFFSET:0x%x PAGESIZE:0x%x "
            "MaxPacketSize: %u\n",
            blData.start, blData.end, blData.offset, blData.pagesize,
            blData.maxPacket);
#endif
        printf("ID: %08X-%08X-%08X\n%s flashmap present.\n", blData.UID[0],
            blData.UID[1], blData.UID[2],
            (blData.flashmap) ? "Complex" : "Plain");
    }
    // bus->deleteMessageId(id);
}

std::string bl_GetName(mfBus *bus, uint8_t dev) {
    std::string name = "";
    uint32_t id = bus->sendMessageWait(dev, MFBUS_GETNAME, NULL, 0, 3, 200);
    if (id) {
        // dump_messages(&bus->proto);
        MF_MESSAGE msg;
        bus->readMessageId(msg, id);
        name = std::string((char *)msg.data);
    }
    // bus->deleteMessageId(id);
    return name;
}

typedef struct {
    uint16_t size;
    uint32_t addr;
} FLASH_TABLE_T;

void bl_GetFlashMap(mfBus *bus, uint8_t dev, uint32_t firmwareSize,
    MFBUS_BL_VARS &blData, FLASH_TABLE_T *&flashmap,
    uint16_t &flashmapLen, uint16_t &firstErasePage,
    uint16_t &lastErasePage) {
    if (blData.flashmap) {
        uint32_t id = bus->sendMessageWait(dev, MFBUS_BL_GETFLASHMAP, NULL, 0);
        if (!id)
            ERR("Error getting bootloader vars...");
        MF_MESSAGE msg;
        bus->readMessageId(msg, id);
        if (msg.head.size) {
            flashmapLen = msg.head.size / sizeof(uint16_t);
            flashmap = (FLASH_TABLE_T *)malloc(flashmapLen * sizeof(FLASH_TABLE_T));
            uint16_t *receivedMap = (uint16_t *)msg.data;
            uint16_t offset = 0;
            for (uint16_t i = 0; i < flashmapLen; i++) {
                flashmap[i].size = receivedMap[i];
                flashmap[i].addr = offset * 1024;
                if (flashmap[i].addr == blData.offset) {
                    firstErasePage = i;
                }
                if (blData.offset + firmwareSize > 1024 * offset) {
                    lastErasePage = i;
                }
                offset += flashmap[i].size;
            }
        }
    }
    else {
        // usual block structure
        flashmapLen = (blData.end - blData.start) / blData.pagesize;
        flashmap = (FLASH_TABLE_T *)malloc(flashmapLen * sizeof(FLASH_TABLE_T));
        firstErasePage = blData.offset / blData.pagesize;
        lastErasePage = firstErasePage + ((firmwareSize - 1) / blData.pagesize);
        // printf("maplen:%u fep:%u, lep:%u\n", flashmapLen, firstErasePage,
        //       lastErasePage);
        for (uint16_t i = 0; i < flashmapLen; i++) {
            flashmap[i].size = blData.pagesize / 1024;
            flashmap[i].addr = i * blData.pagesize;
            //  printf("p:%u, size=%0x, addr=%0X\n", i, flashmap[i].size,
            //         flashmap[i].addr);
        }
    }
}
uint8_t bl_ErasePage(mfBus *bus, uint8_t dev, uint32_t page) {
    uint8_t ret = 1;
    // return ret;
    // printf("Erasing page %u\n", page);
    uint32_t id = bus->sendMessageWait(dev, MFBUS_BL_ERASE, (uint8_t *)&page,
        sizeof(page), 10, 3000);
    if (id) {
        MF_MESSAGE msg;
        bus->readMessageId(msg, id);
        uint8_t *answer = (uint8_t *)msg.data;
        // printf("Got answer %d\n", *answer);
        if (*answer == 0) {
            ret = 1;
        }
        else {
            printf("[ERR_%d]\n", *answer);
        }
    }
    else {
        printf("Transfer error/timeout\n");
    }
    // bus->deleteMessageId(id);
    return ret;
}

uint8_t bl_WriteData(mfBus *bus, uint8_t dev, uint8_t *data, uint32_t len) {
    uint8_t ret = 0;
    uint32_t id = bus->sendMessageWait(dev, MFBUS_BL_WRITE, data, len, 20, 500);
    // dump_messages(proto);
    if (id) {
        MF_MESSAGE msg;
        bus->readMessageId(msg, id);
        uint8_t *answer = (uint8_t *)msg.data;
        // printf("Got answer %d\n", *answer);
        if (*answer == 0)
            ret = 1;
        else
            printf("Error %d!", *answer);
    }
    else {
        printf("Transfer error/timeout\n");
    }
    // bus->deleteMessageId(id);
    return ret;
}
typedef struct {
    std::string target = "";
    uint32_t size = 0;
    char *data = NULL;
} MF_FIRMWARE;
std::vector<MF_FIRMWARE> files;
#define DIVIDER ','
#define DATASIZE (256 - 16)
uint32_t getFWblock(MF_FIRMWARE *fw, void *buf, uint32_t pos, uint32_t len) {
    uint32_t _read;
    if (fw->size < pos + len)
        _read = (fw->size - pos);
    else
        _read = len;
    memcpy(buf, &fw->data[pos], _read);
    return _read;
}
void loadFirmwareFile(std::string sign, std::vector<MF_FIRMWARE> &list) {
    std::ifstream firmware;
    uint32_t firmwareSize = 0;
    MF_FIRMWARE fw;
    uint32_t splitPos = sign.find_first_of(DIVIDER);
    if (splitPos == std::string::npos)
        return;
    std::string target = sign.substr(0, splitPos);
    std::string name = sign.substr(splitPos + 1);
    firmware.open(name, std::ios::binary);
    if (firmware.is_open()) {
        firmware.seekg(0, std::ios::end);
        firmwareSize = firmware.tellg();
    }
    else {
        printf("Not open\n");
        return;
    }
#ifdef AES_DECRYPT
    std::ofstream encrypted;
    std::string encName = std::string(name);
    std::string _fname = "";
    std::string _fext = "";
    std::size_t dotPlace = encName.find_last_of('.');
    if (dotPlace == std::string::npos) {
        dotPlace = encName.length();
    }
    _fname = encName.substr(0, dotPlace);
    _fext = encName.substr(dotPlace);
    encName = _fname + "_encrypted" + _fext;
    encrypted.open(encName.c_str(), std::ios::binary);
#endif
    if (firmwareSize > 0) {
        fw.size = firmwareSize;
        fw.data = (char *)malloc(fw.size);
        if (fw.data == NULL) {
            fw.size = 0;
            return;
        }
    }
    // char tempfw[firmwareSize];
    firmware.seekg(0, std::ios::beg);
    firmware.read(fw.data, firmwareSize);
    if (firmware.gcount() != firmwareSize) {
        printf("__%lu__%u__", (unsigned long)firmware.gcount(), firmwareSize);
        free(fw.data);
        return;
    }
#ifdef AES_DECRYPT
#if 0
    uint32_t size = 0;
    do {
        uint32_t wrsize = DATASIZE;
        if (size + wrsize > firmwareSize) {
            wrsize = firmwareSize - size;
        }
        cbc.setKey(AES_KEY, 16);
        cbc.setIV(AES_IV, 16);
        cbc.encrypt((uint8_t *)&fw.data[size], (uint8_t *)&fw.data[size], wrsize);
        size += wrsize;
    } while (size < firmwareSize);
#else
    cbc.setKey(AES_KEY, 16);
    cbc.setIV(AES_IV, 16);
    cbc.encrypt((uint8_t *)fw.data, (uint8_t *)fw.data, firmwareSize);
#endif
    encrypted.write(fw.data, firmwareSize);
    // memcpy(fw.data, tempfw, firmwareSize);
#endif
    fw.target = target;
    list.push_back(fw);
    // free(fw.data);
#ifdef AES_DECRYPT
    encrypted.close();
#endif
    firmware.close();
    return;
}
MF_FIRMWARE *findFWforDevice(std::string name, std::vector<MF_FIRMWARE> &list) {
    for (uint32_t i = 0; i < list.size(); i++)
        if (!list[i].target.compare(name))
            return &list[i];
    return NULL;
}


static int run()
{
    mfProtoPhyUart *UART = nullptr;
    mfProto *proto;
    mfBus *bus = nullptr;
    uint32_t device = 0;
    uint32_t mainDevice = 0;
    FLASH_TABLE_T *flashmap = NULL;
    uint16_t flashmapLen = 0;
    uint16_t firstErasePage = 0;
    uint16_t lastErasePage = 0;

    setbuf(stdout, NULL);
    serialInfo found = enumerate_ports();
    if (!found.vid.length())
    {
        printf("\nNo proper devices found...\n");
        exit(1);
    }
    printf("Proper device found: [%s:%s] %s\n", found.vid.c_str(),
        found.pid.c_str(), found.desc.c_str());
#ifndef FLASHER
    UART = new mfProtoPhyUart("UART", found.vid, found.pid, 115200, 1000);
    proto = new mfProto("PROTO", *UART);
    bus = new mfBus("BUS", *proto);

    if (!UART->isOpen())
        printf("Serial port is closed!");
    //  bus->monitorMode(true);
    bus->begin();
    DELAY(500);
    if (!bus->getSlavesCount()) {
        printf("No Slaves...");
        /// ERR("No devices found...");
    }
    mainDevice = bus->getSlave(0).device;
#endif
#ifdef FLASHER
    MFBUS_BL_VARS blData;
    uint8_t blCheck = 0;
    uint8_t retries = 10;
    std::string name;
    while (retries--) {
        found = enumerate_ports();
        if (!found.vid.length()) {
            printf("\n...\n");
        }
        else {
            // printf("new inst\n");
            UART = new mfProtoPhyUart("UART", found.vid, found.pid, 115200, 1000);
            proto = new mfProto("PROTO", *UART);
            bus = new mfBus("BUS", *proto);
            bus->begin();
            DELAY(1000);
            if (!bus->getSlavesCount()) {
                printf("...\n");
                /// ERR("No devices found...");
            }
            else {
                // printf("ok...\n");
                mainDevice = bus->getSlave(0).device;
                // UART->UART->open();
                name = bl_GetName(bus, mainDevice);
                bl_CheckState(bus, mainDevice, UART, blCheck, name);
                if (blCheck)
                    break;
                DELAY(4000);
            }
            UART->UART->close();
            do {
            } while (UART->UART->isOpen());
            delete bus;
            delete proto;
            delete UART;
            // printf("deleted\n");
        }
        DELAY(1000);
    }
    if (!blCheck) {
        printf("Unable to switch to the bootloader mode...\n");
        exit(1);
    }
    //bus->begin();
    bl_DumpDebug(bus, mainDevice);
    DELAY(1000);
    uint8_t slavesCount = bus->getSlavesCount();
    printf("Found: ");
    std::vector<MFBUS_SLAVE_T> slaves;
    for (uint8_t s = 0; s < slavesCount; s++) {
        MFBUS_SLAVE_T slave = bus->getSlave(s);
        slaves.push_back(slave);
        std::string slaveName = bl_GetName(bus, slave.device);
        printf("[%s] ", slaveName.c_str());
    }
    printf("\n");

    bl_DumpDebug(bus, mainDevice);
    for (uint32_t f = 0; f < slaves.size(); f++) {
        device = slaves[f].device;
        // printf("[%u]\n", device);
        uint32_t firmwareSize = 0;
        name = bl_GetName(bus, device);
        // printf("[%s]\n", name.c_str());
        MF_FIRMWARE *fw = findFWforDevice(name, files);
        if (fw == NULL) {
            printf("No firmware for device [%s]\n", name.c_str());
            continue;
        }
        printf("Trying to flash device [%s]\n", name.c_str());
        do {
            bl_CheckState(bus, device, UART, blCheck, name);
        } while (!blCheck);
        if (blCheck) {
            firmwareSize = fw->size;
            bl_GetVars(bus, device, blData);
            bl_GetFlashMap(bus, device, firmwareSize, blData, flashmap, flashmapLen,
                firstErasePage, lastErasePage);
            blData.maxPacket = headerSize + DATASIZE;
            bl_DumpDebug(bus, device);
            uint32_t counter;
            uint32_t bytesRead = 0;
            uint32_t filesize_temp;
            printf("Erasing...\n[");
            uint16_t lastStep = 0;
            uint16_t curStep = 0;
            counter = 1;
            float stepSize = 50.0 / (lastErasePage - firstErasePage + 1);

            for (uint32_t p = firstErasePage; p <= lastErasePage; p++) {
                curStep = (uint16_t)((float)counter * stepSize);
                uint16_t bLen = curStep - lastStep;
                lastStep = curStep;
                counter++;
                static char block[1024 * 1024];
                block[bLen] = '\0';
                memset(block, '#', bLen);
                uint8_t ret = 0;
                do {
                    ret = bl_ErasePage(bus, device, p);
                } while (ret == 0);
                printf("%s", block);
            }
            printf("]\n");
            counter = 0;
            bl_DumpDebug(bus, device);
            DELAY(2000);
            uint8_t percent = 0;
            filesize_temp = 0;
            uint32_t readStep = blData.maxPacket - headerSize;
            uint32_t packets = firmwareSize / readStep;
            if (firmwareSize % readStep)
                packets++;
            std::ofstream tt;
            // tt.open("temp", std::ios::binary);
            uint32_t startTime = TICKS();
            printf("Flashing...\n[");
            do {
                uint8_t buf[1024];
                // firmware.read((char *)_buf, readStep);
                bytesRead = getFWblock(fw, (uint8_t *)&buf[headerSize], filesize_temp,
                    readStep);
                // tt.write((const char *)&buf[headerSize], bytesRead);
                if (!bytesRead)
                    break;
                filesize_temp += bytesRead;
                MFBUS_FLASH_PACKET_HEADER *head = (MFBUS_FLASH_PACKET_HEADER *)buf;
                head->data.data_size = bytesRead;
                head->addr = (blData.start + blData.offset) + (counter * readStep);
                uint32_t checksum =
                    mfCRC::crc32((uint32_t *)&buf[headerSize], bytesRead);
                head->data.crc = checksum;

                uint8_t ret = 0;
                do {
                    ret =
                        bl_WriteData(bus, device, buf, head->data.data_size + headerSize);
                    uint8_t percent_temp = (50 * filesize_temp / firmwareSize);
                    if ((percent_temp > percent) && (ret)) {
                        printf("#");
                        percent = percent_temp;
                    }
                    DELAY(1);
                    bl_DumpDebug(bus, device);
                } while (ret == 0);
                counter++;
                // firmware.seekg(readStep, std:);
                // DELAY(10);
            } while (bytesRead == readStep);
            uint32_t timeElapsed = (TICKS() - startTime) / 1000;
            uint32_t secs = timeElapsed % 60;
            uint32_t mins = (timeElapsed / 60) % 60;
            uint32_t hours = (timeElapsed) / 3600;
            printf("]\nFlashing finished! %u bytes written.\n", filesize_temp);
            printf("Time elapsed:");
            if (hours)
                printf(" %uh", hours);
            if (mins)
                printf(" %um", mins);
            printf(" %02us.\n", secs);
            // tt.close();
        }
    }
    bl_DumpDebug(bus, device);
    DELAY(1000);
    uint32_t id = bus->sendMessageWait(mainDevice, MFBUS_REBOOT, NULL, 0, 3, 150);
    bus->deleteMessageId(id);
    exit(0);
#else
#define INTERVAL 10
    uint32_t ticks = TICKS();
    uint32_t printT = ticks;
    while (1) {
        if (TICKS() - printT >= INTERVAL) {
            printT += INTERVAL;
            // dump_messages(proto);
            bl_DumpDebug(bus, mainDevice);
        }
        _sleep(0);
    }
    return 0;
#endif
}

int main(int argc, char *argv[]) {
    char default_name[] = "alpro_v10,firmware.bin";
    /*  if (!usbmain()) {
        printf("No devices found :(\n");
        exit(1);
      }*/
    if (argc < 2)
        loadFirmwareFile(default_name, files);
    else
        for (uint16_t i = 1; i < argc; i++) {
            loadFirmwareFile(argv[i], files);
        }
    run();
}