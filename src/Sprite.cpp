#include "Arduino.h"
#include "Sprite.h"

Sprite::Sprite(EVE_esp32& vga) : _vga(vga){

}

Sprite::~Sprite(){

}

void Sprite::resetParam(uint16_t num){
    if (num == 0) return;
    
    if (_img){
        delete[] _img;
        _img = nullptr;
    }

    _maxImage = num;
    _img = new Image[_maxImage];
}

void Sprite::freeMemory(){
    _created = false;
    _psram_ok = false;

    if (buf) {
        heap_caps_free(buf);
        buf = nullptr;
    }
}

bool Sprite::createImages(uint16_t xx, uint16_t yy, uint16_t num){
    if (!_vga.vbInited() || xx <= 0 || yy <= 0 || num == 0) return false;

    resetParam(num);
    _bpp = _vga.vbBPP();
    _shift = (_bpp == _16BIT ? 1 : 0);
    size_t size = (size_t(xx) << _shift) * yy;
    size *= _maxImage;
    if (!allocateMemory(size)) return false;

    uint32_t offsetLine = 0;
    for (int i = 0; i < _maxImage; i++){
        _img[i].width       = xx;
        _img[i].height      = yy;
        _img[i].xx          = _img[i].width - 1;
        _img[i].yy          = _img[i].height - 1;
        _img[i].cx          = _img[i].width >> 1;
        _img[i].cy          = _img[i].height >> 1;        
        _img[i].lineSize    = _img[i].width << _shift;
        _img[i].size        = _img[i].width * _img[i].height;
        _img[i].fullSize    = _img[i].lineSize * _img[i].height;
        _img[i].offset      = _img[i].fullSize * i;
        _img[i].offsetLine  = offsetLine;
        offsetLine += _img[i].height;
    }

    if (_bpp == _16BIT){
        uint16_t *line = (uint16_t*)buf;

        for (int i = 0; i < yy * num; i++) {
            lineBuf16[i] = line;
            line += xx; 
        }          
    } else {
        uint8_t *line = buf;

        for (int i = 0; i < yy * num; i++) {
            lineBuf8[i] = line;
            line += xx;
        }         
    }

    return (_created = true);
}

bool Sprite::allocateMemory(size_t size){
    if (size == 0) return false;

    freeMemory();
    #if defined(psramFound)
        _psram_ok = psramFound();
    #else
        _psram_ok = esp_psram_is_initialized();
    #endif

    size_t aligned = (size + 31) & ~size_t(31);
    if (_psram_ok) {
        buf = (uint8_t*)heap_caps_aligned_alloc(
            32, aligned,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
        );
    } else {
        buf = (uint8_t*)heap_caps_aligned_alloc(
            32, aligned,
            MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
        );
    }

    if (!buf) {
        Serial.println(F("Error: failed to allocate image buffer"));
        return false;
    }

    memset(buf, 0, aligned);
    return true;
}

bool Sprite::loadImages(const uint8_t* data){
    if (!_vga.vbInited() || !data) return false;

    uint8_t bpp = *data++;
    if (bpp != _8BIT && bpp != _16BIT) return false;
    _bpp = _vga.vbBPP();
    resetParam(*data++);

    data += _maxImage << 2;
    const uint8_t* ptr = data;    
    Serial.printf("bit: %d, imd: %d\n", bpp, _maxImage);

    int fullSize = 0;
    uint32_t offsetLine = 0;
    _shift = (_bpp == _16BIT ? 1 : 0);    
    for (int i = 0; i < _maxImage; i++) {
        _img[i].width      = (*data++) | (*data++ << 8);
        _img[i].height     = (*data++) | (*data++ << 8);
        Serial.printf("w: %d, h: %d\n", _img[i].width, _img[i].height);

        _img[i].xx         = _img[i].width - 1;
        _img[i].yy         = _img[i].height - 1;
        _img[i].cx         = _img[i].width >> 1;
        _img[i].cy         = _img[i].height >> 1;
        _img[i].lineSize   = _img[i].width << _shift;
        _img[i].size       = _img[i].width * _img[i].height;
        _img[i].fullSize   = _img[i].lineSize * _img[i].height;
        _img[i].offset     = fullSize;
        _img[i].offsetLine = offsetLine;

        data += (_img[i].width << (bpp == _16BIT ? 1 : 0)) * _img[i].height;
        offsetLine += _img[i].height;
        fullSize += _img[i].fullSize;
    }
    
    if (!allocateMemory(fullSize)) return false;

    uint32_t index = 0;
    uint32_t addr = 0;
    for (int i = 0; i < _maxImage; i++){
        for (int y = 0; y < _img[i].height; y++){
            if (_bpp == _16BIT){
                lineBuf16[index++] = (uint16_t*)(buf + addr);
            } else {
                lineBuf8[index++] = buf + addr;
            }

            addr += _img[i].lineSize;
        }
    }

    for (int i = 0; i < _maxImage; i++){
        ptr += 4;

        if (_bpp == bpp){
            memcpy(buf + _img[i].offset, ptr, _img[i].fullSize);
            ptr += _img[i].fullSize; 
        } else if ((bpp == _16BIT) && (_bpp == _8BIT)){
            const uint16_t* sour = (uint16_t*)ptr;
            uint8_t* dest = buf + _img[i].offset;
            int size = _img[i].size;

            while (size-- > 0){
                uint16_t col16 = *sour++;
                uint8_t r3 = (col16 >> 13) & 0x07;
                uint8_t g3 = (col16 >> 8)  & 0x07;
                uint8_t b2 = (col16 >> 3)  & 0x03;
                
                *dest++ = (r3 << 5) | (g3 << 2) | b2;
            }

            ptr += _img[i].size << 1;
        } else if ((bpp == _8BIT) && (_bpp == _16BIT)){
            const uint8_t* sour = ptr;
            uint16_t* dest = (uint16_t*)(buf + _img[i].offset);                       
            int size = _img[i].size;

            while (size-- > 0){
                uint8_t col8 = *sour++;

                uint8_t r3 = (col8 >> 5) & 0x07;
                uint8_t g3 = (col8 >> 2) & 0x07;
                uint8_t b2 =  col8       & 0x03;

                uint16_t r5 = (r3 << 2) | (r3 >> 1);
                uint16_t g6 = (g3 << 3) |  g3;
                uint16_t b5 = (b2 << 3) | (b2 << 1) | (b2 >> 1);

                *dest++ = (r5 << 11) | (g6 << 5) | b5;
            }

            ptr += _img[i].size;
        }               
    }

    return (_created = true);
}

void Sprite::sendToEve(uint32_t addr, uint16_t num, uint8_t format){
    if (!_created || num >= _maxImage) return; 
    _vga.sendData(addr, buf + _img[num].offset, _img[num].fullSize);    
}
/*
for (int i = 0; i < _maxImage; i++){
    ptr += 4; // пропускаем width/height

    if (_vga.vbBPP() == _bpp){

        memcpy(buf + _img[i].offset, ptr, _img[i].fullSize);
        ptr += _img[i].fullSize;

    } else if ((_bpp == _16BIT) && (_vga.vbBPP() == _8BIT)){

        int pixels = _img[i].width * _img[i].height;

        for (int p = 0; p < pixels; p++){

            uint16_t col16 = (uint16_t)ptr[p*2] | ((uint16_t)ptr[p*2+1] << 8);

            uint8_t r3 = (col16 >> 13) & 0x07;
            uint8_t g3 = (col16 >> 8)  & 0x07;
            uint8_t b2 = (col16 >> 3)  & 0x03;

            buf[_img[i].offset + p] = (r3 << 5) | (g3 << 2) | b2;
        }

        ptr += pixels * 2;

    } else if ((_bpp == _8BIT) && (_vga.vbBPP() == _16BIT)){

        int pixels = _img[i].width * _img[i].height;

        uint16_t* dst = (uint16_t*)(buf + _img[i].offset);

        for (int p = 0; p < pixels; p++){

            uint8_t c = ptr[p];

            uint8_t r3 = (c >> 5) & 0x07;
            uint8_t g3 = (c >> 2) & 0x07;
            uint8_t b2 =  c       & 0x03;

            uint16_t r5 = (r3 << 2) | (r3 >> 1);
            uint16_t g6 = (g3 << 3) |  g3;
            uint16_t b5 = (b2 << 3) | (b2 << 1) | (b2 >> 1);

            dst[p] = (r5 << 11) | (g6 << 5) | b5;
        }

        ptr += pixels;

    }
}
*/

void Sprite::putImage(int x, int y, uint16_t num){
    if (!_created || num >= _maxImage) return;    
    if (x > _vga.vbvX2() || y > _vga.vbvY2()) return;
    if (num >= _maxImage) return;
    int xx = x + _img[num].xx;
    int yy = y + _img[num].yy;
    if (xx < _vga.vbvX1() || yy < _vga.vbvY1()) return;

    int sxl = (x  < _vga.vbvX1() ? (_vga.vbvX1() - x)  : 0);
    int sxr = (xx > _vga.vbvX2() ? (xx - _vga.vbvX2()) : 0);
    int syu = (y  < _vga.vbvY1() ? (_vga.vbvY1() - y)  : 0);
    int syd = (yy > _vga.vbvY2() ? (yy - _vga.vbvY2()) : 0);

    int copyX = (_img[num].width  - sxl - sxr) << _shift;
    int copyY =  _img[num].height - syu - syd;
    if (copyX <= 0 || copyY <= 0) return;

    //uint8_t* img = buf + (syu * _img[num].lineSize) + (sxl << _vga.Shift()) + _img[num].offset;
    uint8_t* img = (_bpp == _16BIT)
        ? (uint8_t*)&lineBuf16[_img[num].offsetLine + syu][sxl]
        : (uint8_t*)&lineBuf8[_img[num].offsetLine + syu][sxl];
    uint8_t* scr = (_bpp == _16BIT)
        ? (uint8_t*)&_vga.lineBuf16[y + syu][x + sxl]
        : (uint8_t*)&_vga.lineBuf8 [y + syu][x + sxl];

    int scrLineSize   = _vga.vbLineSize();
    int imageLineSize = _img[num].lineSize;

    while (copyY-- > 0){
        memcpy(scr, img, copyX);
        scr += scrLineSize;
        img += imageLineSize;
    }
}

void Sprite::cls(uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;

    if (_bpp == _16BIT){
        uint16_t* scr = (uint16_t*)(buf + _img[num].offset);

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(scr, (uint8_t)(col & 0xFF), _img[num].fullSize);
        } else {
            uint16_t* cpy = scr;

            int size = 0;
            while (size++ < _img[num].width) *scr++ = col;

            int dummy = 1; 
            int lines = _img[num].yy;  
            int copyBytes = _img[num].lineSize;
            int offset = _img[num].width;
        
            while (lines > 0){ 
                if (lines >= dummy){
                    memcpy(scr, cpy, copyBytes);
                    lines -= dummy;
                    scr += offset;
                    copyBytes <<= 1;
                    offset <<= 1;
                    dummy <<= 1;
                } else {
                    copyBytes =_img[num].lineSize * lines;
                    memcpy(scr, cpy, copyBytes);
                    break;
                }
            }            
        }
    } else {
        memset(buf + _img[num].offset, (uint8_t)(col & 0xFF), _img[num].fullSize);
    }
}

void Sprite::putPixel(int x, int y, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;   
    if (x < 0 || y < 0 || x >= _img[num].width ||  y >= _img[num].height) return;

    if (_bpp == _16BIT){
        lineBuf16[_img[num].offsetLine + y][x] = col;
    } else {
        lineBuf8[_img[num].offsetLine + y][x] = (uint8_t)(col & 0xFF);
    }
}

void Sprite::putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint16_t num){
    if (!_created || num >= _maxImage) return;
    if (x < 0 || y < 0 || x > _img[num].xx ||  y > _img[num].yy) return;
 
    if (_bpp == _16BIT){
        lineBuf16[_img[num].offsetLine + y][x] = RGB16(r, g, b);
    } else {
        lineBuf8[_img[num].offsetLine + y][x] = RGB8(r, g, b);
    }
}

void Sprite::hLine(int x1, int y, int x2, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;
    if (y < 0 || y >= _img[num].height) return;

    if (x1 > x2) std::swap(x1, x2);
    if (x1 >= _img[num].width || x2 < 0) return;
    x1 = std::max(0, x1);
    x2 = std::min(_img[num].xx, x2); 
    
    if (_bpp == _16BIT){
        uint16_t* scr = lineBuf16[_img[num].offsetLine + y] + x1;

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(scr, (uint8_t)(col & 0xFF), x2 - x1 + 1);
        } else {
            while (x1++ <= x2) *scr++ = col;
        }          
    } else {
        uint8_t* scr = lineBuf8[_img[num].offsetLine + y] + x1;
        memset(scr, (uint8_t)(col & 0xFF), x2 - x1 + 1);        
    }
}

void Sprite::vLine(int x, int y1, int y2, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return; 
    if (x < 0 || x >= _img[num].width) return;

    if (y1 > y2) std::swap(y1, y2);
    if (y1 >= _img[num].height || y2 < 0) return;
    y1 = std::max(0, y1);
    y2 = std::min(_img[num].yy, y2); 
    
    int skip = _img[num].width;
    if (_bpp == _16BIT){ 
        uint16_t* scr = lineBuf16[_img[num].offsetLine + y1] + x;

        while (y1++ <= y2){ 
            *scr = col;
            scr += skip;
        }
    } else {
        uint8_t* scr = lineBuf8[_img[num].offsetLine + y1] + x;

        uint8_t color = (uint8_t)(col & 0xFF);
        while (y1++ <= y2){ 
            *scr = color;
            scr += skip;
        }       
    }    
}

void Sprite::rect(int x1, int y1, int x2, int y2, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (x1 >= _img[num].width || y1 >= _img[num].height || x2 < 0 || y2 < 0) return;

    if (x1 >= 0 && y1 >= 0 && x2 < _img[num].width && y2 < _img[num].height){
        int sizeX = x2 - x1 + 1; 
        int sizeX2X = sizeX << 1;
        int sizeY = y2 - y1 - 1;
        int width = _img[num].width;
        int skip1 = x2 - x1; 
        int skip2 = width - x2 + x1;
        int offset = width * y1 + x1;

        if (_bpp == _16BIT){
            uint16_t* scr = lineBuf16[_img[num].offsetLine + y1] + x1;
            uint16_t* cpy = scr;
            
            if ((uint8_t)col == (uint8_t)(col >> 8)){
                memset(scr, col & 0xFF, sizeX << 1);
                scr += sizeX;
            } else {
                while (sizeX-- > 0) *scr++ = col;
            }            
            scr += skip2 - 1;

            while (sizeY-- > 0){
                *scr = col; scr += skip1;
                *scr = col; scr += skip2;
            }
            memcpy(scr, cpy, sizeX2X);
        } else {
            uint8_t* scr = lineBuf8[_img[num].offsetLine + y1] + x1;
            uint8_t color = (uint8_t)col;

            memset(scr, color, sizeX);
            scr += width;

            while (sizeY-- > 0){
                *scr = color; scr += skip1;
                *scr = color; scr += skip2;
            }
            memset(scr, color, sizeX);
        }
    } else {
        hLine(x1, y1, x2, col); 
        hLine(x1, y2, x2, col); 
        vLine(x1, y1, y2, col);  
        vLine(x2, y1, y2, col);          
    }
}

void Sprite::fillRect(int x1, int y1, int x2, int y2, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (x1 >= _img[num].width || y1 >= _img[num].height || x2 < 0 || y2 < 0) return;

    x1 = std::max(0, x1);
    x2 = std::min(_img[num].xx, x2);
    y1 = std::max(0, y1);
    y2 = std::min(_img[num].yy, y2);
    
    int sizeX = x2 - x1 + 1;
    int sizeY = y2 - y1 + 1;     
    int width = _img[num].width;
    
    if (_bpp == _16BIT){
        uint16_t* scr = lineBuf16[_img[num].offsetLine + y1] + x1;
        uint16_t* savePos = scr;

        int skip = width - x2 + x1 - 1;
        int copyBytes = sizeX << 1;
        int saveSizeX = sizeX; 

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(scr, col & 0xFF, sizeX << 1);
            scr += sizeX;
        } else {
            while (sizeX-- > 0) *scr++ = col;
        }
        scr += skip;
        sizeY--;

        while (sizeY-- > 0){
            memcpy(scr, savePos, copyBytes);
            scr += width;
        }
    } else {
        uint8_t* scr = lineBuf8[_img[num].offsetLine + y1] + x1;
        uint8_t color = (uint8_t)(col & 0xFF);
        
        while (sizeY-- > 0){
            memset(scr, color, sizeX);
            scr += width;
        } 
    }
}

void Sprite::line(int x1, int y1, int x2, int y2, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;

    if (x1 == x2) {
        vLine(x1, y1, y2, col);
    } else if (y1 == y2){
        hLine(x1, y1, x2, col);
    } else {
        // Алгоритм Брезенхэма
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1; 
        int err = dx - dy;
        uint8_t color = (uint8_t)(col & 0xFF);

        while (true) {
            if (x1 >= 0 && y1 >= 0 && x1 < _img[num].width &&  y1 < _img[num].height){
                if (_bpp == _16BIT){
                    lineBuf16[_img[num].offsetLine + y1][x1] = col;
                } else {
                    lineBuf8[_img[num].offsetLine + y1][x1] = color;
                }
            }

            if (x1 == x2 && y1 == y2) break;
            int err2 = err * 2;
            if (err2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (err2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }    
}

void Sprite::triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col, uint16_t num){
    line(x1, y1, x2, y2, col, num);
    line(x2, y2, x3, y3, col, num);
    line(x3, y3, x1, y1, col, num);
}

void Sprite::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;

    // Сортируем вершины по y (y1 <= y2 <= y3)
    if (y1 > y2){ std::swap(y1, y2); std::swap(x1, x2); }
    if (y1 > y3){ std::swap(y1, y3); std::swap(x1, x3); }
    if (y2 > y3){ std::swap(y2, y3); std::swap(x2, x3); }

    auto drawScanline = [&](int y, int xStart, int xEnd){
        hLine(xStart, y, xEnd, col, num);
    };

    auto edgeInterp = [](int y1, int x1, int y2, int x2, int y) -> int {
        if (y2 == y1) return x1; // избежать деления на 0
        return x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    };

    // Верхняя часть треугольника
    for (int y = y1; y <= y2; y++){
        int xa = edgeInterp(y1, x1, y3, x3, y);
        int xb = edgeInterp(y1, x1, y2, x2, y);
        if (xa > xb) std::swap(xa, xb);
        drawScanline(y, xa, xb);
    }

    // Нижняя часть треугольника
    for (int y = y2 + 1; y <= y3; y++){
        int xa = edgeInterp(y1, x1, y3, x3, y);
        int xb = edgeInterp(y2, x2, y3, x3, y);
        if (xa > xb) std::swap(xa, xb);
        drawScanline(y, xa, xb);
    }    
}

void Sprite::circle(int xc, int yc, int r, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;

    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    auto drawCircleSegments = [&](int x, int y){
        // Рисуем горизонтальные линии длиной 1 для верх/низ
        putPixel(xc + x, yc + y, col, num);
        putPixel(xc - x, yc + y, col, num);
        putPixel(xc + x, yc - y, col, num);
        putPixel(xc - x, yc - y, col, num);

        if (x != y){
            putPixel(xc + y, yc + x, col, num);
            putPixel(xc - y, yc + x, col, num);
            putPixel(xc + y, yc - x, col, num);
            putPixel(xc - y, yc - x, col, num);
        }
    };

    while (y >= x){
        drawCircleSegments(x, y);
        x++;
        if (d > 0){
            y--;
            d = d + ((x - y) << 2) + 10;
        } else {
            d = d + (x << 2) + 6;
        }
    }    
}

void Sprite::fillCircle(int xc, int yc, int r, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;

    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    // верхняя и нижняя линии по центру
    hLine(xc - r, yc, xc + r, col, num);

    while (y >= x){
        // Рисуем линии для 4-х сегментов (верх/низ) одной итерацией
        if (x > 0){
            hLine(xc - y, yc + x, xc + y, col, num);
            hLine(xc - y, yc - x, xc + y, col, num);
        }
        if (y > x){
            hLine(xc - x, yc + y, xc + x, col, num);
            hLine(xc - x, yc - y, xc + x, col, num);
        }

        x++;
        if (d > 0){
            y--;
            d = d + ((x - y) << 2) + 10;
        } else {
            d = d + (x << 2) + 6;
        }
    }    
}

void Sprite::polygon(int x, int y, int radius, int sides, int ang, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;

    int* vx = new int[sides];
    int* vy = new int[sides];    
    int angleStep = 360 / sides;

    // Вычисляем вершины через LUT
    for (int i = 0; i < sides; i++){
        int angle = ang + i * angleStep;
        _vga.LUT(vx[i], vy[i], x, y, radius, angle);
    }

    // Соединяем вершины линиями
    for (int i = 0; i < sides; i++){
        int x1 = vx[i];
        int y1 = vy[i];
        int x2 = vx[(i + 1) % sides];
        int y2 = vy[(i + 1) % sides];

        line(x1, y1, x2, y2, col, num); 
    }

    delete[] vx;
    delete[] vy;    
}

void Sprite::fillPolygon(int x, int y, int radius, int sides, int rotation, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;

    int* vx = new int[sides];
    int* vy = new int[sides];
    int angleStep = 360 / sides;

    // Вычисляем вершины через LUT
    for (int i = 0; i < sides; i++){
        int angle = rotation + i * angleStep;
        _vga.LUT(vx[i], vy[i], x, y, radius, angle);
    }

    // Алгоритм scanline для закрашивания
    int yMin = vy[0], yMax = vy[0];
    for (int i = 1; i < sides; i++){
        if (vy[i] < yMin) yMin = vy[i];
        if (vy[i] > yMax) yMax = vy[i];
    }

    for (int yCur = yMin; yCur <= yMax; yCur++){
        // собираем все пересечения текущей строки с ребрами
        int intersections[sides];
        int n = 0;

        for (int i = 0; i < sides; i++){
            int x1 = vx[i], y1 = vy[i];
            int x2 = vx[(i + 1) % sides], y2 = vy[(i + 1) % sides];

            if ((yCur >= y1 && yCur < y2) || (yCur >= y2 && yCur < y1)){
                int xInt = x1 + (yCur - y1) * (x2 - x1) / (y2 - y1);
                intersections[n++] = xInt;
            }
        }

        // сортировка пересечений
        for (int i = 0; i < n-1; i++){
            for (int j = i+1; j < n; j++){
                if (intersections[i] > intersections[j]) std::swap(intersections[i], intersections[j]);
            }
        }

        // рисуем горизонтальные линии между парами пересечений
        for (int i = 0; i < n; i += 2){
            if (i+1 < n){
                hLine(intersections[i], yCur, intersections[i+1], col, num);
            }
        }
    }

    delete[] vx;
    delete[] vy;    
}

void Sprite::star(int x, int y, int radius, int sides, int ang, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage || sides < 2) return;

    int points = sides * 2; // удвоенное количество вершин (внешние+внутренние)
    int* vx = new int[points];
    int* vy = new int[points];

    int angleStep = 360 / points;

    for (int i = 0; i < points; i++){
        int r = (i % 2 == 0) ? radius : radius / 2; // чётные - внешний радиус, нечётные - внутренний
        int angle = ang + i * angleStep;
        _vga.LUT(vx[i], vy[i], x, y, r, angle);
    }

    // соединяем вершины линиями
    for (int i = 0; i < points; i++){
        int x1 = vx[i],     y1 = vy[i];
        int x2 = vx[(i+1)%points], y2 = vy[(i+1)%points];
        line(x1, y1, x2, y2, col, num);
    }

    delete[] vx;
    delete[] vy;
}

void Sprite::fillStar(int x, int y, int radius, int sides, int rotation, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage || sides < 2) return;

    int points = sides * 2;
    int* vx = new int[points];
    int* vy = new int[points];

    int angleStep = 360 / points;

    for (int i = 0; i < points; i++){
        int r = (i % 2 == 0) ? radius : radius / 2;
        int angle = rotation + i * angleStep;
        _vga.LUT(vx[i], vy[i], x, y, r, angle);
    }

    // используем тот же scanline, что и в fillPolygon
    int yMin = vy[0], yMax = vy[0];
    for (int i = 1; i < points; i++){
        if (vy[i] < yMin) yMin = vy[i];
        if (vy[i] > yMax) yMax = vy[i];
    }

    for (int yCur = yMin; yCur <= yMax; yCur++){
        int intersections[points];
        int n = 0;

        for (int i = 0; i < points; i++){
            int x1 = vx[i], y1 = vy[i];
            int x2 = vx[(i + 1) % points], y2 = vy[(i + 1) % points];

            if ((yCur >= y1 && yCur < y2) || (yCur >= y2 && yCur < y1)){
                int xInt = x1 + (yCur - y1) * (x2 - x1) / (y2 - y1);
                intersections[n++] = xInt;
            }
        }

        // сортируем пересечения
        for (int i = 0; i < n-1; i++){
            for (int j = i+1; j < n; j++){
                if (intersections[i] > intersections[j])
                    std::swap(intersections[i], intersections[j]);
            }
        }

        // рисуем горизонтальные линии
        for (int i = 0; i < n; i += 2){
            if (i+1 < n){
                hLine(intersections[i], yCur, intersections[i+1], col, num);
            }
        }
    }

    delete[] vx;
    delete[] vy;    
}

void Sprite::ellipse(int xc, int yc, int rx, int ry, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;

    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int twoRx2 = 2 * rx2;
    int twoRy2 = 2 * ry2;

    int x = 0;
    int y = ry;
    int px = 0;
    int py = twoRx2 * y;

    // Первая часть (dx/dy < 1)
    int p = round(ry2 - (rx2 * ry) + (0.25 * rx2));
    while (px < py) {
        putPixel(xc + x, yc + y, col, num);
        putPixel(xc - x, yc + y, col, num);
        putPixel(xc + x, yc - y, col, num);
        putPixel(xc - x, yc - y, col, num);

        x++;
        px += twoRy2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }
    }

    // Вторая часть (dx/dy >= 1)
    p = round(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y >= 0) {
        putPixel(xc + x, yc + y, col, num);
        putPixel(xc - x, yc + y, col, num);
        putPixel(xc + x, yc - y, col, num);
        putPixel(xc - x, yc - y, col, num);

        y--;
        py -= twoRx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }
    }
}

void Sprite::fillEllipse(int xc, int yc, int rx, int ry, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;

    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int twoRx2 = 2 * rx2;
    int twoRy2 = 2 * ry2;

    int x = 0;
    int y = ry;
    int px = 0;
    int py = twoRx2 * y;

    // Первая часть (dx/dy < 1)
    int p = round(ry2 - (rx2 * ry) + (0.25 * rx2));
    while (px < py) {
        hLine(xc - x, yc + y, xc + x, col, num);
        hLine(xc - x, yc - y, xc + x, col, num);

        x++;
        px += twoRy2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }
    }

    // Вторая часть (dx/dy >= 1)
    p = round(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y >= 0) {
        hLine(xc - x, yc + y, xc + x, col, num);
        hLine(xc - x, yc - y, xc + x, col, num);

        y--;
        py -= twoRx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }
    }    
}

void Sprite::arc(int xc, int yc, int r, int angle1, int angle2, uint16_t col, uint16_t num){
    if (!_created || num >= _maxImage) return;

    if (angle1 > angle2) std::swap(angle1, angle2);
    angle1 %= 360;
    angle2 %= 360;

    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    auto inAngle = [&](int angle) {
        if (angle1 <= angle2) return (angle >= angle1 && angle <= angle2);
        return (angle >= angle1 || angle <= angle2); // переход через 360
    };

    auto plot = [&](int dx, int dy) {
        // вычисляем угол точки относительно центра
        int angle = (int)(atan2((float)dy, (float)dx) * 180.0 / M_PI);
        if (angle < 0) angle += 360;
        if (inAngle(angle)) putPixel(xc + dx, yc + dy, col, num);
    };

    while (y >= x) {
        plot(x, y);
        plot(-x, y);
        plot(x, -y);
        plot(-x, -y);
        plot(y, x);
        plot(-y, x);
        plot(y, -x);
        plot(-y, -x);

        x++;
        if (d > 0) {
            y--;
            d += 4 * (x - y) + 10;
        } else {
            d += 4 * x + 6;
        }
    }    
}