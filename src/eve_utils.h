#pragma once

#define bswap16(x) __builtin_bswap16(x)
#define bswap32(x) __builtin_bswap32(x)

#define PTR_OFFSET(ptr, offset)         ((void*)((uint8_t*)(ptr) + (offset)))
#define PTR_OFFSET_T(ptr, offset, type) ((type*)((uint8_t*)(ptr) + (offset)))

static float fps = 0;
static uint32_t frameCount      = 0;        // ISR
static uint32_t fpsStartTime    = millis(); // μs
static uint32_t timer           = 0;        // vsync counter

static inline void delay_ms(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

static inline void updateFPS() {
    frameCount++;
    uint32_t now = millis();
    if (now - fpsStartTime >= 1000) {
        fps = frameCount * 1000.0f / (now - fpsStartTime);
        frameCount = 0;
        fpsStartTime = now;
    }
}

// ---- 8 bit (RGB332) ----
inline uint8_t RGB8(uint8_t r, uint8_t g, uint8_t b) { return ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6); }
inline uint8_t R8(uint8_t c) { return (c >> 5) & 0x07; }
inline uint8_t G8(uint8_t c) { return (c >> 2) & 0x07; }
inline uint8_t B8(uint8_t c) { return  c       & 0x03; }

// ---- 16 bit (RGB565) ----
inline uint16_t RGB16(uint8_t r, uint8_t g, uint8_t b) { return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3); }
inline uint16_t R16(uint16_t c) { return (c >> 11) & 0x1F; }
inline uint16_t G16(uint16_t c) { return (c >> 5)  & 0x3F; }
inline uint16_t B16(uint16_t c) { return  c        & 0x1F; }


// Matrix
static constexpr const float deg_to_rad = 0.017453292519943295769236907684886;
static constexpr const uint8_t FP_SCALE = 16;
#define FP_ONE   (1 << FP_SCALE)

//size_t psRamSize() { return ESP.getPsramSize(); }
/*
static uint8_t testBuf[8189];
size_t bytes = sizeof(testBuf);
    for (int i = 0; i < 2048; i++)
        testBuf[i] = 0x12345678;

    uint32_t t0 = micros();
    _spi.wrMem(RAM_G, testBuf, bytes);
    uint32_t dt = micros() - t0;

    double mbps = (double)bytes / (double)dt; // bytes per microsecond
    Serial.printf("wrMem: %u bytes in %u us => %.2f MB/s\n",
                  bytes, dt, mbps);
*/



