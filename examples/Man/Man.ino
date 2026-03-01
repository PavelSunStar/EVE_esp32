#include "EVE_esp32.h"
#include "GFX.h"
#include "Sprite.h"

#include "0.h"
 
EVE_esp32 eve;
GFX gfx(eve);
Sprite sp(eve);

EVE_Image img[8];

#define MAX 250
struct Man{
  float x;
  int y;
  float dx;
  int index;
};
Man m[MAX];
int pos[8];

void initMan(){
  for (int i = 0; i < 8; i++){
    int p = 38 * 38 * 2 * i;
    sp.sendToEve(p, i);
    pos[i] = p;
  }

  for (int i = 0; i < MAX; i++){
    m[i].x = random(640);
    m[i].y = random(100, 480 - 38);
    m[i].index = random(8);

    float t = (float)m[i].y / (eve.Height() - 38);   // 0..1
    float base = 0.3f + 4.5f * t * t;                // сверху медленно, снизу быстро (квадрат)
    float jitter = (float)random(80, 140) / 100.0f;  // 0.8..1.4
    m[i].dx = base * jitter;   
  }
}

int a, c;
void makeList(){
  eve.dlNewList();
  eve.dlStart();

  eve.dl(BITMAP_HANDLE(0));
  eve.dl(BITMAP_LAYOUT(ARGB1555, 38 * 2, 38));
  eve.dl(BITMAP_LAYOUT_H(0,0)); 

  // размер вывода
  eve.dl(BITMAP_SIZE(NEAREST, BORDER, BORDER, 38, 38));
  eve.dl(BITMAP_SIZE_H(0,0));

  eve.dl(BEGIN(BITMAPS));
  for (int i = 0; i < MAX; i++){
    int b = (m[i].y * 255) / eve.Height();
    b = (b + 25 > 255 ? 255 : b + 25);
    eve.dl(COLOR_RGB(b, b, b));
    eve.dl(BITMAP_SOURCE(pos[(c + m[i].index) % 8]));
    eve.dl(VERTEX2F((int)m[i].x << 4, m[i].y << 4));  // координаты в 1/16 пикселя

    m[i].x += m[i].dx;
    if (m[i].x > 640){
      m[i].x = -38; // чтобы появлялся плавно
      //m[i].y = random(100, 480 - 38);
      float t = (float)m[i].y / (eve.Height() - 38);
      float base = 0.3f + 4.5f * t * t;
      float jitter = (float)random(80, 140) / 100.0f;
      m[i].dx = base * jitter;
    }  
  }
  eve.dl(END());
  eve.dlEnd();

  a++;
  if (a % 5 == 0) c++;
}

void setup() {
Serial.begin(115200);
unsigned long t0 = millis();
while (!Serial && millis() - t0 < 2000) { delay(10); }  // подождать монитор/порт
Serial.println("setup start");
  eve.init();

  eve.createVideoBuffer();
  gfx.testRGBPanel();

  sp.loadImages(_0);
  initMan();   
} 
 

void loop() {
  updateFPS();
  Serial.println(fps);
  makeList();
  eve.swap();
  eve.sendScr();
  //delay(100);
}

/*
  //eve.createVideoBuffer();
  //gfx.testRGBPanel();
  //sp.putImage(100, 100, 5);  
  //eve.sendScr();
  //sp.putImage(100, 100, 5);//random(sp.MaxImages()));
  //eve.sendScr();
  // включаем альфа-блендинг
  //eve.dl(BLEND_FUNC(SRC_ALPHA, ONE_MINUS_SRC_ALPHA));
*/
