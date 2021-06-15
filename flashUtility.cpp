#include "flashUtility.h"
#include <cstring>

FLASH::FLASH(uint32_t _startAddress){
    flash.init();
    flash_start = flash.get_flash_start();
    flash_size = flash.get_flash_size();
    flash_end = flash_start + flash_size - 1;
    page_size = flash.get_page_size();
    sector_size = flash.get_sector_size(flash_end);
    page_buffer = new uint8_t[page_size];
    addr = flash_end - sector_size + 1;
    startAddress = _startAddress;
    flash.deinit();

}

void FLASH::eraseFlash(){
    flash.erase(startAddress, sector_size);
}

void FLASH::writeELehreConfig(float channelOffsets[2]){
    flash.init();
    addr = startAddress;
    uint8_t dataBuffer[8];
    memcpy(&dataBuffer[0],&channelOffsets[0],8);
    status = flash.program(dataBuffer, addr, 8);
    flash.deinit();
}

void FLASH::readELehreConfig(float* GROUNDOFFSET){
    flash.init();
    uint8_t bytebuffer[8]={0};
    status = flash.read(bytebuffer, addr, 8);
    flash.deinit();
    memcpy(GROUNDOFFSET[0], &bytebuffer[0], 8);
}