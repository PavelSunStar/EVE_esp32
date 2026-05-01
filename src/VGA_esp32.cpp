#include "VGA_esp32.h"
#include <esp_LCD_panel_ops.h>

VGA_esp32::VGA_esp32(){
    #if defined(psramFound)
        _psram_ok = psramFound();
    #else
        _psram_ok = esp_psram_is_initialized();
    #endif

    #if IS_P4
        _pins = defPins_P4;
    #endif

    //FPS
    _frameCount = 0;
    _fpsStartTime = millis(); 
}

VGA_esp32::~VGA_esp32(){
    if (_scr.line8) delete[] _scr.line8;
    if (_scr.line16) delete[] _scr.line16;
    if (_scr.buf) heap_caps_free(_scr.buf);
    if (_scr.bg) heap_caps_free(_scr.bg);
    if (_pal) heap_caps_free(_pal);

    #if IS_P4
        if (_ppa_fill){
            ppa_unregister_client(_ppa_fill);
            _ppa_fill = nullptr;
        }

        if (_ppa_copy){
            ppa_unregister_client(_ppa_copy);
            _ppa_copy = nullptr;
        }

        if (bounce_srm){
            ppa_unregister_client(bounce_srm);
            bounce_srm = nullptr;
        }
    #endif
}
/*
void* VGA_esp32::allocateMemory(size_t request, bool psram, size_t* outAligned){
    if (psram && !_psram_ok){
        Serial.println("Error: PSRAM not present...");
        return nullptr;
    }

    size_t cache_align = 0;
    uint32_t caps = psram ? (MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA) : MALLOC_CAP_DMA;

    if (esp_cache_get_alignment(caps, &cache_align) != ESP_OK || cache_align < 2) {
        Serial.println("allocateMemory: esp_cache_get_alignment failed");
        return nullptr;
    }

    size_t aligned_size = (request + cache_align - 1) & ~(cache_align - 1);

    Serial.printf("allocateMemory: req=%u aligned=%u caps=0x%X align=%u\n",
                  (unsigned)request, (unsigned)aligned_size,
                  (unsigned)caps, (unsigned)cache_align);

    void* buffer = heap_caps_aligned_calloc(cache_align, 1, aligned_size, caps);
    if (!buffer) {
        Serial.println("allocateMemory: heap_caps_aligned_calloc failed");
        return nullptr;
    }

    if (psram) _psramAling = cache_align;
    else       _sramAlign  = cache_align;

    if (outAligned) *outAligned = aligned_size;
    return buffer;
}
*/
bool VGA_esp32::setBufferAddr(){
    if (!_scr.buf || _scr.width <= 0 || _scr.height <= 0) {
        Serial.println("initBuffer: invalid screen buffer");
        return false;
    }

    if (_scr.line8) {
        delete[] _scr.line8;
        _scr.line8 = nullptr;
    }

    if (_scr.line16) {
        delete[] _scr.line16;
        _scr.line16 = nullptr;
    }   

    int lines = _scr.height * (_dBuff ? 2 : 1);
    if (_scr.bpp == 16) {
        uint16_t* p = (uint16_t*)_scr.buf;
        _scr.line16 = new uint16_t*[lines]();

        if (!_scr.line16) {
            Serial.println("initBuffer: failed to alloc line16 table");
            return false;
        }

        for (int i = 0; i < lines; i++) {
            _scr.line16[i] = p;
            p += _scr.width;
        }
    } else {
        uint8_t* p = _scr.buf;
        _scr.line8 = new uint8_t*[lines]();

        if (!_scr.line8) {
            Serial.println("initBuffer: failed to alloc line8 table");
            return false;
        }

        for (int i = 0; i < lines; i++) {
            _scr.line8[i] = p;
            p += _scr.width;
        }
    }

    Serial.println("Set buffer address...Ok");
    return true;
}

bool VGA_esp32::init(Mode &m, bool usePal, uint8_t scale, bool dBuff, uint8_t bpp){
    if (m.pclk_hz == 0) return (_inited = false);
    if (bpp != _8BIT && bpp != _16BIT) return (_inited = false);

    _m = m;
    _usePal = usePal;
    _dBuff = dBuff;
    _scale = (scale > 2 ? 2 : scale);

    _scr.bpp = (_usePal ? _8BIT : bpp);
    _scr.width      = m.hRes >> _scale;
    _scr.height     = m.vRes >> _scale;
    _scr.maxX       = _scr.width - 1;
    _scr.maxY       = _scr.height - 1;
    _scr.cx         = _scr.width >> 1;
    _scr.cy         = _scr.height >> 1;
    _scr.lineSize   = _scr.width << (_scr.bpp == _16BIT ? 1 : 0);
    _scr.size       = _scr.width * _scr.height;
    _scr.fullSize   = _scr.lineSize * _scr.height;

    if (_usePal && !makeDefPal()) {
        Serial.println("ERROR: initPal failed");
        return (_inited = false);
    }

    if (!setRGBPanel()){
        Serial.println("ERROR: setRGBPanel failed");
        return (_inited = false);
    }
    
    Serial.println("\n[=== Init VGA ===]");
    memoryInfo();
    setViewport(0, 0, _scr.maxX, _scr.maxY);

    size_t req = _scr.fullSize << (_dBuff ? 1 : 0);
    Serial.printf("Alloc request: %u bytes, dbuff=%d, bpp=%d, scale=%d\n",
                  (unsigned)req, _dBuff, _scr.bpp, _scale);

    _scr.buf = (uint8_t*) allocateMemory(req, true);
    if (!_scr.buf){
        Serial.println("ERROR: allocateMemory failed");
        return false;
    }
    Serial.println("Alloc...Ok");

    if (!setBufferAddr()){
        Serial.println("ERROR: setBufferAddr failed");
        return false;
    }



    #if IS_P4
        _ppaFill = ppa_InitFill();
        _ppaCopy = ppa_InitCopy();
    #endif

    if (_dBuff) {
        if (!regSemaphore()) return false;
    } else {
        sem_vsync_end = nullptr;
        sem_gui_ready = nullptr;
    }

    regCallBack();
    if (!initPanel()) return false;

    Serial.println("VGA init...done\n");
    return (_inited = true);
}

void VGA_esp32::setViewport(int x0, int y0, int x1, int y1){
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);

    _scr.x0 = std::clamp(x0, 0, _scr.maxX);
    _scr.y0 = std::clamp(y0, 0, _scr.maxY);
    _scr.x1 = std::clamp(x1, 0, _scr.maxX);
    _scr.y1 = std::clamp(y1, 0, _scr.maxY);
}

void VGA_esp32::memoryInfo(){
    Serial.printf(
        "RAM free: %u | RAM largest: %u\nPSRAM free: %u | PSRAM largest: %u\n",
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)
    ); 
}

bool VGA_esp32::setRGBPanel(){
    panel_config = {};

    //Timing
    panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
    panel_config.timings.pclk_hz                = _m.pclk_hz;
    panel_config.timings.h_res                  = _m.hRes;
    panel_config.timings.v_res                  = _m.vRes;

    panel_config.timings.hsync_pulse_width      = _m.hSync;
    panel_config.timings.hsync_back_porch       = _m.hBack;
    panel_config.timings.hsync_front_porch      = _m.hFront;
    panel_config.timings.flags.hsync_idle_low   = (_m.hPol == 1) ^ 1;

    panel_config.timings.vsync_pulse_width      = _m.vSync;
    panel_config.timings.vsync_back_porch       = _m.vBack;
    panel_config.timings.vsync_front_porch      = _m.vFront;
    panel_config.timings.flags.vsync_idle_low   = (_m.vPol == 1) ^ 1;
    
    panel_config.timings.flags.de_idle_high     = true;
    panel_config.timings.flags.pclk_active_neg  = true;
    panel_config.timings.flags.pclk_idle_high   = true;

    //Panel config
    int colorBits = (_usePal ? _16BIT : _scr.bpp);
    panel_config.data_width             = colorBits;        
    panel_config.bits_per_pixel         = colorBits;   
    panel_config.num_fbs                = 0;
    panel_config.bounce_buffer_size_px  = optimal_bounce_buffer_px();
    panel_config.sram_trans_align       = _sramAlign;
    panel_config.psram_trans_align      = _psramAling;
    //panel_config.dma_burst_size = 64;

    //Pins config
    if (_scr.bpp == _16BIT || _usePal){
        // B0..B4
        for (int i = 0; i < 5; i++)
            panel_config.data_gpio_nums[i] = _pins.b[i];

        // G0..G5
        for (int i = 0; i < 6; i++)
            panel_config.data_gpio_nums[5 + i] = _pins.g[i];

        // R0..R4
        for (int i = 0; i < 5; i++)
            panel_config.data_gpio_nums[11 + i] = _pins.r[i];
    } else {
        panel_config.data_gpio_nums[0] = _pins.b[3];
        panel_config.data_gpio_nums[1] = _pins.b[4];
        panel_config.data_gpio_nums[2] = _pins.g[3];
        panel_config.data_gpio_nums[3] = _pins.g[4];
        panel_config.data_gpio_nums[4] = _pins.g[5];
        panel_config.data_gpio_nums[5] = _pins.r[2];
        panel_config.data_gpio_nums[6] = _pins.r[3];
        panel_config.data_gpio_nums[7] = _pins.r[4];
    }
    panel_config.hsync_gpio_num = _pins.h;
    panel_config.vsync_gpio_num = _pins.v;
    panel_config.de_gpio_num = -1;
    panel_config.pclk_gpio_num = _pins.pClk;
    panel_config.disp_gpio_num = -1;

    //Flags
    panel_config.flags.disp_active_low = true;
    panel_config.flags.refresh_on_demand = false;
    panel_config.flags.fb_in_psram = false;
    panel_config.flags.double_fb = false;
    panel_config.flags.no_fb = true;
    panel_config.flags.bb_invalidate_cache = false;
    
    esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &panel_handle);
    if (err != ESP_OK) {
        Serial.printf("esp_lcd_new_rgb_panel error: %d\n", err);
        return false;
    }

    Serial.println("RGB Panel set...Ok");
    return true;
}

bool VGA_esp32::initPanel(){
    err = esp_lcd_panel_reset(panel_handle);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to reset panel: %s\n", esp_err_to_name(err));
        return false;
    }

    err = esp_lcd_panel_init(panel_handle);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to init panel: %s\n", esp_err_to_name(err));
        return false;
    }

    Serial.println("Init panel complete...Ok");
    return true;
}

void VGA_esp32::setPins(Pins p){
    _pins = p;
}

void VGA_esp32::setPins(
    int r0, int r1, int r2, int r3, int r4,
    int g0, int g1, int g2, int g3, int g4, int g5,
    int b0, int b1, int b2, int b3, int b4,
    int hsync, int vsync,
    int pClkPin)
{
    // Заполняем структуру
    _pins.r[0] = r0; _pins.r[1] = r1; _pins.r[2] = r2; _pins.r[3] = r3; _pins.r[4] = r4;
    _pins.g[0] = g0; _pins.g[1] = g1; _pins.g[2] = g2; _pins.g[3] = g3; _pins.g[4] = g4; _pins.g[5] = g5;
    _pins.b[0] = b0; _pins.b[1] = b1; _pins.b[2] = b2; _pins.b[3] = b3; _pins.b[4] = b4;
    _pins.h = hsync;
    _pins.v = vsync;
    _pins.pClk = pClkPin;
}

uint16_t VGA_esp32::optimal_bounce_buffer_px(){
    int res = 0;
    _shift = (_usePal ? 1 : (_scr.bpp == 16 ? 1 : 0));
    if (_m.hRes == 640 && _m.vRes == 480) res = 30720 >> _shift; 
    _lastPos     = _m.hRes * _m.vRes - res;

    _lines = (res / _m.hRes) >> _scale;
    _pixels = _m.hRes >> 3;
    _copyBytes = _m.hRes << _shift;
    _copyBytes2x = _copyBytes << 1;
    _skip = (_m.hRes >> 2) << _shift;

    #if IS_P4
        _ppa_res = res;
    #endif

    return res;
}

bool VGA_esp32::regSemaphore(){
    sem_vsync_end = nullptr;
    sem_gui_ready = nullptr;

    if (!_dBuff){
        Serial.println("Double buffer not selected.");
        return true;
    }

    sem_vsync_end = xSemaphoreCreateBinary();
    sem_gui_ready = xSemaphoreCreateBinary();

    if (!sem_vsync_end || !sem_gui_ready){
        Serial.println("Error: Semaphores init fail.");
        return false;
    }

    Serial.println("Registered semaphores...Ok");
    return true;
}

void VGA_esp32::regCallBack(){
    esp_lcd_rgb_panel_event_callbacks_t cb = {
        .on_color_trans_done = nullptr,
        .on_vsync            = on_vsync,
        .on_bounce_empty = (
            _usePal
                ? on_bounce_empty_pal
                : (IS_P4 ? on_bounce_empty_p4 : on_bounce_empty)
        ),              
        .on_frame_buf_complete = nullptr
    };

    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cb, this)); 
    Serial.println("Registered callBacks...Ok"); 
}

bool IRAM_ATTR VGA_esp32::on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx){
    VGA_esp32* vga = (VGA_esp32*)user_ctx;

    vga->_timer++;

    return false;
}

    // ESP32-P4 RGB workaround:
    // controller behaves as if output starts 8 pixels later
    // and tail must be taken from next DMA chunk

    // ESP32-P4 workaround:
    // RGB output behaves as if each DMA chunk starts 8 pixels late.
    // To compensate, bounce buffer is assembled from:
    // 1) main part shifted by 8 pixels
    // 2) tail taken from the next logical source chunk
    // Separate paths are required for scale 0/1/2.

    // ESP32-P4 RGB / bounce buffer workaround
    //
    // Problem:
    // On ESP32-S3 the RGB bounce callback data is displayed correctly,
    // but on ESP32-P4 the visible image is shifted horizontally by 8 pixels.
    // At first this looks like a timing / HSYNC / porch problem, but changing
    // VGA timings does not fix it.
    //
    // Observation:
    // The issue is not caused by the source framebuffer contents themselves.
    // Even when writing test data directly into bounce_buf, ESP32-P4 still shows
    // the same 8-pixel horizontal misalignment.
    //
    // Actual behavior:
    // ESP32-P4 does not behave like a simple cyclic 8-pixel line shift.
    // Instead, each output chunk behaves as if the first 8 pixels of the current
    // logical block are skipped, and the missing tail must be taken from the next
    // logical source block.
    //
    // In other words, the displayed line is effectively assembled as:
    //   [current block starting from pixel +8] + [tail from next block]
    //
    // Because of this, a normal memcpy or a simple rotate of the last 8 pixels
    // is not sufficient. A dedicated ESP32-P4 path is required when filling
    // bounce_buf, including separate handling for scale 0 / 1 / 2.
    //
    // Notes:
    // - This workaround is only needed on ESP32-P4.
    // - ESP32-S3 works correctly with the normal bounce buffer logic.
    // - The root cause appears to be inside the ESP32-P4 RGB/LCD_COM/DMA path.

bool IRAM_ATTR VGA_esp32::on_bounce_empty_p4(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx){
    VGA_esp32* vga = (VGA_esp32*) user_ctx;

    int shift = vga->_shift;
    int bytes = 8 << shift;
    int fix   = len_bytes - bytes;
    int position = pos_px << shift;
    int fill = position + len_bytes + bytes;    

    if (vga->_scale == 0){
        uint8_t *dest = (uint8_t*)bounce_buf;
        uint8_t *sour = vga->_scr.buf + vga->_frontBuf + (pos_px << (vga->_shift));

        if (fill <= vga->_scr.fullSize) memcpy(dest + fix, sour + len_bytes, bytes);        
            memcpy(dest, sour + bytes, fix);    
    } else {
        uint32_t* dest = (uint32_t*)((uint8_t*)bounce_buf + (vga->_m.hRes << shift) - bytes);

        uint32_t col;        
        int lines           = vga->_lines - 1;
        int pixels          = vga->_pixels;
        int copyBytes       = vga->_copyBytes;
        int copyBytes2x     = vga->_copyBytes2x;
        int skip            = vga->_skip;
        uint32_t* savePos   = nullptr; 

        if (vga->_scale == 1){
            if (vga->_scr.bpp == 16){
                uint16_t* sour = (uint16_t*)(vga->_scr.buf + vga->_frontBuf) + (pos_px >> 2);
                uint16_t* secondSour = sour + 4;
                uint16_t* firstSour = sour + (len_bytes >> 3);

                for (int i = 0; i < pixels; i++){
                    col = *sour++; *dest++ = col | (col << 16);
                    col = *sour++; *dest++ = col | (col << 16);
                    col = *sour++; *dest++ = col | (col << 16);
                    col = *sour++; *dest++ = col | (col << 16);
                }  

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                    }    
                    memcpy(dest, savePos, copyBytes); dest += skip;
                } 

                col = *firstSour++; *dest++ = col | (col << 16);
                col = *firstSour++; *dest++ = col | (col << 16);
                col = *firstSour++; *dest++ = col | (col << 16);
                col = *firstSour++; *dest++ = col | (col << 16);

                pixels--;
                dest = (uint32_t*)(bounce_buf);
                while (pixels-- > 0){
                    col = *secondSour++; *dest++ = col | (col << 16);
                    col = *secondSour++; *dest++ = col | (col << 16);
                    col = *secondSour++; *dest++ = col | (col << 16);
                    col = *secondSour++; *dest++ = col | (col << 16);
                }                 
            } else {
                uint8_t* sour = vga->_scr.buf + vga->_frontBuf + (pos_px >> 2);
                uint8_t* secondSour = sour + 4;
                uint8_t* firstSour = sour + (len_bytes >> 2);

                for (int i = 0; i < pixels; i++){
                    col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                    col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                }  
                
                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                        col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);
                    dest += skip;
                }    
                
                col = *firstSour++; col |= (*firstSour++ << 16); col |= col << 8; *dest++ = col;
                col = *firstSour++; col |= (*firstSour++ << 16); col |= col << 8; *dest++ = col;

                pixels--;
                dest = (uint32_t*)(bounce_buf);
                while (pixels-- > 0){
                    col = *secondSour++; col |= (*secondSour++ << 16); col |= col << 8; *dest++ = col;
                    col = *secondSour++; col |= (*secondSour++ << 16); col |= col << 8; *dest++ = col;
                }                 
            }
        } else {
            int skip2 = skip << 1;

            if (vga->_scr.bpp == 16){
                uint16_t* sour = (uint16_t*)(vga->_scr.buf + vga->_frontBuf) + (pos_px >> 4);
                uint16_t* secondSour = sour + 2;
                uint16_t* firstSour = sour + (len_bytes >> 3);

                savePos = dest;
                for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                }  
                memcpy(dest, savePos, copyBytes); dest += skip;
                memcpy(dest, savePos, copyBytes); dest += skip; 

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);   dest += skip;
                    memcpy(dest, savePos, copyBytes2x); dest += skip2;
                }

                col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;                

                pixels--;
                dest = (uint32_t*)(bounce_buf);
                while (pixels-- > 0){
                        col = *secondSour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                        col = *secondSour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                }                 
            } else {
                uint8_t* sour = vga->_scr.buf + vga->_frontBuf + (pos_px >> 4);
                uint8_t* secondSour = sour + 2;
                uint8_t* firstSour = sour + (len_bytes >> 4); 

                savePos = dest;
                for (int i = 0; i < pixels; i++){
                    col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                    col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                }  
                memcpy(dest, savePos, copyBytes); dest += skip;
                memcpy(dest, savePos, copyBytes); dest += skip;                 
                
                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                        col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);   dest += skip;
                    memcpy(dest, savePos, copyBytes2x); dest += skip2;
                } 
                
                col = *firstSour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                col = *firstSour++; col |= (col << 16); col |= col << 8; *dest++ = col;  
                
                pixels--;
                dest = (uint32_t*)(bounce_buf);
                while (pixels-- > 0){
                    col = *secondSour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                    col = *secondSour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                }                    
            }
        }
    }

    BaseType_t hp_task_woken = pdFALSE;
    if (pos_px >= vga->_lastPos && vga->_dBuff) {
        if (xSemaphoreTakeFromISR(vga->sem_gui_ready, &hp_task_woken) == pdTRUE) {
            std::swap(vga->_frontBuf, vga->_backBuf);
            std::swap(vga->_frontBufLine, vga->_backBufLine);
            xSemaphoreGiveFromISR(vga->sem_vsync_end, &hp_task_woken);
        }
    }

    return (hp_task_woken == pdTRUE);
}

bool IRAM_ATTR VGA_esp32::on_bounce_empty_pal(
    esp_lcd_panel_handle_t panel,
    void *bounce_buf,
    int pos_px,
    int len_bytes,
    void *user_ctx
){
    VGA_esp32* vga = (VGA_esp32*)user_ctx;
    //vga->_t = micros();

    uint32_t* dest = (uint32_t*)bounce_buf;

    if (vga->_scale == 0){

        uint16_t* pal16 = (uint16_t*)vga->_pal;
        uint8_t* sour = vga->_scr.buf + vga->_frontBuf + pos_px;

        int loops = len_bytes >> 4; // 8 pixels per loop

        while (loops--){
            uint32_t c0, c1;

            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = (c1 << 16) | c0;
            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = (c1 << 16) | c0;
            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = (c1 << 16) | c0;
            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = (c1 << 16) | c0;
        }

    } else if (vga->_scale == 1){

        uint32_t* pal32 = (uint32_t*)vga->_pal;
        uint8_t* sour = vga->_scr.buf + vga->_frontBuf + (pos_px >> 2);

        int lines = vga->_lines;
        int pixels = vga->_pixels;
        int copyBytes = vga->_copyBytes;
        int skip = vga->_skip;

        while (lines-- > 0){
            uint32_t* savePos = dest;

            for (int i = 0; i < pixels; i++){
                *dest++ = pal32[*sour++];
                *dest++ = pal32[*sour++];
                *dest++ = pal32[*sour++];
                *dest++ = pal32[*sour++];
            }

            memcpy(dest, savePos, copyBytes);
            dest += skip;
        }

    } else {

        uint32_t* pal32 = (uint32_t*)vga->_pal;
        uint8_t* sour = vga->_scr.buf + vga->_frontBuf + (pos_px >> 4);

        int lines = vga->_lines;
        int pixels = vga->_pixels;
        int copyBytes = vga->_copyBytes;
        int copyBytes2x = vga->_copyBytes2x;
        int skip = vga->_skip;
        int skip2x = skip << 1;
        uint32_t idx;

        while (lines-- > 0){
            uint32_t* savePos = dest;

            for (int i = 0; i < pixels; i++){
                idx = ((uint32_t)*sour++) << 1;
                *dest++ = pal32[idx++];
                *dest++ = pal32[idx];    

                idx = ((uint32_t)*sour++) << 1;
                *dest++ = pal32[idx++];
                *dest++ = pal32[idx];  
            }

            memcpy(dest, savePos, copyBytes);
            dest += skip;

            memcpy(dest, savePos, copyBytes2x);
            dest += skip2x;
        }
    }

    BaseType_t hp_task_woken = pdFALSE;

    if (pos_px >= vga->_lastPos && vga->_dBuff) {
        if (xSemaphoreTakeFromISR(vga->sem_gui_ready, &hp_task_woken) == pdTRUE) {
            std::swap(vga->_frontBuf, vga->_backBuf);
            std::swap(vga->_frontBufLine, vga->_backBufLine);
            xSemaphoreGiveFromISR(vga->sem_vsync_end, &hp_task_woken);
        }
    }

    //vga->_t = micros() - vga->_t;
    return (hp_task_woken == pdTRUE);
}

/*
bool IRAM_ATTR VGA_esp32::on_bounce_empty_pal(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx){
    VGA_esp32* vga = (VGA_esp32*) user_ctx;
    vga->_t = micros();

    uint32_t c;
    uint16_t* pal = (uint16_t*)vga->_pal;    
    uint32_t* dest = (uint32_t*)bounce_buf;

    if (vga->_scale == 0){
        uint8_t* sour = vga->_scr.buf + vga->_frontBuf + pos_px;
        int pixels = len_bytes >> 4;
        uint32_t c0;

        while (pixels--) {
            c0 = pal[*sour++]; c = pal[*sour++]; *dest++ = (c << 16) | c0;
            c0 = pal[*sour++]; c = pal[*sour++]; *dest++ = (c << 16) | c0;
            c0 = pal[*sour++]; c = pal[*sour++]; *dest++ = (c << 16) | c0;
            c0 = pal[*sour++]; c = pal[*sour++]; *dest++ = (c << 16) | c0;
        }
    } else {
        uint32_t* savePos = nullptr;
        int lines = vga->_lines;
        int pixels = vga->_pixels;
        int copyBytes = vga->_copyBytes;
        int skip = vga->_skip;
        
        if (vga->_scale == 1){
            uint8_t* sour = vga->_scr.buf + vga->_frontBuf + (pos_px >> 2);

            while (lines-- > 0){
                savePos = dest;
                for (int i = 0; i < pixels; i++){
                    c = pal[*sour++]; *dest++ = (c << 16) | c; 
                    c = pal[*sour++]; *dest++ = (c << 16) | c; 
                    c = pal[*sour++]; *dest++ = (c << 16) | c; 
                    c = pal[*sour++]; *dest++ = (c << 16) | c; 
                }
                memcpy(dest, savePos, copyBytes);
                dest += skip;
            }
        } else {
            uint8_t* sour = vga->_scr.buf + vga->_frontBuf + (pos_px >> 4);
            int skip2x = skip << 1;
            int copyBytes2x = vga->_copyBytes2x;

            while (lines-- > 0){
                savePos = dest;
                for (int i = 0; i < pixels; i++){
                    c = pal[*sour++]; c = (c << 16) | c; *dest++ = c; *dest++ = c; 
                    c = pal[*sour++]; c = (c << 16) | c; *dest++ = c; *dest++ = c; 
                }
                memcpy(dest, savePos, copyBytes); dest += skip;
                memcpy(dest, savePos, copyBytes2x); dest += skip2x;
            }    
        }
    }

    BaseType_t hp_task_woken = pdFALSE;
    if (pos_px >= vga->_lastPos && vga->_dBuff) {
        if (xSemaphoreTakeFromISR(vga->sem_gui_ready, &hp_task_woken) == pdTRUE) {
            std::swap(vga->_frontBuf, vga->_backBuf);
            std::swap(vga->_frontBufLine, vga->_backBufLine);
            xSemaphoreGiveFromISR(vga->sem_vsync_end, &hp_task_woken);
        }
    }   

    vga->_t = micros() - vga->_t;
    return (hp_task_woken == pdTRUE);
}
*/
bool IRAM_ATTR VGA_esp32::on_bounce_empty(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx){
    VGA_esp32* vga = (VGA_esp32*) user_ctx;

    if (vga->_scale == 0){
        uint8_t *dest = (uint8_t*)(bounce_buf);
        uint8_t *sour = vga->_scr.buf + vga->_frontBuf + (pos_px << (vga->_shift));
        memcpy(dest, sour, len_bytes);
    } else {
        uint32_t *dest      = (uint32_t*)(bounce_buf); 

        uint32_t col;        
        int lines           = vga->_lines;
        int pixels          = vga->_pixels;
        int copyBytes       = vga->_copyBytes;
        int copyBytes2x     = vga->_copyBytes2x;
        int skip            = vga->_skip;
        uint32_t* savePos   = nullptr; 
        
        if (vga->_scale == 1){
            if (vga->_scr.bpp == 16){
                uint16_t *sour = (uint16_t*)(vga->_scr.buf + vga->_frontBuf) + (pos_px >> 2);

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                    }    
                    memcpy(dest, savePos, copyBytes);
                    dest += skip;
                } 

                //if (fix != 0) memcpy(dest, savePos + 16, 16);
            } else {
                uint8_t *sour = vga->_scr.buf + vga->_frontBuf + (pos_px >> 2);

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                        col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);
                    dest += skip;
                } 
            }        
        } else {
            int skip2 = skip << 1;

            if (vga->_scr.bpp == 16){
                uint16_t *sour = (uint16_t*)(vga->_scr.buf + vga->_frontBuf) + (pos_px >> 4);

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);   dest += skip;
                    memcpy(dest, savePos, copyBytes2x); dest += skip2;
                }
            } else {
                uint8_t *sour = vga->_scr.buf + vga->_frontBuf + (pos_px >> 4);

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                        col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);   dest += skip;
                    memcpy(dest, savePos, copyBytes2x); dest += skip2;
                }
            }       
        }
    }

    BaseType_t hp_task_woken = pdFALSE;
    if (pos_px >= vga->_lastPos && vga->_dBuff) {
        if (xSemaphoreTakeFromISR(vga->sem_gui_ready, &hp_task_woken) == pdTRUE) {
            std::swap(vga->_frontBuf, vga->_backBuf);
            std::swap(vga->_frontBufLine, vga->_backBufLine);
            xSemaphoreGiveFromISR(vga->sem_vsync_end, &hp_task_woken);
        }
    }   
      
    return (hp_task_woken == pdTRUE);
}

void VGA_esp32::swap(){
    if (_dBuff){
        if (!sem_gui_ready || !sem_vsync_end){
            Serial.println("swap: semaphore not initialized");
            return;
        }

        xSemaphoreGive(sem_gui_ready);
        xSemaphoreTake(sem_vsync_end, portMAX_DELAY);
    }

    updateFPS();
}

void VGA_esp32::updateFPS(){
    uint64_t now = millis();
    _frameCount++;

    if (now - _fpsStartTime >= 1000) {
        _fps = _frameCount * 1000.0f / (now - _fpsStartTime);
        _frameCount = 0;
        _fpsStartTime = now;
    }
}

void VGA_esp32::bgScrollXY(int sx, int sy){
    if (!_scr.bg) return;

    int w = _scr.width;
    int h = _scr.height;

    // нормализация в диапазон 0..w-1 / 0..h-1
    sx %= w;
    sy %= h;
    if (sx < 0) sx += w;
    if (sy < 0) sy += h;

    if (sx == 0 && sy == 0){
        memcpy(_scr.buf + _backBuf, _scr.bg, _scr.fullSize);
        return;
    }

    int bppBytes = (_scr.bpp == 16) ? 2 : 1;
    int first    = sx * bppBytes;
    int second   = _scr.lineSize - first;

    uint8_t* dest = _scr.buf + _backBuf;

    for (int y = 0; y < h; y++){
        int srcY = y + sy;
        if (srcY >= h) srcY -= h;

        uint8_t* sour = _scr.bg + srcY * _scr.lineSize;

        if (sx == 0){
            memcpy(dest, sour, _scr.lineSize);
        } else {
            memcpy(dest,          sour + first, second);
            memcpy(dest + second, sour,         first);
        }

        dest += _scr.lineSize;
    }
}

void VGA_esp32::cls(uint16_t col){
    if (!_inited) return;

    #if IS_P4    
    if (_ppa_fill && (_scr.bpp == _16BIT)){ 
        ppa_fill_oper_config_t cfg = {};

        cfg.out.buffer = (void*)(_scr.buf + _backBuf);
        cfg.out.buffer_size = _scr.fullSize;
        cfg.out.pic_w = _scr.width;
        cfg.out.pic_h = _scr.height;
        cfg.out.block_offset_x = 0;
        cfg.out.block_offset_y = 0;
        cfg.out.fill_cm = PPA_FILL_COLOR_MODE_RGB565;

        cfg.fill_block_w = _scr.width;
        cfg.fill_block_h = _scr.height;
        cfg.fill_color_val = col;  
            
        cfg.mode = PPA_TRANS_MODE_BLOCKING;
        cfg.user_data = nullptr;   

        ppa_do_fill(_ppa_fill, &cfg);
        return;        
    } 
    #endif

    if (_scr.bpp == _16BIT){
        uint16_t* scr = (uint16_t*)(_scr.buf + _backBuf);

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(scr, (uint8_t)(col & 0xFF),_scr.fullSize);
        } else {
            int size = 0;
            uint16_t* cpy = scr;
            while (size++ < _scr.width) *scr++ = col;

            int dummy = 1; 
            int lines = _scr.maxY;  
            int copyBytes = _scr.lineSize;
            int offset = _scr.width;
        
            while (lines > 0){ 
                if (lines >= dummy){
                    memcpy(scr, cpy, copyBytes);
                    lines -= dummy;
                    scr += offset;
                    copyBytes <<= 1;
                    offset <<= 1;
                    dummy <<= 1;
                } else {
                    copyBytes =_scr.lineSize * lines;
                    memcpy(scr, cpy, copyBytes);
                    break;
                }
            }            
        }        
    } else {
        memset(_scr.buf + _backBuf, (uint8_t) col, _scr.fullSize);
    }
}

#if IS_P4
bool VGA_esp32::ppa_InitFill(){
    if (_ppaFill) return true;

    ppa_client_config_t cfg = {};
    cfg.oper_type = PPA_OPERATION_FILL;
    cfg.max_pending_trans_num = 1; 
    err = ppa_register_client(&cfg, &_ppa_fill);

    return (err == ESP_OK);
}

bool VGA_esp32::ppa_InitCopy(){
    if (_ppaCopy) return true;

    ppa_client_config_t cfg = {};
    cfg.oper_type = PPA_OPERATION_SRM;
    cfg.max_pending_trans_num = 1;

    err = ppa_register_client(&cfg, &_ppa_copy);
    _ppaCopy = (err == ESP_OK);

    if (!_ppaCopy){
        Serial.printf("ppa_InitCopy error: %s\n", esp_err_to_name(err));
    }

    return _ppaCopy;
}

bool VGA_esp32::ppa_Copy(void* dst, void* src, size_t bytes){
    if (!_ppaCopy || !dst || !src || bytes == 0) return false;
    if (_scr.bpp != _16BIT) return false;

    ppa_srm_oper_config_t cfg = {};

    // INPUT
    cfg.in.buffer = src;
    cfg.in.pic_w = _scr.width;
    cfg.in.pic_h = _scr.height;
    cfg.in.block_w = _scr.width;
    cfg.in.block_h = _scr.height;
    cfg.in.block_offset_x = 0;
    cfg.in.block_offset_y = 0;
    cfg.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

    // OUTPUT
    cfg.out.buffer = dst;
    cfg.out.buffer_size = bytes;
    cfg.out.pic_w = _scr.width;
    cfg.out.pic_h = _scr.height;
    cfg.out.block_offset_x = 0;
    cfg.out.block_offset_y = 0;
    cfg.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

    // SRM settings
    cfg.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
    cfg.scale_x = 1.0f;
    cfg.scale_y = 1.0f;
    cfg.mirror_x = false;
    cfg.mirror_y = false;
    cfg.rgb_swap = false;
    cfg.byte_swap = false;
    cfg.alpha_update_mode = PPA_ALPHA_NO_CHANGE;

    cfg.mode = PPA_TRANS_MODE_BLOCKING;
    cfg.user_data = nullptr;

    err = ppa_do_scale_rotate_mirror(_ppa_copy, &cfg);
    if (err != ESP_OK){
        Serial.printf("ppa_Copy error: %s\n", esp_err_to_name(err));
        return false;
    }

    return true;
}
#endif

bool VGA_esp32::initBG(const uint8_t* data, int width, int height, uint8_t num){
    if (data){
        _scr.bg = (uint8_t*)loadImage(data, _scr.bpp, _bgImg, num);
    } else {
        _bgImg.width        = (width < _scr.width ? _scr.width : width);
        _bgImg.height       = (height < _scr.height ? _scr.height : height);
        _bgImg.maxX         = _bgImg.width - 1;
        _bgImg.maxY         = _bgImg.height - 1;
        _bgImg.cx           = _bgImg.width >> 1;
        _bgImg.cy           = _bgImg.height >> 1;
        _bgImg.lineSize     = _bgImg.width * (_scr.bpp == _16BIT ? 2 : 1);
        _bgImg.size         = _bgImg.width * _bgImg.height;
        _bgImg.fullSize     = _bgImg.lineSize * _bgImg.height;
        _bgImg.offset       = 0;
        _bgImg.offsetLine   = 0;   
        
        _scr.bg = (uint8_t*) allocateMemory(_bgImg.fullSize);
        if (!_scr.bg) return (_bg = false);
        memset(_scr.bg, 0, _bgImg.fullSize);
    }

/*    
    _scr.bg = (uint8_t*) allocateMemory(_scr.fullSize, true);
    if (!_scr.bg) return false;

    #if IS_P4
    if (!_ppaCopy) ppa_InitCopy();
    #endif

    Serial.println("VGA background init...done\n");
*/ 
    return (_bg = true);
}

void VGA_esp32::scrToBg(){
    if (!_bg) return;

    uint8_t* src = _scr.buf + _backBuf;
    uint8_t* dst = _scr.bg;

/*
    #if IS_P4
    if (_ppaCopy && _scr.bpp == _16BIT){
        if (ppa_Copy(dst, src, _scr.fullSize)) return;
    }
    #endif
*/    
        memcpy(dst, src, _scr.fullSize);
}

void VGA_esp32::scrToScr(){
    if (_dBuff){
        memcpy(_scr.buf + _backBuf, _scr.buf + _frontBuf, _scr.fullSize);
    }
}

void VGA_esp32::bgToScr(int x, int y, bool repX, bool repY){
    if (!_bg) return;

    // ---------- Y ----------
    int pfy = y;

    if (repY){
        pfy %= _bgImg.height;
        if (pfy < 0) pfy += _bgImg.height;
    } else {
        pfy = std::clamp(pfy, 0, BG_MaxY());
    }

    bool wrapY = repY && (pfy > BG_MaxY());

    // ---------- X ----------
    int pfx = x;

    if (repX){
        pfx %= _bgImg.width;
        if (pfx < 0) pfx += _bgImg.width;
    } else {
        pfx = std::clamp(pfx, 0, BG_MaxX());
    }

    bool wrapX = repX && (pfx > BG_MaxX());

    int fx = pfx << _shift;

    int flx = 0;
    int slx = 0;

    if (wrapX){
        int flxPix = _bgImg.width - pfx;
        int slxPix = _scr.width - flxPix;

        flx = flxPix << _shift;
        slx = slxPix << _shift;
    }

    uint8_t* dest = _scr.buf + _backBuf;

    auto copyLines = [&](uint8_t* sour, int lines){
        if (!wrapX){
            while (lines-- > 0){
                memcpy(dest, sour + fx, _scr.lineSize);

                dest += _scr.lineSize;
                sour += _bgImg.lineSize;
            }
        } else {
            while (lines-- > 0){
                memcpy(dest, sour + fx, flx);
                memcpy(dest + flx, sour, slx);

                dest += _scr.lineSize;
                sour += _bgImg.lineSize;
            }
        }
    };

    if (!wrapY){
        uint8_t* sour = _scr.bg + pfy * _bgImg.lineSize;
        copyLines(sour, _scr.height);
    } else {
        int fly = _bgImg.height - pfy;
        int sly = _scr.height - fly;

        uint8_t* sour1 = _scr.bg + pfy * _bgImg.lineSize;
        copyLines(sour1, fly);

        uint8_t* sour2 = _scr.bg;
        copyLines(sour2, sly);
    }
}

/*
    #if IS_P4
    if (_ppaCopy && _scr.bpp == _16BIT){
        if (ppa_Copy(dst, src, _scr.fullSize)) return;
    }
    #endif
*/
bool VGA_esp32::makeDefPal(){
    if (_pal){ 
        heap_caps_free(_pal);
        _pal = nullptr;
    } 

    uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;

    int mul = 1;
    switch (_scale){
        case 0: mul = 1; break;
        case 1: mul = 2; break;
        case 2: mul = 4; break;
        default: return false;
    }

    _pal = (uint16_t*)heap_caps_malloc(256 * mul * sizeof(uint16_t), caps);
    if (!_pal) return false;

    uint16_t* p = (uint16_t*)_pal; 

    for (int i = 0; i < 256; i++){
        uint16_t col = rgb332to565((uint8_t)i);

        for (int n = 0; n < mul; n++){
            *p++ = col;
        }
    }

    return true;
}

void VGA_esp32::clearPal(uint8_t from, uint8_t to, uint16_t col){
    if (!_pal) return;
    if (from > to) std::swap(from, to);

    for (int i = from; i <= to; i++) {
        _pal[i] = col;
    }
}

void VGA_esp32::fadeOut(uint8_t from, uint8_t to, uint8_t step){
    if (step == 0 || !_pal) return;
    if (from > to) std::swap(from, to);

    uint8_t r, g, b;    
    for (int i = from; i <= to; i++){
        rgb565to888(_pal[i], r, g, b);

        r = (r > step) ? r - step : 0;
        g = (g > step) ? g - step : 0;
        b = (b > step) ? b - step : 0;

        _pal[i] = rgb888to565(r, g, b);
    }
}

void VGA_esp32::fadeIn(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t step){
    if (step == 0 || !_pal) return;

    uint8_t rr, gg, bb;
    rgb565to888(_pal[index], rr, gg, bb);

    rr = (rr < r) ? (uint8_t)min((int)rr + step, (int)r) : rr;
    gg = (gg < g) ? (uint8_t)min((int)gg + step, (int)g) : gg;
    bb = (bb < b) ? (uint8_t)min((int)bb + step, (int)b) : bb;

    _pal[index] = rgb888to565(rr, gg, bb);
}

void VGA_esp32::fadeTo(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t step){
    if (step == 0 || !_pal) return;

    uint8_t rr, gg, bb;
    rgb565to888(_pal[index], rr, gg, bb);

    auto approach = [](uint8_t cur, uint8_t target, uint8_t step) -> uint8_t {
        if (cur < target) {
            int v = (int)cur + step;
            return (v > target) ? target : (uint8_t)v;
        }

        if (cur > target) {
            int v = (int)cur - step;
            return (v < target) ? target : (uint8_t)v;
        }

        return cur;
    };

    rr = approach(rr, r, step);
    gg = approach(gg, g, step);
    bb = approach(bb, b, step);

    _pal[index] = rgb888to565(rr, gg, bb);
}

void VGA_esp32::fadeRangeTo(uint8_t from, uint8_t to,
                            uint8_t r, uint8_t g, uint8_t b,
                            uint8_t step){
    if (step == 0 || !_pal) return;
    if (from > to) std::swap(from, to);

    for (int i = from; i <= to; i++) {
        fadeTo(i, r, g, b, step);
    }
}

void VGA_esp32::fadePalTo(const uint16_t* target, uint8_t from, uint8_t to, uint8_t step){
    if (step == 0 || !_pal || !target) return;

    if (from > to) std::swap(from, to);

    for (int i = from; i <= to; i++) {
        uint8_t r, g, b;
        rgb565to888(target[i], r, g, b);
        fadeTo((uint8_t)i, r, g, b, step);
    }
}