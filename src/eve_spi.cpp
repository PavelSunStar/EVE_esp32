#include <Arduino.h>
#include "eve_spi.h"
#include "BT81X/reg.h"
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"

eve_spi::eve_spi(){

}

eve_spi::~eve_spi(){
    freeMem();
}

void eve_spi::setQuad(bool enable, uint8_t width){ 
    _spiWidth = width;
    _quad = enable; 
}

bool eve_spi::init(uint32_t frequency, uint32_t buffSize, int qSize, const spiPins& pins){
    if (_inited) return true;
    if (frequency == 0 || buffSize == 0 || qSize <= 0) return false;

    Serial.printf("speed: %d, buffSize: %d, qSize: %d\n", frequency, buffSize, qSize);
    Serial.printf("pinCS: %d, pinPD: %d, pinInt: %d, pinMOSI: %d, SPI_MISO: %d, SPI_IO2: %d, SPI_IO3: %d, SPI_SCK: %d\n", 
                    pins.CS, pins.PD, pins.Int, pins.MOSI, pins.MISO, pins.IO2, pins.IO3, pins.SCK);

    _frequency = frequency;
    _buffSize = buffSize;
    _qSize = qSize;
    _pins = pins;

    initPins();
    if (!initBus()) return false;
    if (!initDev()) return false;
    if (!allocDMABuffer()) return false;

    _inited = true;
    return _inited;
}

bool eve_spi::initPins(){
    gpio_config_t io = {};

    // PD — выход, высокий уровень (inactive)
    if (_pins.PD >= 0) {
        io = {};  // обнуляем
        io.mode         = GPIO_MODE_OUTPUT;
        io.pin_bit_mask = (1ULL << _pins.PD);
        io.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io.pull_up_en   = GPIO_PULLUP_DISABLE;
        gpio_config(&io);
        gpio_set_level((gpio_num_t)_pins.PD, 1);
        Serial.printf("PD_N configured: GPIO%d → HIGH\n", _pins.PD);
    } else {
        Serial.println("PD_N not defined → assuming always powered");
    }

    // CS — выход, высокий уровень (inactive)
    if (_pins.CS >= 0) {
        io = {};
        io.mode         = GPIO_MODE_OUTPUT;
        io.pin_bit_mask = (1ULL << _pins.CS);
        io.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io.pull_up_en   = GPIO_PULLUP_DISABLE;
        gpio_config(&io);
        gpio_set_level((gpio_num_t)_pins.CS, 1);
        Serial.printf("CS configured: GPIO%d → HIGH\n", _pins.CS);
    }

    // INT — вход с подтяжкой вверх
    if (_pins.Int >= 0) {
        io = {};
        io.mode         = GPIO_MODE_INPUT;
        io.pull_up_en   = GPIO_PULLUP_ENABLE;
        io.pin_bit_mask = (1ULL << _pins.Int);
        gpio_config(&io);
        Serial.printf("INT configured: GPIO%d (input + pull-up)\n", _pins.Int);
    }

    return true;
}

bool eve_spi::initBus(){
    // SPI bus configuration
    memset(&_busCfg, 0, sizeof(_busCfg));
    _busCfg.sclk_io_num     = _pins.SCK;
    _busCfg.mosi_io_num     = _pins.MOSI;   // IO0
    _busCfg.miso_io_num     = _pins.MISO;   // IO1
    _busCfg.quadwp_io_num   = _pins.IO2;    // IO2
    _busCfg.quadhd_io_num   = _pins.IO3;    // IO3
    _busCfg.max_transfer_sz = _buffSize + 32;

    _busCfg.flags = SPICOMMON_BUSFLAG_MASTER;
    _quadPossible = (_pins.IO2 >= 0 && _pins.IO3 >= 0);
    if (_quadPossible) _busCfg.flags |= SPICOMMON_BUSFLAG_QUAD;

    // Инициализация SPI шины с DMA
    _err = spi_bus_initialize(SPI2_HOST, &_busCfg, SPI_DMA_CH_AUTO);
    if (_err != ESP_OK) {
        Serial.printf("SPI bus init failed: %d\n", _err);
        return false;
    } 
   
    Serial.println("Spi bus init...Ok");
    return true;
}

bool eve_spi::initDev(){
    memset(&_devCfg, 0, sizeof(_devCfg));
    _devCfg.clock_speed_hz      = _frequency;          // например 20000000 (20 МГц) или 40000000 для Quad
    _devCfg.mode                = 0;                   // SPI Mode 0 — обязательно для BT81x/BT816/BT817/BT818
    _devCfg.spics_io_num        = -1; //_pins.CS;      // если -1 → ручное управление CS (часто нужно для Quad)
    _devCfg.queue_size          = _qSize;              // 4–8 нормально, 16 если много асинхронных транзакций

    // Важно для стабильности на высоких частотах (>20 МГц)
    _devCfg.cs_ena_pretrans     = 1;                   // минимум 1 такт CS перед первой передачей
    _devCfg.cs_ena_posttrans    = 1;                   // минимум 1 такт CS после последней передачи

    // Флаги устройства — здесь можно включить HALFDUPLEX или другие, но обычно 0
    _devCfg.flags               = SPI_DEVICE_HALFDUPLEX;   // <-- важно для QIO    

    // Дополнительные полезные параметры (рекомендуется добавить)
    _devCfg.duty_cycle_pos      = 128;                 // 50% duty cycle — стандартно и стабильно
    _devCfg.input_delay_ns      = 0;                   // обычно 0, если нет проблем с таймингом MISO
    _devCfg.post_cb             = nullptr;             // callback после транзакции (можно позже)
    _devCfg.pre_cb              = nullptr;             // callback перед транзакцией

    _err = spi_bus_add_device(SPI2_HOST, &_devCfg, &_spi);
    if (_err != ESP_OK) {
        Serial.printf("SPI add device failed: %d\n", _err);
        spi_bus_free(SPI2_HOST);
        return false;
    }  
    
    Serial.println("SPI config...Ok");
    return true;
}

bool eve_spi::allocDMABuffer(){
    _dmaTxBuf = (uint8_t*)heap_caps_aligned_alloc(32, _buffSize, MALLOC_CAP_DMA);
    _dmaRxBuf = (uint8_t*)heap_caps_aligned_alloc(32, _buffSize, MALLOC_CAP_DMA);
    
    if (!_dmaTxBuf || !_dmaRxBuf) {
        Serial.println("DMA buffer allocation failed");
        freeMem();
        return false;
    }

    Serial.printf("Allocate memmory size: %d...Ok\n", _buffSize);
    return true;
}

void eve_spi::freeMem(){
    if (_dmaTxBuf) {
        heap_caps_free(_dmaTxBuf);
        _dmaTxBuf = NULL;
    }
    if (_dmaRxBuf) {
        heap_caps_free(_dmaRxBuf);
        _dmaRxBuf = NULL;
    }
    
    if (_spi) {
        spi_bus_remove_device(_spi);
        _spi = NULL;
    }
    
    spi_bus_free(SPI2_HOST);
    _inited = false;
}

//========================================================================================
void eve_spi::eveHardReset(){
    if (_pins.PD < 0) return;

    gpio_set_level((gpio_num_t)_pins.PD, 0);
    delay_ms(20);
    gpio_set_level((gpio_num_t)_pins.PD, 1);
    delay_ms(50);
}

void eve_spi::makeHdr(uint32_t addr, bool is_write)noexcept{
    addr = (addr & 0x3FFFFF) | ((uint32_t)is_write << 23);

    _txrxBuf[0] = addr >> 16;
    _txrxBuf[1] = addr >>  8;
    _txrxBuf[2] = addr;
}

void eve_spi::hostCommand(uint8_t cmd, uint8_t param){
    uint8_t buf[3] = { cmd, param, 0x00 };

    CS_Guard lock(*this);

    bool old = _quad;
    _quad = false;
    tx_only(buf, 3);
    _quad = old;
}

uint8_t eve_spi::rd8(uint32_t addr){
    makeHdr(addr, false);

    const uint8_t extra = (_quad && (_spiWidth & 0x04)) ? 1 : 0;
    const uint8_t dummyCount = 1 + extra;

    uint8_t hdr[5];
    hdr[0] = _txrxBuf[0];
    hdr[1] = _txrxBuf[1];
    hdr[2] = _txrxBuf[2];
    hdr[3] = 0x00;
    if (dummyCount == 2) hdr[4] = 0x00;

    CS_Guard lock(*this);

    // TX: addr + dummy
    if (tx_only(hdr, 3 + dummyCount) != ESP_OK) return 0;

    // RX: 1 byte
    uint8_t v = 0;
    if (rx_only(&v, 1) != ESP_OK) return 0;

    return v;
}

uint16_t eve_spi::rd16(uint32_t addr){
    makeHdr(addr, false);

    const uint8_t extra = (_quad && (_spiWidth & 0x04)) ? 1 : 0;
    const uint8_t dummyCount = 1 + extra;

    uint8_t hdr[5];
    hdr[0] = _txrxBuf[0];
    hdr[1] = _txrxBuf[1];
    hdr[2] = _txrxBuf[2];
    hdr[3] = 0x00;                 // dummy
    if (dummyCount == 2) hdr[4] = 0x00;

    uint8_t d[2] = {0, 0};

    CS_Guard lock(*this);

    if (tx_only(hdr, 3 + dummyCount) != ESP_OK) return 0;
    if (rx_only(d, 2) != ESP_OK) return 0;

    return (uint16_t)d[0] |
           ((uint16_t)d[1] << 8);
}

uint32_t eve_spi::rd32(uint32_t addr){
    makeHdr(addr, false);

    const uint8_t extra = (_quad && (_spiWidth & 0x04)) ? 1 : 0;
    const uint8_t dummyCount = 1 + extra;

    uint8_t hdr[5];
    hdr[0] = _txrxBuf[0];
    hdr[1] = _txrxBuf[1];
    hdr[2] = _txrxBuf[2];
    hdr[3] = 0x00;
    if (dummyCount == 2) hdr[4] = 0x00;

    uint8_t d[4] = {0,0,0,0};

    CS_Guard lock(*this);

    if (tx_only(hdr, 3 + dummyCount) != ESP_OK) return 0;
    if (rx_only(d, 4) != ESP_OK) return 0;

    return (uint32_t)d[0] |
           ((uint32_t)d[1] << 8) |
           ((uint32_t)d[2] << 16) |
           ((uint32_t)d[3] << 24);
}

void eve_spi::wr8(uint32_t addr, uint8_t data){
    makeHdr(addr, true);
    _txrxBuf[3] = data;

    CS_Guard lock(*this);
    transfer(_txrxBuf, nullptr, 4);

    if (addr == REG_SPI_WIDTH) _spiWidth = data;
}

void eve_spi::wr16(uint32_t addr, uint16_t data){
    uint8_t buf[5];

    addr = (addr & 0x3FFFFF) | (1UL << 23); // write bit

    buf[0] = addr >> 16;
    buf[1] = addr >> 8;
    buf[2] = addr;
    buf[3] = (uint8_t)(data & 0xFF);
    buf[4] = (uint8_t)((data >> 8) & 0xFF);

    CS_Guard lock(*this);
    tx_only(buf, sizeof(buf));
}

void eve_spi::wr32(uint32_t addr, uint32_t data){
    uint8_t buf[7];
    addr = (addr & 0x3FFFFF) | (1UL << 23); // write bit

    buf[0] = addr >> 16;
    buf[1] = addr >> 8;
    buf[2] = addr;
    buf[3] = (uint8_t)(data & 0xFF);
    buf[4] = (uint8_t)((data >> 8) & 0xFF);
    buf[5] = (uint8_t)((data >> 16) & 0xFF);
    buf[6] = (uint8_t)((data >> 24) & 0xFF);

    CS_Guard lock(*this);
    tx_only(buf, sizeof(buf));
}

void eve_spi::transfer(const uint8_t* txData, uint8_t* rxData, uint32_t len){
    if (!len) return;

    if ((_devCfg.flags & SPI_DEVICE_HALFDUPLEX) && txData && rxData) {
        Serial.printf("HALFDUPLEX ERROR: tx+rx len=%u\n", (unsigned)len);
        _err = ESP_ERR_INVALID_ARG;
        return;
    }

    spi_transaction_t t{};
    t.length   = len * 8;
    t.tx_buffer = txData;
    t.rx_buffer = rxData;
    t.flags = 0;
    if (_quad) t.flags |= SPI_TRANS_MODE_QIO;

    _err = spi_device_transmit(_spi, &t);
    if (_err != ESP_OK) {
        Serial.printf("spi_device_transmit err=%d len=%u tx=%p rx=%p\n",
                      _err, (unsigned)len, txData, rxData);
    }
}

esp_err_t eve_spi::tx_only(const void* data, size_t bytes) {
    spi_transaction_t t{};
    t.length   = bytes * 8;     // TX bits
    t.rxlength = 0;             // RX bits
    t.tx_buffer = data;
    t.rx_buffer = nullptr;
    t.flags = 0;
    if (_quad) t.flags |= SPI_TRANS_MODE_QIO;

    //Serial.printf("TXONLY bytes=%u flags=0x%X tx=%p rx=%p\n", (unsigned)bytes, (unsigned)t.flags, t.tx_buffer, t.rx_buffer);

    return spi_device_transmit(_spi, &t);
}

esp_err_t eve_spi::rx_only(void* data, size_t bytes) {
    spi_transaction_t t{};
    t.length   = 0;             // TX bits = 0 (важно!)
    t.rxlength = bytes * 8;     // RX bits
    t.tx_buffer = nullptr;
    t.rx_buffer = data;
    t.flags = 0;
    if (_quad) t.flags |= SPI_TRANS_MODE_QIO;

    //Serial.printf("RXONLY bytes=%u flags=0x%X tx=%p rx=%p\n", (unsigned)bytes, (unsigned)t.flags, t.tx_buffer, t.rx_buffer);

    return spi_device_transmit(_spi, &t);
}

bool eve_spi::wrMem(uint32_t addr, const void* src, size_t len){
    if (!_inited || !src || len == 0) return false;

    const uint8_t* p = (const uint8_t*)src;
    const size_t maxPayload = (size_t)_buffSize - 3;

    while (len) {
        size_t chunk = (len > maxPayload) ? maxPayload : len;

        if (!wrMemChunk(addr, p, chunk))
            return false;

        addr += (uint32_t)chunk;
        p    += chunk;
        len  -= chunk;
    }
    return true;
}

bool eve_spi::wrMemChunk(uint32_t addr, const void* src, size_t len){
    if (!src || len == 0) return true;

    const size_t maxPayload = (size_t)_buffSize - 3;
    if (len > maxPayload) {
        Serial.printf("wrMemChunk too big: %u > %u\n", (unsigned)len, (unsigned)maxPayload);
        return false;
    }

    // write header (3 bytes)
    addr = (addr & 0x3FFFFF) | (1UL << 23);
    _dmaTxBuf[0] = (uint8_t)(addr >> 16);
    _dmaTxBuf[1] = (uint8_t)(addr >> 8);
    _dmaTxBuf[2] = (uint8_t)(addr);

    memcpy(_dmaTxBuf + 3, src, len);

    spi_transaction_t t{};
    t.length    = (len + 3) * 8; // TX bits
    t.rxlength  = 0;
    t.tx_buffer = _dmaTxBuf;
    t.rx_buffer = nullptr;
    t.flags     = 0;
    if (_quad) t.flags |= SPI_TRANS_MODE_QIO;

    CS_Guard lock(*this);

    _err = spi_device_transmit(_spi, &t);
    if (_err != ESP_OK) {
        Serial.printf("wrMemChunk transmit err=%d\n", _err);
        return false;
    }
    return true;
}

bool eve_spi::rdMem(uint32_t addr, void* dst, size_t len){
    if (!_inited || !dst || len == 0) return false;

    uint8_t* p = (uint8_t*)dst;
    const size_t maxChunk = (size_t)_buffSize;

    while (len) {
        size_t chunk = (len > maxChunk) ? maxChunk : len;

        if (!rdMemChunk(addr, p, chunk))
            return false;

        addr += (uint32_t)chunk;
        p    += chunk;
        len  -= chunk;
    }
    return true;
}

bool eve_spi::rdMemChunk(uint32_t addr, void* dst, size_t len){
    if (!dst || len == 0) return true;

    // Чтобы не упереться в ограничения драйвера/буфера — читаем не больше _buffSize за раз
    const size_t maxChunk = (size_t)_buffSize;
    if (len > maxChunk) {
        Serial.printf("rdMemChunk too big: %u > %u\n", (unsigned)len, (unsigned)maxChunk);
        return false;
    }

    makeHdr(addr, false);

    const uint8_t extra = (_quad && (_spiWidth & 0x04)) ? 1 : 0;
    const uint8_t dummyCount = 1 + extra;

    uint8_t hdr[5];
    hdr[0] = _txrxBuf[0];
    hdr[1] = _txrxBuf[1];
    hdr[2] = _txrxBuf[2];
    hdr[3] = 0x00;
    if (dummyCount == 2) hdr[4] = 0x00;

    CS_Guard lock(*this);

    // 1) TX header + dummy
    if (tx_only(hdr, 3 + dummyCount) != ESP_OK) return false;

    // 2) RX bulk data
    if (rx_only(dst, len) != ESP_OK) return false;

    return true;
}
