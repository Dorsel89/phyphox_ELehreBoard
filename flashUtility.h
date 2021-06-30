#ifndef FLASHUTILITY_H // include guard
#define FLASHUTILITY_H
#include "mbed.h"
#include <FlashIAP.h>


class FLASH{

    public:
        FLASH(uint32_t startAddress);
        
        void eraseFlash();
        void flashInit();
        int8_t writeELehreConfig(uint16_t* SN, float* GROUNDOFFSETS); 
        int8_t readELehreConfig(uint16_t* SN, float* GROUNDOFFSETS);
        FlashIAP flash;
        int8_t status;

    private:
        uint32_t    flash_start;
        uint32_t    flash_size;
        uint32_t    flash_end;
        uint32_t    page_size;
        uint32_t    sector_size;
        uint8_t*    page_buffer;
        uint32_t    addr;
        uint32_t startAddress;

};


#endif