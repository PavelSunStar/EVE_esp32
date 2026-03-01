#pragma once

#include "EVE_esp32.h"
#include "config.h"
#include "eve_utils.h"

class GFX{
    public:
        GFX(EVE_esp32& vga);
        ~GFX();

        void cls(uint16_t col = 0);
        void putPixel(int x, int y, uint16_t col);
        void hLine(int x1, int y, int x2, uint16_t col);
        void vLine(int x, int y1, int y2, uint16_t col);
        void rect(int x1, int y1, int x2, int y2, uint16_t col);
        void fillRect(int x1, int y1, int x2, int y2, uint16_t col);
        void line(int x1, int y1, int x2, int y2, uint16_t col);
        void triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col);
        void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col);
        void circle(int xc, int yc, int r, uint16_t col);
        void fillCircle(int xc, int yc, int r, uint16_t col);
        void polygon(int x, int y, int radius, int sides, int ang, uint16_t col);
        void fillPolygon(int x, int y, int radius, int sides, int rotation, uint16_t col);
        void star(int x, int y, int radius, int sides, int ang, uint16_t col);
        void fillStar(int x, int y, int radius, int sides, int rotation, uint16_t col);
        void ellipse(int xc, int yc, int rx, int ry, uint16_t col);
        void fillEllipse(int xc, int yc, int rx, int ry, uint16_t col);
        void arc(int xc, int yc, int r, int angle1, int angle2, uint16_t col);

        void blur();
        void testRGBPanel();
        
        static inline uint16_t div3_u16(uint16_t x) {
            // x/3 без деления (хорошо для диапазонов до ~65535)
            return (uint16_t)((uint32_t)x * 21845u >> 16);
        }        

        static inline uint16_t blur565(uint16_t c0, uint16_t c1, uint16_t c2, uint16_t c3){
            // Суммируем RB отдельно от G
            uint32_t rb = (c0 & 0xF81F)
                        + (c1 & 0xF81F)
                        + (((c2 & 0xF81F) + (c3 & 0xF81F)) >> 1);

            uint32_t g  = (c0 & 0x07E0)
                        + (c1 & 0x07E0)
                        + (((c2 & 0x07E0) + (c3 & 0x07E0)) >> 1);

            // деление на 3 без деления
            rb = (rb * 21845u) >> 16;
            g  = (g  * 21845u) >> 16;

            return (uint16_t)((rb & 0xF81F) | (g & 0x07E0));
        }

    protected:
        EVE_esp32&  _vga;
};

