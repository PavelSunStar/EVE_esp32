#pragma once

#include "EVE_esp32.h"

class Sprite{
    public:
        uint8_t* buf = nullptr;

        int MaxImages() { return frames; }

        Sprite(EVE_esp32& vga);
        ~Sprite();

        bool loadImages(const uint8_t* data);
        void putImage(int x, int y, int num = 0);

        EVE_Image sendToEve(uint32_t addr, uint16_t num = 0, uint8_t format = ARGB1555);

    protected:
        Image   *info;          // динамический массив структур Image  
        int bpp, pixelSize;     // бит на пиксель (8 или 16)        
        uint8_t frames = 0;     // количество кадров / изображений

        void destroy();
        bool allocateMemory(size_t size);

        EVE_esp32& _vga;    
};