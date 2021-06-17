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
    flash.erase(startAddress, 4*1024);
}

int8_t FLASH::writeELehreConfig(float* f){
    flash.init();
    flash.erase(startAddress, 4*1024);
    //int8_t dataBuffer[4*2] = {0};
    memcpy(&page_buffer[0],&f[0],8);
    status = flash.program(page_buffer, startAddress, 8);
    //ThisThread::sleep_for(10ms);
    flash.deinit();
    return status;
}

int8_t FLASH::readELehreConfig(float* f){
    flash.init();
    uint8_t bytebuffer[8]={0};
    status = flash.read(bytebuffer, startAddress, 8);
    memcpy(&f[0], &bytebuffer[0], 8);
    flash.deinit();
    return status;
}