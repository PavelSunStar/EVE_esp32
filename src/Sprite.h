#pragma once

#include <Arduino.h>
#include "esp_heap_caps.h"
#include "esp_psram.h"   // esp_psram_is_initialized()
#include "EVE_esp32.h"

#define MAX_SPRITE_LINES    1024

struct Image{
    int width, height;
    int xx, yy;
    int cx, cy;    
    int lineSize, size;
    int fullSize;
    int offset, offsetLine;
};

class Sprite{
    public:
        uint8_t*    buf = nullptr;
        uint8_t*    lineBuf8[MAX_SPRITE_LINES];
        uint16_t*   lineBuf16[MAX_SPRITE_LINES];

        uint16_t Images()               { return _maxImage; }
        int Width(uint16_t num = 0)     { if (!_created || num >= _maxImage) return 0; return _img[num].width; }
        int Height(uint16_t num = 0)    { if (!_created || num >= _maxImage) return 0; return _img[num].height; }

        Sprite(EVE_esp32& vga);
        ~Sprite();

        bool loadImages(const uint8_t* data);
        void sendToEve(uint32_t addr, uint16_t num, uint8_t format = ARGB1555);
        bool createImages(uint16_t xx, uint16_t yy, uint16_t num = 1);
        void putImage(int x, int y, uint16_t num = 0);

        void cls(uint16_t col = 0, uint16_t num = 0);
        void putPixel(int x, int y, uint16_t col, uint16_t num = 0);
        void putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint16_t num = 0);
        void hLine(int x1, int y, int x2, uint16_t col, uint16_t num = 0);
        void vLine(int x, int y1, int y2, uint16_t col, uint16_t num = 0);
        void rect(int x1, int y1, int x2, int y2, uint16_t col, uint16_t num = 0);
        void fillRect(int x1, int y1, int x2, int y2, uint16_t col, uint16_t num = 0);
        void line(int x1, int y1, int x2, int y2, uint16_t col, uint16_t num = 0);
        void triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col, uint16_t num = 0);
        void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col, uint16_t num = 0);
        void circle(int xc, int yc, int r, uint16_t col, uint16_t num = 0);
        void fillCircle(int xc, int yc, int r, uint16_t col, uint16_t num = 0);
        void polygon(int x, int y, int radius, int sides, int ang, uint16_t col, uint16_t num = 0);
        void fillPolygon(int x, int y, int radius, int sides, int rotation, uint16_t col, uint16_t num = 0);
        void star(int x, int y, int radius, int sides, int ang, uint16_t col, uint16_t num = 0);
        void fillStar(int x, int y, int radius, int sides, int rotation, uint16_t col, uint16_t num = 0);
        void ellipse(int xc, int yc, int rx, int ry, uint16_t col, uint16_t num = 0);
        void fillEllipse(int xc, int yc, int rx, int ry, uint16_t col, uint16_t num = 0);
        void arc(int xc, int yc, int r, int angle1, int angle2, uint16_t col, uint16_t num = 0);

    protected:
        Image*      _img = nullptr;
        uint8_t     _bpp;
        uint8_t     _shift;
        bool        _created = false; 
        bool        _psram_ok = false;

        uint16_t    _maxImage;

        void freeMemory();
        void resetParam(uint16_t num);
        bool allocateMemory(size_t size);

        EVE_esp32& _vga;    
};