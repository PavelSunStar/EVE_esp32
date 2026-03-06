#pragma once

struct Mode {
    int pclk_hz;
    int hRes, hCycle, hOffset, hSync0, hSync1;
    int vRes, vCycle, vOffset, vSync0, vSync1;
    int div;
};

struct EVE_Screen{
    uint8_t bpp;
    uint16_t maxCol;
	uint16_t width, height;
	uint16_t xx, yy;
	uint16_t cx, cy;
};

struct Video_Buffer{
    uint32_t source;
    uint8_t bpp;
    uint16_t maxCol;
	uint16_t width, height;
    uint16_t lineSize;
	uint16_t xx, yy;
	uint16_t cx, cy;
    uint32_t size, fullSize;
};

struct Viewport{
    uint16_t x1, y1;
    uint16_t x2, y2;
};

struct EVE_Image{
    uint32_t addr;
    uint8_t format;
    int width, height;
    int xx, yy;
    int cx, cy;     
};

