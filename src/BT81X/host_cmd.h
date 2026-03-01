#pragma once

#define ACTIVE          0x00
#define STANDBY         0x41
#define SLEEP           0x42
#define PWRDOWN         0x50 // 0x43 
#define CLKEXT	        0x44
#define CLKINT	        0x48
#define CLKSEL	        0x61 // 0x62
#define CORERST         0x66 // иногда встречается, редко нужен
#define RST_PULSE	    0x68
#define PINDRIVE        0x70 
#define PIN_PD_STATE    0x71 
