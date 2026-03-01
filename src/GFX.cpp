#include "GFX.h"

GFX::GFX(EVE_esp32& vga) : _vga(vga){

}

GFX::~GFX(){

}

void GFX::cls(uint16_t col){
    if (!_vga.vbInited()) return;
    
    if (_vga.vbBPP() == 16){
        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(_vga.scrBuf, (uint8_t)col, _vga.vbFullSize());
        } else {
            uint16_t* scr = (uint16_t*)_vga.scrBuf;
            uint16_t* cpy = scr;

            int size = 0;
            while (size++ <= _vga.vbXX()) *scr++ = col;   

            int dummy = 1; 
            int lines = _vga.vbYY();  
            int copyBytes = _vga.vbLineSize();
            int offset = _vga.vbWidth();
        
            while (lines > 0){ 
                if (lines >= dummy){
                    memcpy(scr, cpy, copyBytes);
                    lines -= dummy;
                    scr += offset;
                    copyBytes <<= 1;
                    offset <<= 1;
                    dummy <<= 1;
                } else {
                    copyBytes = _vga.vbLineSize() * lines;
                    memcpy(scr, cpy, copyBytes);
                    break;
                }
            }
        } 
    } else {
        uint8_t* scr = _vga.scrBuf;
        memset(scr, (uint8_t)col, _vga.vbFullSize());
    }
}

void GFX::putPixel(int x, int y, uint16_t col){
    if (!_vga.vbInited() || x < _vga.vbvX1() || y < _vga.vbvY1() || x > _vga.vbvX2() ||  y > _vga.vbvY2()) return;

    if (_vga.BPP() == 16){
        _vga.lineBuf16[y][x] = col;
    } else {
        _vga.lineBuf8[y][x] = (uint8_t)(col & 0xFF);
    }
}

void GFX::hLine(int x1, int y, int x2, uint16_t col){
    if (!_vga.vbInited() || y < _vga.vbvY1() || y > _vga.vbvY2()) return;

    if (x1 > x2) std::swap(x1, x2);
    if (x1 > _vga.vbvX2() || x2 < _vga.vbvX1()) return;
    x1 = std::max(_vga.vbvX1(), x1);
    x2 = std::min(_vga.vbvX2(), x2);

    if (_vga.BPP() == 16){ 
        uint16_t* scr = _vga.lineBuf16[y] + x1;
        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(_vga.scrBuf, (uint8_t)col, x2 - x1 + 1);
        } else {
            while (x1++ <= x2) *scr++ = col;
        }    
    } else {
        uint8_t* scr = _vga.lineBuf8[y] + x1;
        memset(scr, (uint8_t)(col & 0xFF), x2 - x1 + 1);
    }        
}

void GFX::vLine(int x, int y1, int y2, uint16_t col){
    if (!_vga.vbInited() || x < _vga.vbvX1() || x > _vga.vbvX2()) return;

    if (y1 > y2) std::swap(y1, y2);
    if (y1 > _vga.vbvY2() || y2 < _vga.vbvY1()) return;
    y1 = std::max(_vga.vbvY1(), y1);
    y2 = std::min(_vga.vbvY2(), y2);

    if (_vga.BPP() == 16){ 
        uint16_t* scr = _vga.lineBuf16[y1] + x;
        while (y1++ <= y2){ 
            *scr = col;
            scr += _vga.Width();
        }
    } else {
        uint8_t* scr = _vga.lineBuf8[y1] + x;
        uint8_t color = (uint8_t)(col & 0xFF);
        while (y1++ <= y2){ 
            *scr = color;
            scr += _vga.Width();
        }       
    }
}

void GFX::rect(int x1, int y1, int x2, int y2, uint16_t col){
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    if (!_vga.vbInited() || x1 > _vga.vbvX2() || y1 > _vga.vbvY2() || x2 < _vga.vbvX1() || y2 < _vga.vbvY1()){
        return;
    } else if (x1 >= _vga.vbvX1() && y1 >= _vga.vbvY1() && x2 <= _vga.vbvX2() && y2 <= _vga.vbvY2()){
        int sizeX = x2 - x1 + 1; 
        int sizeX2X = sizeX << 1;
        int sizeY = y2 - y1 - 1;
        int width = _vga.Width();
        int skip1 = x2 - x1; 
        int skip2 = width - x2 + x1;
        int offset = width * y1 + x1;

        if (_vga.BPP() == 16){
            uint16_t* scr = _vga.lineBuf16[y1] + x1;
            uint16_t* cpy = scr;
            
            while (sizeX-- > 0) *scr++ = col;
            scr += skip2 - 1;

            while (sizeY-- > 0){
                *scr = col; scr += skip1;
                *scr = col; scr += skip2;
            }
            memcpy(scr, cpy, sizeX2X);
        } else {
            uint8_t* scr = _vga.lineBuf8[y1] + x1;
            uint8_t color = (uint8_t) col;

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

void GFX::fillRect(int x1, int y1, int x2, int y2, uint16_t col){
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (!_vga.vbInited() || x1 > _vga.vbvX2() || y1 > _vga.vbvY2() || x2 < _vga.vbvX1() || y2 < _vga.vbvY1()) return; 

    x1 = std::max(_vga.vbvX1(), x1);
    x2 = std::min(_vga.vbvX2(), x2);
    y1 = std::max(_vga.vbvY1(), y1);
    y2 = std::min(_vga.vbvY2(), y2);
    
    int sizeX = x2 - x1 + 1;
    int sizeY = y2 - y1 + 1;     
    int width = _vga.Width();

    if (_vga.BPP() == 16){
        uint16_t* scr = _vga.lineBuf16[y1] + x1;
        uint16_t* savePos = scr;

        int skip = width - x2 + x1 - 1;
        int copyBytes = sizeX << 1;
        int saveSizeX = sizeX; 

        while (sizeX-- > 0) *scr++ = col;
        scr += skip;
        sizeY--;

        while (sizeY-- > 0){
            memcpy(scr, savePos, copyBytes);
            scr += width;
        }
    } else {
        uint8_t* scr = _vga.lineBuf8[y1] + x1;
        uint8_t color = (uint8_t)(col & 0xFF);
        
        while (sizeY-- > 0){
            memset(scr, color, sizeX);
            scr += width;
        }    
    }     
}

void GFX::line(int x1, int y1, int x2, int y2, uint16_t col){
    if (!_vga.vbInited()) return;

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
            if (x1 >= _vga.vbvX1() && x1 <= _vga.vbvX2() && y1 >= _vga.vbvY1() && y1 <= _vga.vbvY2()){
                if (_vga.BPP() == 16){
                    _vga.lineBuf16[y1][x1] = col;
                } else {
                    _vga.lineBuf8[y1][x1] = color;
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

void GFX::triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
    if (!_vga.vbInited()) return;

    line(x1, y1, x2, y2, col);
    line(x2, y2, x3, y3, col);
    line(x3, y3, x1, y1, col);
}

void GFX::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
    if (!_vga.vbInited()) return;

    // Сортируем вершины по y (y1 <= y2 <= y3)
    if (y1 > y2){ std::swap(y1, y2); std::swap(x1, x2); }
    if (y1 > y3){ std::swap(y1, y3); std::swap(x1, x3); }
    if (y2 > y3){ std::swap(y2, y3); std::swap(x2, x3); }

    auto drawScanline = [&](int y, int xStart, int xEnd){
        hLine(xStart, y, xEnd, col);
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

void GFX::circle(int xc, int yc, int r, uint16_t col){
    if (!_vga.vbInited()) return;

    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    auto drawCircleSegments = [&](int x, int y){
        // Рисуем горизонтальные линии длиной 1 для верх/низ
        putPixel(xc + x, yc + y, col);
        putPixel(xc - x, yc + y, col);
        putPixel(xc + x, yc - y, col);
        putPixel(xc - x, yc - y, col);

        if (x != y){
            putPixel(xc + y, yc + x, col);
            putPixel(xc - y, yc + x, col);
            putPixel(xc + y, yc - x, col);
            putPixel(xc - y, yc - x, col);
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

void GFX::fillCircle(int xc, int yc, int r, uint16_t col){
    if (!_vga.vbInited()) return;

    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    // верхняя и нижняя линии по центру
    hLine(xc - r, yc, xc + r, col);

    while (y >= x){
        // Рисуем линии для 4-х сегментов (верх/низ) одной итерацией
        if (x > 0){
            hLine(xc - y, yc + x, xc + y, col);
            hLine(xc - y, yc - x, xc + y, col);
        }
        if (y > x){
            hLine(xc - x, yc + y, xc + x, col);
            hLine(xc - x, yc - y, xc + x, col);
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

void GFX::polygon(int x, int y, int radius, int sides, int ang, uint16_t col){
    if (!_vga.vbInited() || sides < 3) return;        

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

        line(x1, y1, x2, y2, col); 
    }

    delete[] vx;
    delete[] vy;
}

void GFX::fillPolygon(int x, int y, int radius, int sides, int rotation, uint16_t col){
    if (!_vga.vbInited() || sides < 3) return;

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
                hLine(intersections[i], yCur, intersections[i+1], col);
            }
        }
    }

    delete[] vx;
    delete[] vy;
}

void GFX::star(int x, int y, int radius, int sides, int ang, uint16_t col){
    if (!_vga.vbInited() || sides < 2) return; // минимум 2 "луча"

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
        line(x1, y1, x2, y2, col);
    }

    delete[] vx;
    delete[] vy;
}

void GFX::fillStar(int x, int y, int radius, int sides, int rotation, uint16_t col){
    if (!_vga.vbInited() || sides < 2) return;

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
                hLine(intersections[i], yCur, intersections[i+1], col);
            }
        }
    }

    delete[] vx;
    delete[] vy;
}

void GFX::ellipse(int xc, int yc, int rx, int ry, uint16_t col){
    if (!_vga.vbInited()) return;

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
        putPixel(xc + x, yc + y, col);
        putPixel(xc - x, yc + y, col);
        putPixel(xc + x, yc - y, col);
        putPixel(xc - x, yc - y, col);

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
        putPixel(xc + x, yc + y, col);
        putPixel(xc - x, yc + y, col);
        putPixel(xc + x, yc - y, col);
        putPixel(xc - x, yc - y, col);

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

void GFX::fillEllipse(int xc, int yc, int rx, int ry, uint16_t col){
    if (!_vga.vbInited()) return;

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
        hLine(xc - x, yc + y, xc + x, col);
        hLine(xc - x, yc - y, xc + x, col);

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
        hLine(xc - x, yc + y, xc + x, col);
        hLine(xc - x, yc - y, xc + x, col);

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

void GFX::arc(int xc, int yc, int r, int angle1, int angle2, uint16_t col){
    if (!_vga.vbInited()) return;
        
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
        if (inAngle(angle)) putPixel(xc + dx, yc + dy, col);
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

void GFX::testRGBPanel(){
    if (!_vga.vbInited()) return;

    int cx = _vga.vbCX() - 128;
    int cy = _vga.vbCY() - 45;
 
    // clamp 256x90
    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    if (cx + 256 > _vga.vbWidth())  cx = _vga.vbWidth()  - 256;
    if (cy + 90  > _vga.vbHeight()) cy = _vga.vbHeight() - 90;

    const int midX = _vga.vbWidth() / 2;
    const int midY = _vga.vbHeight() / 2;

    if (_vga.vbBPP() == 16) {
        // ---- фон (градиент) ----
        for (int y = 0; y < _vga.vbHeight(); y++) {
            uint8_t gy = (uint32_t)y * 255 / (_vga.vbHeight() - 1);
            uint16_t gg = (uint16_t)(gy >> 2);      // 0..63

            uint16_t* row = _vga.lineBuf16[y];
            for (int x = 0; x < _vga.vbWidth(); x++) {
                uint8_t rx = (uint32_t)x * 255 / (_vga.vbWidth() - 1);
                uint16_t rr = (uint16_t)(rx >> 3);          // 0..31
                uint16_t bb = (uint16_t)((255 - rx) >> 3);  // 0..31
                row[x] = (rr << 11) | (gg << 5) | bb;
            }
        }

        // ---- тестовые полоски 256x90 ----
        for (int y = 0; y < 30; y++) {
            for (int x = 0; x < 256; x++) {
                _vga.lineBuf16[cy + y][cx + x]      = (uint16_t)(x >> 3) << 11; // R
                _vga.lineBuf16[cy + y + 30][cx + x] = (uint16_t)(x >> 2) << 5;  // G
                _vga.lineBuf16[cy + y + 60][cx + x] = (uint16_t)(x >> 3);       // B
            }
        }

        // ---- белая рамка + линии по центру ----
        const uint16_t WHITE = 0xFFFF;

        // рамка: верх/низ
        uint16_t* top = _vga.lineBuf16[0];
        uint16_t* bot = _vga.lineBuf16[(_vga.vbHeight() - 1)];
        for (int x = 0; x < _vga.vbWidth(); x++) {
            top[x] = WHITE;
            bot[x] = WHITE;
        }
        // рамка: лево/право
        for (int y = 0; y < _vga.vbHeight(); y++) {
            uint16_t* row = _vga.lineBuf16[y];
            row[0] = WHITE;
            row[_vga.vbWidth() - 1] = WHITE;
        }

        // вертикальная линия по центру
        for (int y = 0; y < _vga.vbHeight(); y++) {
            _vga.lineBuf16[y][midX] = WHITE;
        }
        // горизонтальная линия по центру
        uint16_t* mid = _vga.lineBuf16[midY];
        for (int x = 0; x < _vga.vbWidth(); x++) {
            mid[x] = WHITE;
        }

    } else {
        // ---- фон (градиент) ----
        for (int y = 0; y < _vga.vbHeight(); y++) {
            uint8_t gy = (uint32_t)y * 255 / (_vga.vbHeight() - 1);
            uint8_t gg = gy >> 5;   // 0..7

            uint8_t* row = _vga.lineBuf8[y];
            for (int x = 0; x < _vga.vbWidth(); x++) {
                uint8_t rx = (uint32_t)x * 255 / (_vga.vbWidth() - 1);
                uint8_t rr = rx >> 5;          // 0..7
                uint8_t bb = (255 - rx) >> 6;  // 0..3
                row[x] = (uint8_t)((rr << 5) | (gg << 2) | bb);
            }
        }

        // ---- тестовые полоски 256x90 ----
        for (int y = 0; y < 30; y++) {
            for (int x = 0; x < 256; x++) {
                _vga.lineBuf8[cy + y][cx + x]      = (uint8_t)(x >> 5) << 5; // R
                _vga.lineBuf8[cy + y + 30][cx + x] = (uint8_t)(x >> 5) << 2; // G
                _vga.lineBuf8[cy + y + 60][cx + x] = (uint8_t)(x >> 6);      // B
            }
        }

        // ---- белая рамка + линии по центру ----
        const uint8_t WHITE = 0xFF; // RGB332: R=7,G=7,B=3

        // рамка: верх/низ
        uint8_t* top = _vga.lineBuf8[0];
        uint8_t* bot = _vga.lineBuf8[(_vga.vbHeight() - 1)];
        for (int x = 0; x < _vga.vbWidth(); x++) {
            top[x] = WHITE;
            bot[x] = WHITE;
        }
        // рамка: лево/право
        for (int y = 0; y < _vga.vbHeight(); y++) {
            uint8_t* row = _vga.lineBuf8[y];
            row[0] = WHITE;
            row[_vga.vbWidth() - 1] = WHITE;
        }

        // вертикальная линия по центру
        for (int y = 0; y < _vga.vbHeight(); y++) {
            _vga.lineBuf8[y][midX] = WHITE;
        }
        // горизонтальная линия по центру
        uint8_t* mid = _vga.lineBuf8[midY];
        for (int x = 0; x < _vga.vbWidth(); x++) {
            mid[x] = WHITE;
        }
    }
}

void GFX::blur(){
    int width  = _vga.XX();
    int height = _vga.YY();
    uint16_t* line0 = _vga.lineBuf16[0];
    uint16_t* line1 = _vga.lineBuf16[1];

    if (_vga.BPP() == 16) {
        // ---- RGB565 ----
        while (height-- > 0){
            for (int x = 1; x < width; x++) {
                uint16_t c0 = *line0;
                uint16_t c1 = *line1--;
                uint16_t c2 = *line1++;
                uint16_t c3 = *line1++;

            // R: 5 бит
            uint16_t r = (uint16_t)(((c0 >> 11) + (c1 >> 11) + (c2 >> 11) + (c3 >> 11)) >> 2);

            // G: 6 бит (без вызова G16)
            uint16_t g = (uint16_t)((((c0 >> 5) & 0x3F) +
                                     ((c1 >> 5) & 0x3F) +
                                     ((c2 >> 5) & 0x3F) +
                                     ((c3 >> 5) & 0x3F)) >> 2);

            // B: 5 бит
            uint16_t b = (uint16_t)(((c0 & 0x1F) + (c1 & 0x1F) + (c2 & 0x1F) + (c3 & 0x1F)) >> 2);

                //uint16_t r = ((c0 >> 11) + (c1 >> 11) + (c2 >> 11) + (c3 >> 11)) >> 2;
                //uint16_t g = (G16(c0) + G16(c1) + G16(c2) + G16(c3)) >> 2;
                //uint16_t b = ((c0 & 0x1F) + (c1 & 0x1F) + (c2 & 0x1F) + (c3 & 0x1F)) >> 2;

                *line0++ = (r << 11) | (g << 5) | b;
            }

            line0 += 2;
            line1 += 2;
        }

    } else {
        // ---- RGB332 ----

    }
}
/*
void GFX::blur()
{
    if (_vga.BPP() != 16) return;

    const int w = _vga.Width();
    const int h = _vga.Height();

    for (int y = 0; y < h - 1; y++) {
        uint16_t* line0 = _vga.lineBuf16[y];
        uint16_t* line1 = _vga.lineBuf16[y + 1];

        uint16_t* p0 = line0 + 1;
        uint16_t* p1 = line1 + 1;

        for (int x = 1; x < w - 1; x++, p0++, p1++) {
            uint16_t c0 = *p0;
            uint16_t c1 = *p1;
            uint16_t c2 = *(p1 - 1);
            uint16_t c3 = *(p1 + 1);

            // R: 5 бит
            uint16_t r = (uint16_t)(((c0 >> 11) + (c1 >> 11) + (c2 >> 11) + (c3 >> 11)) >> 2);

            // G: 6 бит (без вызова G16)
            uint16_t g = (uint16_t)((((c0 >> 5) & 0x3F) +
                                     ((c1 >> 5) & 0x3F) +
                                     ((c2 >> 5) & 0x3F) +
                                     ((c3 >> 5) & 0x3F)) >> 2);

            // B: 5 бит
            uint16_t b = (uint16_t)(((c0 & 0x1F) + (c1 & 0x1F) + (c2 & 0x1F) + (c3 & 0x1F)) >> 2);

            *p0 = (uint16_t)((r << 11) | (g << 5) | b);
        }
    }
}

void GFX::blur()
{
    if (_vga.BPP() != 16) return;

    const int w = _vga.Width();
    const int h = _vga.Height();
    static uint8_t phase = 0;
    phase ^= 1;

    for (int y = 0; y < h - 1; y++) {
        uint16_t* line0 = _vga.lineBuf16[y];
        uint16_t* line1 = _vga.lineBuf16[y + 1];

        int start = 1 + ((y ^ phase) & 1);
        for (int x = start; x < w - 1; x += 2) {
            uint16_t c0 = line0[x];
            uint16_t c1 = line1[x];
            uint16_t c2 = line1[x - 1];
            uint16_t c3 = line1[x + 1];

            uint16_t r = (uint16_t)(((c0 >> 11) + (c1 >> 11) + (c2 >> 11) + (c3 >> 11)) >> 2);
            uint16_t g = (uint16_t)((((c0 >> 5) & 0x3F) + ((c1 >> 5) & 0x3F) +
                                     ((c2 >> 5) & 0x3F) + ((c3 >> 5) & 0x3F)) >> 2);
            uint16_t b = (uint16_t)(((c0 & 0x1F) + (c1 & 0x1F) + (c2 & 0x1F) + (c3 & 0x1F)) >> 2);

            line0[x] = (uint16_t)((r << 11) | (g << 5) | b);
        }
    }
}
/*
void GFX::blur(){
    int width  = _vga.XX();
    int height = _vga.YY();
    uint16_t* line0 = _vga.lineBuf16[0];
    uint16_t* line1 = _vga.lineBuf16[1];

    if (_vga.BPP() == 16) {
        // ---- RGB565 ----
        while (height-- > 0){
            for (int x = 1; x < width; x++) {
                uint16_t c0 = *line0;
                uint16_t c1 = *line1--;
                uint16_t c2 = *line1++;
                uint16_t c3 = *line1++;

                uint16_t r = ((c0 >> 11) + (c1 >> 11) + (c2 >> 11) + (c3 >> 11)) >> 2;
                uint16_t g = (G16(c0) + G16(c1) + G16(c2) + G16(c3)) >> 2;
                uint16_t b = ((c0 & 0x1F) + (c1 & 0x1F) + (c2 & 0x1F) + (c3 & 0x1F)) >> 2;

                *line0++ = (r << 11) | (g << 5) | b;
            }

            line0 += 2;
            line1 += 2;
        }

    } else {
        // ---- RGB332 ----

    }
}
   */ 