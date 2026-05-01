#pragma once

#include <Arduino.h>
#include <algorithm>
#include "config.h"
#include "Image.h"
#include "structures.h"

//#include "esp_psram.h"
//#include "esp_heap_caps.h"
#include "esp_lcd_panel_rgb.h"
//#include "esp_private/esp_cache_private.h"

#if defined(CONFIG_IDF_TARGET_ESP32P4) || defined(ARDUINO_ESP32P4_DEV)
    #define IS_P4 1
    #include "driver/ppa.h"
#else
    #define IS_P4 0
#endif

class VGA_esp32{
    friend class GFX;
    friend class Sprite;
    friend class Tiles;
    friend class Font_def;
    friend class Palette;

    public:
        VGA_esp32();
        ~VGA_esp32();

        uint32_t T(){ return _t;} //test speed

        uint8_t* Buffer8(int y, int x) {
            if (!_inited || !_scr.buf || _scr.bpp != _8BIT) return nullptr;
            if ((unsigned)x >= (unsigned)_scr.width) return nullptr;
            if ((unsigned)y >= (unsigned)_scr.height) return nullptr;

            return _scr.buf + _backBuf + y * _scr.lineSize + x;
        }    
        
        uint16_t* Buffer16(int y, int x) {
            return (!_inited ? nullptr : (&_scr.line16[y][x] + _backBuf));
        }         

        float FPS()         { return _fps; }
        uint32_t Timer()    { return _timer; }

        //Screen 
        int BPP()       { return _scr.bpp; }
        int Width()     { return _scr.width; }
        int Height()    { return _scr.height; }
        int MaxX()      { return _scr.maxX; }
        int MaxY()      { return _scr.maxY; }
        int CX()        { return _scr.cx; }
        int CY()        { return _scr.cy; }

        //Screen viewport
        int vX1()       {return _scr.x0;}
        int vY1()       {return _scr.y0;}
        int vX2()       {return _scr.x1;}
        int vY2()       {return _scr.y1;}

        //Back ground
        inline int BG_MaxX(){return (_bgImg.width - _scr.width); }
        inline int BG_MaxY(){return (_bgImg.height - _scr.height); }

        //void* allocateMemory(size_t request, bool psram = true, size_t* outAligned = nullptr);
        bool init(Mode &m = MODE640x480_60Hz, bool usePal = false, uint8_t scale = 0, bool dBuff = false, uint8_t bpp = _16BIT);
        bool initBG(const uint8_t* data = nullptr, int width = 0, int height = 0, uint8_t num = 0);
        void scrToScr();
        void scrToBg();
        void bgToScr(int x, int y, bool repX = false, bool repY = false); 
        void bgScrollXY(int sx, int sy);       
        void swap();

        void cls(uint16_t col = 0);
        void setViewport(int x0, int y0, int x1, int y1);
        void memoryInfo();

        void setPins(Pins p);     
        void setPins(
            int r0, int r1, int r2, int r3, int r4,
            int g0, int g1, int g2, int g3, int g4, int g5,
            int b0, int b1, int b2, int b3, int b4,
            int hsync, int vsync,
            int pClkPin);

        // Palette
        inline uint16_t* Palette(){
            return (uint16_t*)_pal;
        }
  
        inline void setPalColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b){
            if (!_pal) return;
            _pal[index] = rgb888to565(r, g, b);             
        }
        inline uint16_t getPalColor(uint8_t index){
            if (!_pal) return 0;
            return _pal[index];            
        }
        void savePal(uint16_t* &dest);
        void clearPal(uint8_t from = 0, uint8_t to = 255, uint16_t col = 0);
        void fadeIn(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t step = 0);        
        void fadeOut(uint8_t from = 0, uint8_t to = 255, uint8_t step = 0);
        void fadeTo(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t step = 0);
        void fadeRangeTo(uint8_t from, uint8_t to, uint8_t r, uint8_t g, uint8_t b, uint8_t step = 0);
        void fadePalTo(const uint16_t* target, uint8_t from, uint8_t to, uint8_t step = 0);

    private:
        uint32_t _t;

        bool    _psram_ok   = false;
        bool    _inited     = false;
        bool    _dBuff      = false;
        bool    _bg         = false;

        Mode        _m = {};
        Screen      _scr = {};
        uint8_t     _scale;
        Pins        _pins = defPins_S3;
        Image       _bgImg = {};
        
        size_t _sramAlign = 32;
        size_t _psramAling = 32;
        int _frontBuf, _backBuf;
        int _frontBufLine, _backBufLine;
        int _lines, _lastPos, _pixels, _skip;
        int _shift, _copyBytes, _copyBytes2x;

        // FPS
        float               _fps = 0.0f;
        volatile uint32_t   _frameCount = 0;  // ISR
        uint64_t            _fpsStartTime = 0;         // μs
        volatile uint32_t   _timer = 0;       // vsync counter        

        // Palette
        bool        _usePal;
        uint16_t*   _pal = nullptr;


        uint16_t optimal_bounce_buffer_px();
        bool setRGBPanel();
        bool regSemaphore();
        void regCallBack();
        bool initPanel();
        bool setBufferAddr();
        bool makeDefPal();
        void updateFPS();

        esp_err_t                   err;
        esp_lcd_rgb_panel_config_t  panel_config = {};
        esp_lcd_panel_handle_t      panel_handle = nullptr;
        SemaphoreHandle_t           sem_vsync_end = nullptr;
        SemaphoreHandle_t           sem_gui_ready = nullptr;
        
        //static bool IRAM_ATTR on_color_trans_done(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
        static bool IRAM_ATTR on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
        static bool IRAM_ATTR on_bounce_empty(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx);
        static bool IRAM_ATTR on_bounce_empty_p4(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx);
        static bool IRAM_ATTR on_bounce_empty_pal(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx);
        //static bool IRAM_ATTR on_frame_buf_complete(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx); 
        
        static inline void IRAM_ATTR dup2_565(uint16_t* &dst, uint16_t col) {
            uint32_t color = (uint32_t)col | ((uint32_t)col << 16);
            *((uint32_t*)dst) = color;
            dst += 2;
        }

        static inline void IRAM_ATTR dup4_565(uint16_t* &dst, uint16_t col) {
            uint32_t color = (uint32_t)col | ((uint32_t)col << 16);
            *((uint32_t*)dst) = color; dst += 2;
            *((uint32_t*)dst) = color; dst += 2;
        }

        static inline void IRAM_ATTR dup2_8(uint8_t* &dst, uint8_t col) {
            *dst++ = col;
            *dst++ = col;
        }

        static inline void IRAM_ATTR dup4_8(uint8_t* &dst, uint8_t col) {
            uint32_t v = (uint32_t)col;
            v |= (v << 8);
            v |= (v << 16);
            *((uint32_t*)dst) = v;
            dst += 4;
        }   

        #if IS_P4
            bool    _ppaFill = false;
            bool    _ppaCopy = false;
            
            bool ppa_InitFill();
            bool ppa_InitCopy();
            bool ppa_Copy(void* dst, void* src, size_t bytes);

            ppa_client_handle_t _ppa_fill = nullptr;
            ppa_client_handle_t _ppa_copy = nullptr;

            uint32_t _ppa_res = 0;
            ppa_srm_oper_config_t bounce_cfg = {};
            ppa_client_handle_t bounce_srm = nullptr;
        #endif        
};

