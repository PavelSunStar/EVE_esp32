#pragma once;

// https://brtchip.com/wp-content/uploads/2025/02/DS_BT815_6.pdf
#define CHIP_RESET  0x7C
#define CHIPID_ADDR 0x0C0000UL // to 0x0C0003 (4 bytes)
    //- 0C0000h: 08h
    //- 0C0001h: 15h (BT815), 16h(BT816)
    //- 0C0002h: 01h
    //- 0C0003h: 00h

// All memory and registers in the BT815/6 core are memory mapped in 22-bit address space with a 2-bit
// SPI command prefix. Prefix 0'b00 for read and 0'b10 for write to the address space, 0'b01 is reserved for
// Host Commands and 0'b11 undefined. The following are the memory space definition.
#define RAM_G       0x000000UL // to 0x0FFFFF (1024 byte)
#define ROM         0x200000UL // to 0x2FFFFF (1024 byte)
#define RAM_DL      0x300000UL // to 0x301FFF (8192 byte)
#define RAM_REG     0x302000UL // to 0x302FFF (4096 byte)
#define RAM_CMD     0x308000UL // to 0x308FFF (4096 byte)
#define FLASH       0x800000UL // to 0x107FFFFF (256 MB)

#define DLSWAP_FRAME                0x00000002UL
#define DLSWAP_LINE                 0x00000001UL
#define DLSWAP_DONE                 0x00000000UL

// Registers (page 39)                      (bits) (r/w) (restet value)
#define REG_ID                      0x302000UL
#define REG_FRAMES                  0x302004UL  //  32  r/o 0           Frame counter, since reset
#define REG_CLOCK                   0x302008UL  //  32  r/o 0           Clock cycles, since reset
#define REG_FREQUENCY               0x30200CUL  //  28  r/w 60000000    Main clock frequency (Hz)
#define REG_RENDERMODE              0x302010UL  //  1   r/w 0           Rendering mode: 0 = normal, 1 = singleline
#define REG_SNAPY                   0x302014UL  //  11  r/w 0           Scanline select for RENDERMODE 1
#define REG_SNAPSHOT                0x302018UL  //  1   r/w -           Trigger for RENDERMODE 1
#define REG_SNAPFORMAT              0x30201CUL  //  6   r/w 20h         Pixel format for scanline readout
#define REG_CPURESET                0x302020UL  //  3   r/w 2           Graphics, audio and touch engines reset control. Bit2: audio, bit1: touch, bit0: graphics 
#define REG_TAP_CRC                 0x302024UL  //  32  r/o -           Live video tap crc. Frame CRC is computed every DL SWAP.
#define REG_TAP_MASK                0x302028UL  //  32  r/w FFFFFFFFh   Live video tap mask
#define REG_HCYCLE                  0x30202CUL  //  12  r/w 224h        Horizontal total cycle count
#define REG_HOFFSET                 0x302030UL  //  12  r/w 02Bh        Horizontal display start offset
#define REG_HSIZE                   0x302034UL  //  12  r/w 1E0h        Horizontal display pixel count
#define REG_HSYNC0                  0x302038UL  //  12  r/w 000h        Horizontal sync fall offset
#define REG_HSYNC1                  0x30203CUL  //  12  r/w 029h        Horizontal sync rise offset
#define REG_VCYCLE                  0x302040UL  //  12  r/w 124h        Vertical total cycle count
#define REG_VOFFSET                 0x302044UL  //  12  r/w 00Ch        Vertical display start offset
#define REG_VSIZE                   0x302048UL  //  12  r/w 110h        Vertical display line count
#define REG_VSYNC0                  0x30204CUL  //  10  r/w 000h        Vertical sync fall offset
#define REG_VSYNC1                  0x302050UL  //  10  r/w 00Ah        Vertical sync rise offset
#define REG_DLSWAP                  0x302054UL  //  2   r/w 0           Display list swap control
#define REG_ROTATE                  0x302058UL  //  3   r/w 0           Screen rotation control. Allow normal/mirrored/inverted for landscape or portrait orientation.
#define REG_OUTBITS                 0x30205CUL  //  9   r/w 0           Output bit resolution, 3 register bits each for R/G/B. 0 indicates 8 bits, 1-7 indicates 1-7 bits respectively.
#define REG_DITHER                  0x302060UL  //  1   r/w 1           Output dither enable
#define REG_SWIZZLE                 0x302064UL  //  4   r/w 0           Output RGB signal swizzle
#define REG_CSPREAD                 0x302068UL  //  1   r/w 1           Output clock spreading enable
#define REG_PCLK_POL                0x30206CUL  //  1   r/w 0           PCLK polarity: 0 = output on PCLK rising edge, 1 = output on PCLK falling edge
#define REG_PCLK                    0x302070UL  //  8   r/w 0           PCLK frequency divider, 0 = disable
#define REG_TAG_X                   0x302074UL  //  11  r/w 0           Tag query X coordinate
#define REG_TAG_Y                   0x302078UL  //  11  r/w 0           Tag query Y coordinate
#define REG_TAG                     0x30207CUL  //  8   r/o 0           Tag query result
#define REG_VOL_PB                  0x302080UL  //  8   r/w FFh         Volume for playback
#define REG_VOL_SOUND               0x302084UL  //  8   r/w FFh         Volume for synthesizer sound
#define REG_SOUND                   0x302088UL  //  16  r/w 0           Sound effect select
#define REG_PLAY                    0x30208CUL  //  1   r/w 0h          Start effect playback
#define REG_GPIO_DIR                0x302090UL  //  8   r/w 80h         Legacy GPIO pin direction, 0 = input , 1 = output
#define REG_GPIO                    0x302094UL  //  8   r/w 00h         Legacy GPIO read/write
#define REG_GPIOX_DIR               0x302098UL  //  16  r/w 8000h       Extended GPIO pin direction, 0 = input , 1 = output
#define REG_GPIOX                   0x30209CUL  //  16  r/w 0080h       Extended GPIO read/write
#define REG_INT_FLAGS               0x3020A8UL  //  8   r/o 00h         Interrupt flags, clear by read
#define REG_INT_EN                  0x3020ACUL  //  1   r/w 0           Global interrupt enable, 1=enable
#define REG_INT_MASK                0x3020B0UL  //  8   r/w FFh         Individual interrupt enable, 1=enable
#define REG_PLAYBACK_START          0x3020B4UL  //  20  r/w 0           Audio playback RAM start address
#define REG_PLAYBACK_LENGTH         0x3020B8UL  //  20  r/w 0           Audio playback sample length (bytes)
#define REG_PLAYBACK_READPTR        0x3020BCUL  //  20  r/o -           Audio playback current read pointer
#define REG_PLAYBACK_FREQ           0x3020C0UL  //  16  r/w 8000        Audio playback sampling frequency (Hz)
#define REG_PLAYBACK_FORMAT         0x3020C4UL  //  2   r/w 0           Audio playback format
#define REG_PLAYBACK_LOOP           0x3020C8UL  //  1   r/w 0           Audio playback loop enable
#define REG_PLAYBACK_PLAY           0x3020CCUL  //  1   r/w 0           Start audio playback
#define REG_PWM_HZ                  0x3020D0UL  //  14  r/w 250         BACKLIGHT PWM output frequency (Hz)
#define REG_PWM_DUTY                0x3020D4UL  //  8   r/w 128         BACKLIGHT PWM output duty cycle 0=0%, 128=100%
#define REG_MACRO_0                 0x3020D8UL  //  32  r/w 0           Display list macro command 0
#define REG_MACRO_1                 0x3020DCUL  //  32  r/w 0           Display list macro command 1
#define REG_CMD_READ                0x3020F8UL  //  12  r/w 0           Command buffer read pointer
#define REG_CMD_WRITE               0x3020FCUL  //  12  r/o 0           Command buffer write pointer
#define REG_CMD_DL                  0x302100UL  //  13  r/w 0           Command display list offset
#define REG_TOUCH_MODE              0x302104UL  //  2   r/w 3           Touch-screen sampling mode
#define REG_CTOUCH_EXTENDED         0x302108UL
#define REG_TOUCH_ADC_MODE          0x302108UL  //  1   r/w 1           Set Touch ADC mode Set capacitive touch operation mode: 0: extended mode (multi-touch) 1: FT800 compatibility mode (single touch).
#define REG_EHOST_TOUCH_X           0x30210CUL
#define REG_TOUCH_CHARGE            0x30210CUL  //  16  r/w 9000        Touch charge time, units of 6 clocks Touch host mode: touch x value updated by host
#define REG_TOUCH_SETTLE            0x302110UL  //  4   r/w 3           Touch settle time, units of 6 clocks
#define REG_EHOST_TOUCH_ID          0x302114UL
#define REG_TOUCH_OVERSAMPLE        0x302114UL  //  4   r/w 7           Touch oversample factor Touch host mode: touch ID, 0-4
#define REG_EHOST_TOUCH_Y           0x302118UL
#define REG_TOUCH_RZTHRESH          0x302118UL  //  16  r/w FFFFh       Touch resistance threshold Touch host mode: touch x value updated by host
#define REG_CTOUCH_TOUCH1_XY        0x30211CUL
#define REG_TOUCH_RAW_XY            0x30211CUL  //  32  r/o -           Compatibility mode: touch-screen raw (xMSB16; y-LSB16) Extended mode: touch-screen screen data for touch 1 (x-MSB16; y-LSB16)
#define REG_CTOUCH_TOUCH4_Y         0x302120UL
#define REG_TOUCH_RZ                0x302120UL  //  16  r/o -           Compatibility mode: touch-screen resistance Extended mode: touch-screen screen Y data for touch 4
#define REG_TOUCH_SCREEN_XY         0x302124UL  //  32  r/o -           Compatibility mode: touch-screen screen (x-MSB16; y-LSB16) REG_CTOUCH_TOUCH0_XY Extended mode: touch-screen screen data for touch 0 (x-MSB16; y-LSB16)
#define REG_TOUCH_TAG_XY            0x302128UL  //  32  r/o -           Touch-screen screen (x-MSB16; yLSB16) used for tag 0 lookup
#define REG_TOUCH_TAG               0x30212CUL  //  8   r/o -           Touch-screen tag result 0 
#define REG_TOUCH_TAG1_XY           0x302130UL  //  32  r/o -           Touch-screen screen (x-MSB16; yLSB16) used for tag 1 lookup
#define REG_TOUCH_TAG1              0x302134UL  //  8   r/o -           Touch-screen tag result 1
#define REG_TOUCH_TAG2_XY           0x302138UL  //  32  r/o -           Touch-screen screen (x-MSB16; yLSB16) used for tag 2 lookup
#define REG_TOUCH_TAG2              0x30213CUL  //  8   r/o -           Touch-screen tag result 2
#define REG_TOUCH_TAG3_XY           0x302140UL  //  32  r/o -           Touch-screen screen (x-MSB16; yLSB16) used for tag 3 lookup
#define REG_TOUCH_TAG3              0x302144UL  //  8   r/o -           Touch-screen tag result 3
#define REG_TOUCH_TAG4_XY           0x302148UL  //  32  r/o -           Touch-screen screen (x-MSB16; yLSB16) used for tag 4 lookup
#define REG_TOUCH_TAG4              0x30214CUL  //  8   r/o -           Touch-screen tag result 4
#define REG_TOUCH_TRANSFORM_A       0x302150UL  //  32  r/w 00010000h   Touch-screen transform coefficient(s15.16)
#define REG_TOUCH_TRANSFORM_B       0x302154UL  //  32  r/w 00000000h   Touch-screen transform coefficient(s15.16)
#define REG_TOUCH_TRANSFORM_C       0x302158UL  //  32  r/w 00000000h   Touch-screen transform coefficient(s15.16)
#define REG_TOUCH_TRANSFORM_D       0x30215CUL  //  32  r/w 00000000h   Touch-screen transform coefficient(s15.16)
#define REG_TOUCH_TRANSFORM_E       0x302160UL  //  32  r/w 00010000h   Touch-screen transform coefficient(s15.16)
#define REG_TOUCH_TRANSFORM_F       0x302164UL  //  32  r/w 00000000h   Touch-screen transform coefficient(s15.16)
#define REG_TOUCH_CONFIG            0x302168UL  //  16  r/w 8381h(BT816) 
                                                //          0381h(BT815) 
                                                //              Touch configuration. 
                                                //              RTP/CTP select 
                                                //              RTP: short-circuit, sample clocks 
                                                //              CTP: I2C address, CTPM type, low-power mode, touch host mode
#define REG_CTOUCH_TOUCH4_X         0x30216CUL  //  16  r/o - Extended mode: touch-screen screen X data for touch 4
#define REG_EHOST_TOUCH_ACK         0x302170UL  //  4   r/w 0 Touch host mode: acknowledgement
#define REG_BIST_EN                 0x302174UL  //  1   r/w 0 BIST memory mapping enable
#define REG_TRIM                    0x302180UL  //  5   r/w 0 Internal relaxation clock trimming
#define REG_ANA_COMP                0x302184UL  //  8   r/w 0 Analogue control register
#define REG_SPI_WIDTH               0x302188UL  //  3   r/w 0 QSPI bus width setting Bit [2]: extra dummy cycle on read Bit [1:0]: bus width (0=1-bit, 1=2-bit, 2=4-bit)
#define REG_CTOUCH_TOUCH2_XY        0x30218CUL
#define REG_TOUCH_DIRECT_XY         0x30218CUL  //  32  r/o - Compatibility mode: Touch screen direct (x-MSB16; y-LSB16) conversions Extended mode: touch-screen screen data for touch 2 (x-MSB16; y-LSB16)
#define REG_CTOUCH_TOUCH3_XY        0x302190UL
#define REG_TOUCH_DIRECT_Z1Z2       0x302190UL  //  32  r/o - Compatibility mode: Touch screen direct (z1-MSB16; z2-LSB16) conversions Extended mode: touch-screen screen data for touch 3 (x-MSB16; y-LSB16)
#define REG_DATESTAMP               0x302564UL  //  128 r/o - Stamp date code
#define REG_CMDB_SPACE              0x302574UL  //  12  r/w FFCh Command DL (bulk) space available
#define REG_CMDB_WRITE              0x302578UL  //  32  w/o 0 Command DL (bulk) write
#define REG_ADAPTIVE_FRAMERATE      0x30257CUL  //  1   r/w 1 Reduce frame rate during complex drawing
#define REG_PLAYBACK_PAUSE          0x3025ECUL  //  1   r/w 0 Audio playback pause
#define REG_FLASH_STATUS            0x3025F0UL  //  2   r/w 0 Flash status

#define FREQUENCY(value)            ((value) & 0x0FFFFFFFUL) // 28   0..4294967295
#define RENDERMODE(value)           ((value) & 1UL)          // 1    0..1
#define SNAPY(value)                ((value) & 0x7FFUL)      // 11   0..2047
#define SNAPSHOT(value)             ((value) & 1UL)          // 1    0..1
#define SNAPFORMAT(value)           ((value) & 0x3FUL)       // 6    0..63
#define CPURESET(value)             ((value) & 7UL)          // 3    0..7
#define TAP_MASK(value)             ((value) & 0xFFFFFFFFUL) // 32   0..4294967295
#define HCYCLE(value)               ((value) & 0xFFFUL)      // 12   0..4095
#define HOFFSET(value)              ((value) & 0xFFFUL)      // 12   0..4095
#define HSIZE(value)                ((value) & 0xFFFUL)      // 12   0..4095
#define HSYNC0(value)               ((value) & 0xFFFUL)      // 12   0..4095
#define HSYNC1(value)               ((value) & 0xFFFUL)      // 12   0..4095
#define VCYCLE(value)               ((value) & 0xFFFUL)      // 12   0..4095
#define VOFFSET(value)              ((value) & 0xFFFUL)      // 12   0..4095
#define VSIZE(value)                ((value) & 0xFFFUL)      // 12   0..4095
#define VSYNC0(value)               ((value) & 0x3FFUL)      // 10   0..1023
#define VSYNC1(value)               ((value) & 0x3FFUL)      // 10   0..1023
#define DLSWAP(value)               ((value) & 3UL)          // 2    0..3
#define ROTATE(value)               ((value) & 7UL)          // 3    0..7
//#define OUTBITS(value)              ((value) & 0x1FFUL)      // 9    0..511
#define OUTBITS(r,g,b) ((((uint32_t)(r) & 7U) << 6) | (((uint32_t)(g) & 7U) << 3) | (((uint32_t)(b) & 7U) << 0) )
#define DITHER(value)               ((value) & 1UL)          // 1    0..1
#define SWIZZLE(value)              ((value) & 0xFUL)        // 4    0..15
#define CSPREAD(value)              ((value) & 1UL)          // 1    0..1
#define PCLK_POL(value)             ((value) & 1UL)          // 1    0..1
#define PCLK(value)                 ((value) & 0xFFUL)       // 8    0..255
#define TAG_X(value)                ((value) & 0x7FFUL)      // 11   0..2047
#define TAG_Y(value)                ((value) & 0x7FFUL)      // 11   0..2047
#define VOL_PB(value)               ((value) & 0xFFUL)       // 8    0..255
#define VOL_SOUND(value)            ((value) & 0xFFUL)       // 8    0..255
#define SOUND(value)                ((value) & 0xFFFFUL)     // 16   0..65535
#define PLAY(value)                 ((value) & 1UL)          // 1    0..1
#define GPIO_DIR(value)             ((value) & 0xFFUL)       // 8    0..255
#define GPIO(value)                 ((value) & 0xFFUL)       // 8    0..255
#define GPIOX_DIR(value)            ((value) & 0xFFFFUL)     // 16   0..65535
#define GPIOX(value)                ((value) & 0xFFFFUL)     // 16   0..65535
#define INT_EN(value)               ((value) & 1UL)          // 1    0..1
#define INT_MASK(value)             ((value) & 0xFFUL)       // 8    0..255
#define PLAYBACK_START(value)       ((value) & 0xFFFFFUL)    // 20   0..1048575
#define PLAYBACK_LENGTH(value)      ((value) & 0xFFFFFUL)    // 20   0..1048575
#define PLAYBACK_FREQ(value)        ((value) & 0xFFFFUL)     // 16   0..65535
#define PLAYBACK_FORMAT(value)      ((value) & 3UL)          // 2    0..3
#define PLAYBACK_LOOP(value)        ((value) & 1UL)          // 1    0..1
#define PLAYBACK_PLAY(value)        ((value) & 1UL)          // 1    0..1
#define PWM_HZ(value)               ((value) & 0x3FFFUL)     // 14   0..16383
#define PWM_DUTY(value)             ((value) & 0xFFUL)       // 8    0..255
#define MACRO_0(value)              ((value) & 0xFFFFFFFFUL) // 32   0..4294967295
#define MACRO_1(value)              ((value) & 0xFFFFFFFFUL) // 32   0..4294967295
#define CMD_READ(value)             ((value) & 0xFFFUL)      // 12   0..4095
#define CMD_DL(value)               ((value) & 0x1FFFUL)     // 13   0..8191
#define TOUCH_MODE(value)           ((value) & 3UL)          // 2    0..3
#define CTOUCH_EXTENDED(value)      ((value) & 1UL)          // 1    0..1
#define TOUCH_ADC_MODE(value)       ((value) & 1UL)          // 1    0..1
#define EHOST_TOUCH_X(value)        ((value) & 0xFFFFUL)     // 16   0..65535
#define TOUCH_CHARGE(value)         ((value) & 0xFFFFUL)     // 16   0..65535
#define TOUCH_SETTLE(value)         ((value) & 0xFUL)        // 4    0..15
#define EHOST_TOUCH_ID(value)       ((value) & 0xFUL)        // 4    0..15
#define TOUCH_OVERSAMPLE(value)     ((value) & 0xFUL)        // 4    0..15
#define EHOST_TOUCH_Y(value)        ((value) & 0xFFFFUL)     // 16   0..65535
#define TOUCH_RZTHRESH(value)       ((value) & 0xFFFFUL)     // 16   0..65535
#define TOUCH_TRANSFORM_A(value)    ((value) & 0xFFFFFFFFUL) // 32   0..4294967295
#define TOUCH_TRANSFORM_B(value)    ((value) & 0xFFFFFFFFUL) // 32   0..4294967295
#define TOUCH_TRANSFORM_C(value)    ((value) & 0xFFFFFFFFUL) // 32   0..4294967295
#define TOUCH_TRANSFORM_D(value)    ((value) & 0xFFFFFFFFUL) // 32   0..4294967295
#define TOUCH_TRANSFORM_E(value)    ((value) & 0xFFFFFFFFUL) // 32   0..4294967295
#define TOUCH_TRANSFORM_F(value)    ((value) & 0xFFFFFFFFUL) // 32   0..4294967295
#define TOUCH_CONFIG(value)         ((value) & 0xFFFFUL)     // 16   0..65535
#define EHOST_TOUCH_ACK(value)      ((value) & 0xFUL)        // 4    0..15
#define BIST_EN(value)              ((value) & 1UL)          // 1    0..1
#define TRIM(value)                 ((value) & 0x1FUL)       // 5    0..31
#define ANA_COMP(value)             ((value) & 0xFFUL)       // 8    0..255
#define SPI_WIDTH(value)            ((value) & 7UL)          // 3    0..7
#define CMDB_SPACE(value)           ((value) & 0xFFFUL)      // 12   0..4095
#define ADAPTIVE_FRAMERATE(value)   ((value) & 1UL)          // 1    0..1
#define PLAYBACK_PAUSE(value)       ((value) & 1UL)          // 1    0..1
#define FLASH_STATUS(value)         ((value) & 3UL)          // 2    0..3

