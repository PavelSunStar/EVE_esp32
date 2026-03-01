#pragma once

#include <Arduino.h>
#include "config.h"
#include "eve_utils.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "BT81X/reg.h"

class eve_spi{
    public:
        inline spi_device_handle_t bus() const { return _spi; }

        eve_spi();
        ~eve_spi();

        bool QuadPossible() { return _quadPossible; }
        uint32_t Freq()     { return _frequency; }
        uint32_t QSize()    { return _qSize;}
        int BuffSize()      { return _buffSize;}

        void eveHardReset(); 
        void setQuad(bool enable, uint8_t width);       
        bool init(uint32_t speed = SPI_HZ, uint32_t buffSize = DMA_BUFFER_SIZE, int qSize = QUEUE_SIZE, const spiPins& pins = evePins());

        void makeHdr(uint32_t addr, bool is_write)noexcept;
        uint8_t     rd8(uint32_t addr);
        uint16_t    rd16(uint32_t addr);
        uint32_t    rd32(uint32_t addr);
        void        wr8(uint32_t addr, uint8_t data);
        void        wr16(uint32_t addr, uint16_t data);
        void        wr32(uint32_t addr, uint32_t data);
        void hostCommand(uint8_t cmd, uint8_t param);

        bool rdMem(uint32_t addr, void* dst, size_t len);
        bool wrMem(uint32_t addr, const void* src, size_t len);
        bool rdMemChunk(uint32_t addr, void* dst, size_t len); 
        bool wrMemChunk(uint32_t addr, const void* src, size_t len);       
        void transfer(const uint8_t* txData, uint8_t* rxData, uint32_t len);

        inline void csLo(){ gpio_set_level((gpio_num_t)_pins.CS, 0); }
        inline void csHi(){ gpio_set_level((gpio_num_t)_pins.CS, 1); }

    private:
        bool        _inited = false;
        uint8_t*    _dmaTxBuf = nullptr;
        uint8_t*    _dmaRxBuf = nullptr;
        uint8_t     _txrxBuf[8] = {};

        bool initPins();
        bool initBus();
        bool initDev();
        bool allocDMABuffer();
        void freeMem();

        esp_err_t tx_only(const void* data, size_t bytes);
        esp_err_t rx_only(void* data, size_t bytes);

        bool        _quad = false;
        uint8_t     _spiWidth = 0;
        bool        _quadPossible = false;

        uint32_t    _frequency = 0;
        uint32_t    _qSize = 0;
        int         _buffSize = 0;
        spiPins     _pins;

        esp_err_t                       _err;
        spi_bus_config_t                _busCfg;
        spi_device_interface_config_t   _devCfg;
        spi_host_device_t               _host = SPI2_HOST;
        spi_device_handle_t             _spi = nullptr;  
};

class CS_Guard {
    public:
        explicit CS_Guard(eve_spi& s) : spi(s) { spi.csLo(); }
        ~CS_Guard()                     { spi.csHi(); }

    eve_spi& spi;
};

/*Использование:
C++void swap() {
    DLSwapGuard guard(_spi);   // swap начнётся и завершится автоматически
    // можно сразу начинать готовить следующий кадр
}*/
class DLSwapGuard {
    public:
        explicit DLSwapGuard(eve_spi& s) : spi(s) {
            spi.wr8(REG_DLSWAP, DLSWAP_FRAME);
        }

        ~DLSwapGuard() {
            uint32_t start = millis();
            while (spi.rd8(REG_DLSWAP) != 0) {
                if (millis() - start > 500) {
                    Serial.println("DLSWAP timeout!");
                    break;
                }
                delay(1);
            }
        }

    eve_spi& spi;        
};


/*Блокировка шины на время критичной последовательности
(если в будущем будешь использовать несколько устройств на одной SPI-шине)
*/
class SpiBusLock {
    spi_device_handle_t dev;
public:
    explicit SpiBusLock(spi_device_handle_t d) : dev(d) { spi_device_acquire_bus(dev, portMAX_DELAY); }
    ~SpiBusLock() { spi_device_release_bus(dev); }
};


        //inline void csLo(){ if (_cs_count++ == 0) gpio_set_level((gpio_num_t)_pins.CS, 0); }
        //inline void csHi(){ if (_cs_count > 0 && --_cs_count == 0) gpio_set_level((gpio_num_t)_pins.CS, 1); }