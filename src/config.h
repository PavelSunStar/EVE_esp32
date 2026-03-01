#pragma once

#include <cstdint>
#include "types.h"

typedef struct EveChipInfo {
    uint8_t family;
    uint8_t model;
    uint8_t revision;
    uint8_t build;
};

static const char* modelName(uint8_t m) {
    switch (m) {
        case 0x15: return "BT815";
        case 0x16: return "BT816";
        case 0x17: return "BT817";
        case 0x18: return "BT818";
        default:   return "Unknown";
    }
}

//ESP32-P4 → BT816 header EVE - модуль (BT817/BT818)
struct spiPins {
    int CS;      // (Chip Select) SPI slave select input , active low 
    int PD;      // (Power Down / Reset) Active low power down input.
    int Int;     // Host interrupt open drain output, active low. On board 10kΩ pull-up to 3.3V.
    int MOSI;    // SPI Single mode: SPI MOSI input SPI Dual/Quad mode: SPI data line 0
    int MISO;    // SPI Single mode: SPI MISO output SPI Dual/Quad mode: SPI data line 1
    int IO2;     // SPI Single mode: General purpose IO 0 SPI Quad mode: SPI data line 2
    int IO3;     // SPI Single mode: General purpose IO 1 SPI Quad mode: SPI data line 3
    int SCK;     // CLK	SCK SPI Clock input
};

// esp32-S3
#define GPIO_PD     4  
#define GPIO_INT    5  
#define GPIO_CS     6  
#define SPI_MISO    7 
#define SPI_MOSI    15   
#define SPI_SCK     16   
#define SPI_IO2     17   
#define SPI_IO3     18  
 
/*
// esp32-P4
#define GPIO_CS     21  
#define GPIO_PD     23  
#define GPIO_INT    22  
#define SPI_MOSI    5   
#define SPI_MISO    20  
#define SPI_IO2     4   
#define SPI_IO3     24  
#define SPI_SCK     6 

spiPins pins = {
    .CS   = 10,
    .PD   = 9,
    .Int  = 8,
    .MOSI = 11,   // D0
    .MISO = 13,   // D1
    .IO2  = 12,   // D2
    .IO3  = 14,   // D3
    .SCK  = 7
};

.CS   = 4
.SCK  = 5
.MOSI = 6
.MISO = 15
.IO2  = 16
.IO3  = 17
.PD   = 18
.Int  = 21
*/
static constexpr spiPins evePins() {
    return spiPins{ GPIO_CS, GPIO_PD, GPIO_INT, SPI_MOSI, SPI_MISO, SPI_IO2, SPI_IO3, SPI_SCK };
}

#define QUEUE_SIZE      8
#define DMA_BUFFER_SIZE 16384
#define SPI_HZ          30000000//20000000 // max 30 MHz
#define RAM_DL_MAX_SIZE 0x2000UL // 8192 

#define SCR_BUFFER_SIZE 320*240*2

#define RAM_G_SIZE      (1024*1024L)
#define RAM_DL_SIZE     (8*1024L) 
#define RAM_DL_MASK     (RAM_DL_SIZE - 4)
#define DL_WORDS        (RAM_DL_SIZE >> 2)
#define RAM_CMD_SIZE    (4*1024L)
#define MAX_LINES       1024

#define LUT_SIZE 360

// pclk_hz, hRes, hCycle, hOffset, hSync0, hSync1, vRes, vCycle, vOffset, vSync0, vSync1, div
inline Mode MODE640x480 = {25175000, 640, 800, 144, 0, 96, 480, 525, 35, 0, 2, 3};

