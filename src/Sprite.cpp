#include "Arduino.h"
#include "Sprite.h"

Sprite::Sprite(EVE_esp32& vga) : _vga(vga){

}

Sprite::~Sprite(){
    destroy();
}

bool Sprite::allocateMemory(size_t size){
    if (size <= 0) return false;

    if (buf) {
        free(buf);
        buf = nullptr;
    }

    buf = (uint8_t*)heap_caps_aligned_alloc(32, size, MALLOC_CAP_DMA);

    if (!buf) {
        Serial.println(F("Error: failed to allocate image buffer"));
        return false;
    } else memset(buf, 0, size);

    return true;
}

void Sprite::destroy(){
    if (buf) { free(buf); buf = nullptr; }
    if (info) { delete[] info; info = nullptr; }
}

bool Sprite::loadImages(const uint8_t* data){
    if (!data) return false;

    bpp = *data++;
    if (bpp != 8 && bpp != 16) return false;
    frames = *data++;
    int count = frames + 1; // + основной кадр

    if (info) destroy();
    info = new Image[count];
    memset(info, 0, sizeof(Image) * count);
    Serial.printf("Loading sprite: BPP - %d, Frames - %d\n", bpp, count);

    int fullSize = 0;
    data += count * 4; // пропускаем оффсеты
    uint8_t *ptr = (uint8_t*)data;

    for (int i = 0; i < count; i++) {
        info[i].width  = (*data++) | (*data++ << 8);
        info[i].height = (*data++) | (*data++ << 8);        
        info[i].xx     = info[i].width - 1;
        info[i].yy     = info[i].height - 1;
        info[i].cx     = info[i].width >> 1;
        info[i].cy     = info[i].height >> 1;

        int lineSize = info[i].width * (bpp == 16 ? 2 : 1);
        int size = info[i].height * lineSize;

        info[i].lineSize = info[i].width * (_vga.BPP() == 16 ? 2 : 1);
        info[i].size = info[i].height * info[i].lineSize;
        info[i].offset = (i == 0) ? 0 : (info[i - 1].offset + info[i - 1].size);

        Serial.printf("Loading frame %d: Width - %d, Height - %d, lineSize: %d, Size: %d, ", i, info[i].width, info[i].height, info[i].lineSize, info[i].size);
        Serial.printf("offset: %d\n", info[i].offset);
        data += size;
        fullSize += info[i].size;
    }
    Serial.printf("Total sprite size: %d bytes\n", fullSize);

    allocateMemory(fullSize);
    for (int i = 0; i < count; i++) {
        ptr += 4; // пропускаем ширину и высоту

        if (bpp == _vga.BPP()){

            memcpy(buf + info[i].offset, ptr, info[i].size);

        } else if (bpp == 16 && _vga.BPP() == 8){ 

            int pixels = info[i].width * info[i].height;
            for (int ii = 0; ii < pixels; ii++){
                int j = ii * 2;
                uint16_t col16 = (uint16_t)ptr[j] | ((uint16_t)ptr[j + 1] << 8);

                uint8_t rr = (col16 >> 13) & 0x07;
                uint8_t gg = (col16 >> 8)  & 0x07;
                uint8_t bb = (col16 >> 3) & 0x03;

                buf[info[i].offset + ii] = (rr << 5) | (gg << 2) | bb;
            }

        } else {
            int pixels = info[i].width * info[i].height;

            // источник 8bpp
            const uint8_t* src8 = ptr;

            // назначение 16bpp (буфер у тебя uint8_t*, поэтому кастим)
            uint16_t* dst16 = (uint16_t*)(buf + info[i].offset);

            for (int ii = 0; ii < pixels; ii++){
                uint8_t c = src8[ii];

                uint8_t r3 = (c >> 5) & 0x07;
                uint8_t g3 = (c >> 2) & 0x07;
                uint8_t b2 =  c       & 0x03;

                uint16_t r5 = (uint16_t)((r3 << 2) | (r3 >> 1));
                uint16_t g6 = (uint16_t)((g3 << 3) | (g3));
                uint16_t b5 = (uint16_t)((b2 << 3) | (b2 << 1) | (b2 >> 1));

                dst16[ii] = (r5 << 11) | (g6 << 5) | b5;
            }
        }

        int srcLineSize = info[i].width * (bpp == 16 ? 2 : 1);
        int srcSize     = info[i].height * srcLineSize;
        ptr += srcSize;   
    }

    //Serial.printf("Loading sprite: BPP - %d, Frames - %d\n", bpp, frames);
    //for (int i = 0; i < count; i++) Serial.printf("Frame %d: Offset - %d, Size - %d bytes\n", i, info[i].offset, info[i].size);
  
    return true;
}

void Sprite::putImage(int x, int y, int num){
    if (!_vga.vbInited() || !buf || x > _vga.vbvX2() || y > _vga.vbvY2()) return;
    if (num < 0 || num > frames) return;
    int xx = x + info[num].xx;
    int yy = y + info[num].yy;
    if (xx < _vga.vbvX1() || yy < _vga.vbvY1()) return;
    
    int sxl = (x  < _vga.vbvX1() ? (_vga.vbvX1() - x)  : 0); //Skip X left    
    int sxr = (xx > _vga.vbvX2() ? (xx - _vga.vbvX2()) : 0); //Skip X right
    int syu = (y  < _vga.vbvY1() ? (_vga.vbvY1() - y)  : 0); //Skip Y up
    int syd = (yy > _vga.vbvY2() ? (yy - _vga.vbvY2()) : 0); //Skip Y down

    int copyX = info[num].lineSize - ((sxr + sxl) << _vga.Shift());
    int copyY = info[num].height - syd - syu;
    if (copyX <= 0 || copyY <= 0) return;

    uint8_t* img = buf + (syu * info[num].lineSize) + (sxl << _vga.Shift()) + info[num].offset;
    uint8_t* scr = (_vga.BPP() == 16)
        ? (uint8_t*)&_vga.lineBuf16[y + syu][x + sxl]
        : (uint8_t*)&_vga.lineBuf8 [y + syu][x + sxl];
  
    while(copyY-- > 0){
        memcpy(scr, img, copyX);
        scr += _vga.vbLineSize();
        img += info[0].lineSize;
    }  
}

EVE_Image Sprite::sendToEve(uint32_t addr, uint16_t num, uint8_t format){
    EVE_Image img{};     
    if (!buf || num > frames) return img;
     
    img.addr = addr;
    img.width   = info[num].width;
    img.height  = info[num].height;
    img.xx      = info[num].xx;
    img.yy      = info[num].yy;
    img.cx      = info[num].cx;
    img.cy      = info[num].cy;
    uint32_t pos = addr;

    switch (format){
        case ARGB1555:{
            img.format = ARGB1555;
            uint16_t* image = (uint16_t*)buf + (info[num].offset >> 1);
            uint16_t* dest = (uint16_t*)malloc(img.width << 1);
            int copyBytes = img.width << 1;

            if (!dest) return img;
            for (int y = 0; y < img.height; y++){
                for (int x = 0; x < img.width; x++){
                    uint16_t col = *image++;
                    uint16_t color = (((col >> 11) & 0x1F) << 10) | (((col >> 6) & 0x1F) << 5) | (col & 0x1F);
                    color |= (col == 0x07E0 ? 0 : 0x8000);
                    dest[x] = color;
                }

                _vga.sendData(pos, dest, copyBytes);
                pos += copyBytes;
            }

            free(dest);
            break;
        }

        case 2:
            break;
        default:
            return img;
    }

    return img;
}