#pragma once

#include "config.h"
#include "BT81X/reg.h"
#include "BT81X/host_cmd.h"
#include "BT81X/dl.h"
#include "BT81X/cmd.h"
#include "eve_spi.h"
#include "types.h"
#include "eve_utils.h"

class EVE_esp32{
    public:
        uint8_t*    bg = nullptr;
        uint8_t*    scrBuf = nullptr;
        uint8_t*    tmpBuf = nullptr;
        uint8_t*    lineBuf8[MAX_LINES];
        uint16_t*   lineBuf16[MAX_LINES];
        
        EVE_esp32();
        ~EVE_esp32();

        bool init(Mode &m = MODE640x480, bool setQSPI = true);
        bool createVideoBuffer(uint8_t bpp = 16, bool backGroung = false);

        // DL List
        void            dlNewList();
        void            dlStart(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);
        void            dlCls(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);        
        uint32_t        dl(uint32_t cmd);
        uint32_t        dl(uint32_t pos, uint32_t cmd);
        inline uint32_t dlFast(uint32_t pos, uint32_t cmd);       
        void            dlEnd();
        void            dlCopyList();

        //CMD FIFO
        void cmd(uint32_t data);
        void cmd8(uint8_t data);
        void cmd16(uint16_t data);
        void cmd32(uint32_t data);

        void sendScr();
        bool sendData(uint32_t addr, const void* src, size_t len);
        void swap();

        //Screen
        int BPP()       { return _scr.bpp; }
        int Width()     { return _scr.width; }
        int Height()    { return _scr.height; }
        int XX()        { return _scr.xx; }
        int YY()        { return _scr.yy; }
        int CX()        { return _scr.cx; }
        int CY()        { return _scr.cy; }

        // Virtual buffer screen
        bool vbInited() { return _vbInited; }
        int vbBPP()     { return _vbCfg.bpp; }
        int vbWidth()   { return _vbCfg.width; }
        int vbHeight()  { return _vbCfg.height; }
        int vbLineSize(){ return _vbCfg.lineSize; }
        int vbXX()      { return _vbCfg.xx; }
        int vbYY()      { return _vbCfg.yy; } 
        int vbCX()      { return _vbCfg.cx; }
        int vbCY()      { return _vbCfg.cy; }
        int vbSize()    { return _vbCfg.size; }
        int vbFullSize(){ return _vbCfg.fullSize; }

        //Screen viewport
        int vX1()       {return _vp.x1;};
        int vY1()       {return _vp.y1;};
        int vX2()       {return _vp.x2;};
        int vY2()       {return _vp.y2;};
                
        // Virtual buffer viewport
        int vbvX1()     { return _vpBuf.x1; }
        int vbvY1()     { return _vpBuf.y1; }
        int vbvX2()     { return _vpBuf.x2; }
        int vbvY2()     { return _vpBuf.y2; }

        int Shift()     { return 1; } // BPP 16

        void LUT(int &x, int &y, int xx, int yy, int len, int angle);
        int xLUT(int x, int len, int angle);
        int yLUT(int y, int len, int angle);  

    private:
        uint32_t    _dlBuf[DL_WORDS] = {};
        uint8_t     _cmdBuf[RAM_CMD_SIZE] = {};

        bool boot();
        void initVGA();
        void setScrParam();
        void setFreq();
        double measureClockHz(uint32_t ms);
        void freeMem();

        // Eve video mode
        Mode        _mode;
        EVE_Screen  _scr;
        Viewport    _vp;
        bool        _inited = false;
        bool        _initedBoot = false;

        // Virtual screen
        bool            _vbInited = false;
        bool            _bgInited = false;
        uint32_t        _bgPos;
        Video_Buffer    _vbCfg;
        Viewport        _vpBuf;

        // DL
        uint32_t        _dlPos = 0;
        uint32_t        _dlMemPos = 0;

        //CMD
        bool            _cmdFirst = true;
        uint16_t        _cmdPos;
        uint16_t        _cmdMemPos = 0;
        uint16_t        _cmdSize = RAM_CMD_SIZE;

        EveChipInfo _chipInfo;        
        eve_spi     _spi;

        int16_t sinLUT[LUT_SIZE];
        int16_t cosLUT[LUT_SIZE];
};