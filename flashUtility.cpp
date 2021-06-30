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

int8_t FLASH::writeELehreConfig(uint16_t* mySN, float* f){
    flash.init();
    flash.erase(startAddress, 4*1024);
    //int8_t dataBuffer[4*2] = {0};
    memcpy(&page_buffer[0],&mySN[0],2);
    memcpy(&page_buffer[0]+2,&f[0],8);
    status = flash.program(page_buffer, startAddress, 10);
    //ThisThread::sleep_for(10ms);
    flash.deinit();
    return status;
}

int8_t FLASH::readELehreConfig(uint16_t* mySN, float* f){
    flash.init();
    uint8_t bytebuffer[10]={0};
    status = flash.read(bytebuffer, startAddress, 10);
    memcpy(&mySN[0], &bytebuffer[0], 2);
    memcpy(&f[0], &bytebuffer[0]+2, 8);
    flash.deinit();
    return status;
}