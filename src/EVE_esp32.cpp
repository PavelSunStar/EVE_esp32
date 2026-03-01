#include "EVE_esp32.h"

EVE_esp32::EVE_esp32(){
    for (int i = 0; i < LUT_SIZE; ++i) {
        sinLUT[i] = (int16_t)(1000.0 * sin(i * DEG_TO_RAD));
        cosLUT[i] = (int16_t)(1000.0 * cos(i * DEG_TO_RAD));
    } 
}

EVE_esp32::~EVE_esp32(){
    freeMem();
}

bool EVE_esp32::boot(){
    if (_initedBoot) return true;  

    _spi.eveHardReset();

    uint8_t multiplier = 6;
    multiplier &= 0x3F;
    if (multiplier >= 4 && multiplier <= 6) {
        multiplier |= (1UL << 6);                      // биты [7:6] = 01
    }
    _spi.hostCommand(CLKEXT, 0x00); // CLKINT
    _spi.hostCommand(CLKSEL, multiplier);//multiplier);

    _spi.hostCommand(RST_PULSE, 0x00);
    _spi.hostCommand(ACTIVE,    0x00);
    delay_ms(300);

    uint32_t t0 = millis();
    while (_spi.rd8(REG_ID) != 0x7C) {
        if (millis() - t0 > 2000) {
            Serial.println("Timeout: REG_ID not ready");
            return false;
        }
        delay_ms(10);
    }

    t0 = millis();
    while ((_spi.rd8(REG_CPURESET) & 0x07) != 0 ) {
        if (millis() - t0 > 2000) {
            Serial.println("Timeout: REG_CPURESET not 0");
            return false;
        }
        delay_ms(10);
    }

    // Дополнительно: ждём, пока clock стабилизируется
    uint32_t clk1 = _spi.rd32(REG_CLOCK);
    delay(100);
    uint32_t clk2 = _spi.rd32(REG_CLOCK);
    if (clk2 <= clk1) {
        Serial.println("Clock not ticking");
        return false;
    }

    Serial.println("Boot OK, chip ready");

    uint32_t chip = _spi.rd32(CHIPID_ADDR);
    _chipInfo.family   = (chip >> 0)  & 0xFF;
    _chipInfo.model    = (chip >> 8)  & 0xFF;
    _chipInfo.revision = (chip >> 16) & 0xFF;
    _chipInfo.build    = (chip >> 24) & 0xFF;
    Serial.printf("Model: BT%d%02X, Rev: %02X, Build: %02X\n", _chipInfo.family, _chipInfo.model, _chipInfo.revision, _chipInfo.build);

    _initedBoot = true;
    return _initedBoot;
}

bool EVE_esp32::init(Mode &m, bool setQSPI){
    if (_inited) return true;

    _mode = m;
    Serial.println("\n[=== Init EVE ===]");
    if (!_spi.init()) return false;
    if (!boot()) return false;

    if (setQSPI && _spi.QuadPossible()){
        _spi.setQuad(false, 0x00);          // <--- обязательно: host single
        uint8_t w0 = _spi.rd8(REG_SPI_WIDTH);
        Serial.printf("SPI_WIDTH(before)=0x%02X\n", w0);
        Serial.printf("ID(before)=0x%02X\n", _spi.rd8(REG_ID));

        _spi.wr8(REG_SPI_WIDTH, 0x06);      // записываем в single
        delay(2);

        _spi.setQuad(true, 0x06);           // <--- теперь host quad
        uint8_t w1 = _spi.rd8(REG_SPI_WIDTH); // читаем уже quad-ом
        uint8_t id1 = _spi.rd8(REG_ID);

        Serial.printf("SPI_WIDTH(after)=0x%02X\n", w1);
        Serial.printf("ID(quad)=0x%02X\n", id1);

        if (w1 != 0x06 || id1 != 0x7C) {
            Serial.println("QSPI switch failed -> fallback to SINGLE");
            _spi.setQuad(false, 0x00);
        }
    }

    initVGA(); 
    setScrParam();

    return _inited = true;
}

bool EVE_esp32::createVideoBuffer(uint8_t bpp, bool backGroung){
    if (_vbInited) return true;

    // Virtual screen buffer param
    _vbCfg.bpp         = bpp;
    _vbCfg.width       = _scr.width >> 1;
    _vbCfg.height      = _scr.height >> 1;
    _vbCfg.lineSize    = _vbCfg.width * (_vbCfg.bpp == 16 ? 2 : 1);
    _vbCfg.xx          = _vbCfg.width - 1;
    _vbCfg.yy          = _vbCfg.height - 1;
    _vbCfg.cx          = _vbCfg.width >> 1;
    _vbCfg.cy          = _vbCfg.height >> 1;
    _vbCfg.size        = _vbCfg.width * _vbCfg.height;
    _vbCfg.fullSize    = _vbCfg.size * (_vbCfg.bpp == 16 ? 2 : 1);

    _vpBuf.x1 = 0;
    _vpBuf.y1 = 0;
    _vpBuf.x2 = _vbCfg.xx;
    _vpBuf.y2 = _vbCfg.yy;
    
    
    scrBuf = (uint8_t*)ps_malloc(_vbCfg.fullSize);
    if (!scrBuf){
        Serial.println("Can not allocate screen buffer");
        return false;
    } else {
        if (_vbCfg.bpp == 16){
            uint16_t *line = (uint16_t*)scrBuf;
                
            for (int i = 0; i < _vbCfg.height; i++) {
                lineBuf16[i] = line;
                line += _vbCfg.width; 
            }  
        } else {
            uint8_t *line = scrBuf;
                
            for (int i = 0; i < _vbCfg.height; i++) {
                lineBuf8[i] = line;
                line += _vbCfg.width;
            }  
        } 

        memset(scrBuf, 0, _vbCfg.fullSize);  
        _vbCfg.source = RAM_G + RAM_G_SIZE - _vbCfg.fullSize;      
        _spi.wrMem(_vbCfg.source, scrBuf, _vbCfg.fullSize);
    }    

    if (backGroung){ 
        bg = (uint8_t*)ps_malloc(_vbCfg.fullSize);
        if (!bg){
            Serial.println("Can not allocate back ground buffer");
            return false;
        } else {
            _bgInited = true;
            memset(bg, 0, _vbCfg.fullSize);
        }
    } 

    return _vbInited = true;
}

void EVE_esp32::freeMem(){
    if (scrBuf) {
        heap_caps_free(scrBuf);
        scrBuf = nullptr;
    }  
    
    if (bg){
        heap_caps_free(bg);
        bg = nullptr;
    }
}

void EVE_esp32::setFreq(){
    // 1) sanity-check: REG_CLOCK должен увеличиваться
    uint32_t cycles1 = _spi.rd32(REG_CLOCK);
    delay(50);
    uint32_t cycles2 = _spi.rd32(REG_CLOCK);

    uint32_t delta50 = cycles2 - cycles1;
    //Serial.printf("REG_CLOCK delta in 50ms: %lu\n", delta50);

    // 2) прочитать текущее состояние регистров
    uint32_t cycles = _spi.rd32(REG_CLOCK);        // cycles since reset (НЕ Hz)
    uint32_t freqHz = _spi.rd32(REG_FREQUENCY);    // main clock frequency in Hz (номинал)

    //Serial.printf("Clock cycles since reset: %lu\n", cycles);
    //Serial.printf("REG_FREQUENCY (before): %lu Hz\n", freqHz);

    // 3) измерить реальную частоту (по REG_CLOCK за интервал времени)
    double measuredHz = measureClockHz(200);       // 200ms обычно достаточно
    //Serial.printf("Measured clock: %.0f Hz\n", measuredHz);

    // 4) записать измеренную частоту в REG_FREQUENCY (чтобы EVE/драйвер знали реальную)
    uint32_t f = (uint32_t)(measuredHz + 0.5);
    _spi.wr32(REG_FREQUENCY, f);

    // 5) перечитать и вывести итог
    uint32_t freq_now = _spi.rd32(REG_FREQUENCY);
    //Serial.printf("REG_FREQUENCY updated: %lu Hz\n", freq_now);
}

double EVE_esp32::measureClockHz(uint32_t ms){
    uint32_t c0 = _spi.rd32(REG_CLOCK);
    uint32_t t0 = micros();

    delay(ms);

    uint32_t c1 = _spi.rd32(REG_CLOCK);
    uint32_t t1 = micros();

    uint32_t dc = c1 - c0;
    uint32_t dt_us = t1 - t0;

    if (dt_us == 0) return 0.0;

    return (double)dc * 1000000.0 / (double)dt_us;
}

void EVE_esp32::setScrParam(){
    //Eve param
    _scr.bpp        = 16;
    _scr.width      = _mode.hRes;
    _scr.height     = _mode.vRes;
    _scr.xx         = _scr.width - 1;
    _scr.yy         = _scr.height - 1;
    _scr.cx         = _scr.width >> 1;
    _scr.cy         = _scr.height >> 1;

    _vp.x1 = 0;
    _vp.y1 = 0;
    _vp.x2 = _scr.xx;
    _vp.y2 = _scr.yy; 
}

void EVE_esp32::initVGA(){
    // PCLK off while configuring
    _spi.wr8(REG_PCLK, 0);

    // (опционально, но полезно)
    _spi.wr8(REG_SWIZZLE, 0);
    _spi.wr8(REG_CSPREAD, 1);
    _spi.wr8(REG_DITHER,  1);
    _spi.wr8(REG_ROTATE,  0);
    _spi.wr8(REG_PCLK_POL, 0);
    //_spi.wr32(REG_OUTBITS, OUTBITS(5,6,5));  // можно wr8/wr16/wr32 — регистр 9 бит

    _spi.wr32(REG_HSIZE,    _mode.hRes);    // Количество пикселей в видимой области по горизонтали (640 пикселей).     
    _spi.wr32(REG_HCYCLE,   _mode.hCycle);  // Общее количество пикселей на строку, включая видимую область, фронт-порч, синхропульс и бэк-порч (800 пикселей).  
    _spi.wr32(REG_HOFFSET,  _mode.hOffset); // Смещение начала отображения пикселей по горизонтали: 16 пикселей фронт-порч + 96 пикселей синхропульс + 48 пикселей бэк-порч = 160 пикселей.  
    _spi.wr32(REG_HSYNC0,   _mode.hSync0);  // Момент, когда начинается падение синхросигнала (16 пикселей от начала строки).  
    _spi.wr32(REG_HSYNC1,   _mode.hSync1);  // Момент, когда происходит подъём синхросигнала: 16 пикселей + ширина синхропульса (96 пикселей) = 112 пикселей.  

    // ==========================  
    // Настройки вертикальной синхронизации  
    // ==========================  
    _spi.wr32(REG_VSIZE,    _mode.vRes);    // Количество строк в видимой области экрана (480 строк).  
    _spi.wr32(REG_VCYCLE,   _mode.vCycle);  // Общее количество строк на кадр: видимые строки + фронт-порч, синхропульс и бэк-порч (525 строк).  
    _spi.wr32(REG_VOFFSET,  _mode.vOffset); // Смещение начала отображения вертикальных строк: 10 строк фронт-порч + 2 строки синхропульс + 33 строки бэк-порч = 45 строк.  
    _spi.wr32(REG_VSYNC0,   _mode.vSync0);  // Момент, когда начинается падение вертикального синхросигнала (10 строк от начала поля).  
    _spi.wr32(REG_VSYNC1,   _mode.vSync1);  // Момент подъёма вертикального синхросигнала: 10 строк + ширина синхропульса (2 строки) = 12 строк.  

    _spi.wr32(RAM_DL + 0, CLEAR_COLOR_RGB(0, 255, 0));
    _spi.wr32(RAM_DL + 4, CLEAR(1, 1, 1));
    _spi.wr32(RAM_DL + 8, DL_DISPLAY());

    // swap display list
    _spi.wr8(REG_DLSWAP, DLSWAP_FRAME);
    while (_spi.rd8(REG_DLSWAP) != 0) {}

    _spi.wr8(REG_PCLK, _mode.div);
}

//=================================================
/*
void EVE_esp32::dlStart(uint8_t r, uint8_t g, uint8_t b){
    _dlPos = 0;

    dl(RESTORE_CONTEXT());          // <-- КЛЮЧЕВО: сбросить все состояния

    dl(CLEAR_COLOR_RGB(r, g, b));
    dl(CLEAR(1, 1, 1));

    // полезно явно
    dl(COLOR_RGB(255,255,255));
    dl(COLOR_A(255));
    dl(VERTEX_FORMAT(_1_16));       // если у тебя макросы такие же
}

uint16_t EVE_esp32::dl(uint32_t cmd){
    uint16_t pos = _dlPos;

    if (_dlPos > 8192 - 4) {
        Serial.println("DL overflow");
        return pos;
    }

    _spi.wr32(RAM_DL + _dlPos, cmd);
    _dlPos += 4;

    return pos;
}

uint16_t EVE_esp32::dl(uint16_t pos, uint32_t cmd){
    if (((pos & 3) != 0) || (pos > 8192 - 4)) return 0xFFFF;
    _spi.wr32(RAM_DL + pos, cmd);

    return pos;
}


void EVE_esp32::dlEnd(){
    dl(DL_DISPLAY());
}
*/
void EVE_esp32::swap(){
    //Serial.println(_dlPos);
    // ЖДЁМ, пока прошлый swap завершён
    while (_spi.rd8(REG_DLSWAP) != 0) {}

    // И только потом просим swap
    _spi.wr8(REG_DLSWAP, DLSWAP_FRAME);
}

//========================================
void EVE_esp32::dlNewList(){
    _dlPos = 0;
}

void EVE_esp32::dlStart(uint8_t r, uint8_t g, uint8_t b){
    dlCls(r, g, b);

    if (_vbInited){
        dl(BITMAP_HANDLE(0));
        dl(BITMAP_SOURCE(_vbCfg.source));
        dl(BITMAP_LAYOUT(RGB565, 320*2, 240));
        dl(BITMAP_LAYOUT_H(0,0));
        dl(BITMAP_SIZE(0, 0, 0, 640, 480));
        dl(BITMAP_SIZE_H(1, 0));
        dl(BITMAP_TRANSFORM_A(128));
        dl(BITMAP_TRANSFORM_E(128));
        dl(BEGIN(BITMAPS));
        dl(VERTEX2F(0, 0));
        dl(END());

        dl(BITMAP_TRANSFORM_A(256));
        dl(BITMAP_TRANSFORM_E(256));
    }
}

void EVE_esp32::dlCls(uint8_t r, uint8_t g, uint8_t b){
    dl(CLEAR_COLOR_RGB(r, g, b));
    dl(CLEAR(1, 1, 1));
}

uint32_t EVE_esp32::dl(uint32_t cmd){
    if (_dlPos >= DL_WORDS){
        Serial.println("DL overflow");
        return UINT16_MAX;
    }

    _dlBuf[_dlPos++] = cmd;

    return _dlPos;
}

uint32_t EVE_esp32::dl(uint32_t pos, uint32_t cmd){
    if (pos >= DL_WORDS) {
        Serial.println("DL invalid write or overflow");
        return UINT32_MAX;
    }

    _dlBuf[pos] = cmd;
    return pos;
}

inline uint32_t EVE_esp32::dlFast(uint32_t pos, uint32_t cmd){
    _dlBuf[pos] = cmd;
    return pos;
}

void EVE_esp32::dlEnd(){
    dl(DL_DISPLAY());
    _spi.wrMem(RAM_DL, _dlBuf, _dlPos << 2);
}

void EVE_esp32::dlCopyList(){
    swap();
    _spi.wrMem(RAM_DL, _dlBuf, _dlPos << 2);
}

//While(rd16(REG_CMD_WRITE) != rd16(REG_CMD_READ)); //Wait till the compression was done
// [=== CMD ===]
void EVE_esp32::cmd(uint32_t data){
    if (_cmdFirst){
        _cmdMemPos = 0;
        _cmdFirst = false;
        _cmdPos = (uint16_t)(_spi.rd32(REG_CMD_READ) & 0xFFF);

        cmd32(CMD_DLSTART);
        cmd32(data);
    } else {
        cmd32(data);
    }
} 

void EVE_esp32::cmd8(uint8_t data) {
    if (_cmdMemPos + 1 < _cmdSize) {
        _cmdBuf[_cmdMemPos++] = data;
    } else {
        Serial.println("CMD buffer overflow (8-bit)");
    }
}

void EVE_esp32::cmd16(uint16_t data){
    if (_cmdMemPos + 2 < _cmdSize){
        _cmdBuf[_cmdMemPos++] = (uint8_t)(data & 0xFF);
        _cmdBuf[_cmdMemPos++] = (uint8_t)(data >> 8) & 0xFF;
    } else {
        Serial.println("CMD buffer overflow (16-bit)");
    }
}

void EVE_esp32::cmd32(uint32_t data) {
    if (_cmdMemPos + 4 < _cmdSize) {
        _cmdBuf[_cmdMemPos++] = (uint8_t)(data & 0xFF);
        _cmdBuf[_cmdMemPos++] = (uint8_t)(data >> 8) & 0xFF;
        _cmdBuf[_cmdMemPos++] = (uint8_t)(data >> 16) & 0xFF;
        _cmdBuf[_cmdMemPos++] = (uint8_t)(data >> 24) & 0xFF;
    } else {
        Serial.println("CMD buffer overflow (32-bit)");
    }
}

bool EVE_esp32::sendData(uint32_t addr, const void* src, size_t len){
    if (!src || len <= 0) return false;
    return _spi.wrMem(addr, src, len);
}

void EVE_esp32::sendScr(){
    if (_vbInited) _spi.wrMem(_vbCfg.source, scrBuf, _vbCfg.fullSize);
}

void EVE_esp32::LUT(int &x, int &y, int xx, int yy, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    x = xx + (len * cosLUT[angle]) / 1000;
    y = yy + (len * sinLUT[angle]) / 1000;
}

int EVE_esp32::xLUT(int x, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    return x + (len * cosLUT[angle]) / 1000;
}

int EVE_esp32::yLUT(int y, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    return y + (len * sinLUT[angle]) / 1000;
}

/*
void EVE_esp32::memcpyEVE(uint32_t dst, uint32_t src, uint32_t num)
{
    cmdBegin();
    cmd32(CMD_MEMCPY);
    cmd32(dst);
    cmd32(src);
    cmd32(num);
    cmdEnd();
    cmdWait();
}

//scrBuf = (uint8_t*)heap_caps_aligned_alloc(32, _vbCfg.fullSize * (dBuff ? 2 : 1), MALLOC_CAP_DMA);
*/

