#pragma once

// In BT817/8, coprocessor adds a new feature “command list”, which enables user to construct a 
// series of coprocessor command or display list at RAM_G.  There are the following new commands 
// to facilitate:  
#define CMD_NEWLIST             0xFFFFFF68UL    // This command starts the compilation of a command list into RAM_G.
#define CMD_CALLLIST            0xFFFFFF67UL    // This command calls a command list.
#define CMD_RETURN              0xFFFFFF66UL    // This command ends a command list.
#define CMD_ENDLIST             0xFFFFFF69UL    // This command terminates the compilation of a command list into RAM_G.

// These commands begin and finish the display list: 
#define CMD_DLSTART             0xFFFFFF00UL    // start a new display list 
#define CMD_SWAP                0xFFFFFF01UL    //swap the current display list

// Commands to draw graphics objects: 
#define CMD_TEXT                0xFFFFFF0CUL    // draw a UTF-8 text string 
#define CMD_BUTTON              0xFFFFFF0DUL    // draw a button with a UTF-8 label.  
#define CMD_CLOCK               0xFFFFFF14UL    // draw an analog clock 
#define CMD_BGCOLOR             0xFFFFFF09UL    // set the background color 
#define CMD_FGCOLOR             0xFFFFFF0AUL    // set the foreground color 
#define CMD_GRADCOLOR           0xFFFFFF34UL    // set up the highlight color used in 3D effects for CMD_BUTTON and CMD_KEYS  
#define CMD_GAUGE               0xFFFFFF13UL    // draw a gauge 
#define CMD_GRADIENT            0xFFFFFF0BUL    // draw a smooth color gradient ???
#define CMD_KEYS                0xFFFFFF0EUL    // draw a row of keys 
#define CMD_PROGRESS            0xFFFFFF0FUL    // draw a progress bar 
#define CMD_SCROLLBAR           0xFFFFFF11UL    // draw a scroll bar 
#define CMD_SLIDER              0xFFFFFF10UL    // draw a slider 
#define CMD_DIAL                0xFFFFFF2DUL    // draw a rotary dial control 
#define CMD_TOGGLE              0xFFFFFF12UL    // draw a toggle switch with UTF-8 labels 
#define CMD_NUMBER              0xFFFFFF2EUL    // draw a decimal number 
#define CMD_SETBASE             0xFFFFFF38UL    // set the base for number output 
#define CMD_FILLWIDTH           0xFFFFFF58UL    // set the text fill width

// Commands to operate on RAM_G: 
#define CMD_MEMCRC              0xFFFFFF18UL    // compute a CRC-32 for RAM_G 
#define CMD_MEMZERO             0xFFFFFF1CUL    // write zero to RAM_G 
#define CMD_MEMSET              0xFFFFFF1BUL    // fill RAM_G with a byte value 
#define CMD_MEMWRITE            0xFFFFFF1AUL    // write bytes into RAM_G 
#define CMD_MEMCPY              0xFFFFFF1DUL    // copy a block of RAM_G 
#define CMD_APPEND              0xFFFFFF1EUL    // append more commands to display list 

//Commands for loading data into RAM_G: 
#define CMD_INFLATE             0xFFFFFF22UL    // decompress data into RAM_G 
#define CMD_INFLATE2            0xFFFFFF50UL    // decompress data into RAM_G with more options 
#define CMD_LOADIMAGE           0xFFFFFF24UL    // load a JPEG/PNG image into RAM_G 
#define CMD_MEDIAFIFO           0xFFFFFF39UL    // set up a streaming media FIFO in RAM_G 
#define CMD_VIDEOFRAME          0xFFFFFF41UL    // load video frame from RAM_G or flash memory. 

// Commands for setting the bitmap transform matrix: 
#define CMD_LOADIDENTITY        0xFFFFFF26UL    // set the current matrix to identity 
#define CMD_TRANSLATE           0xFFFFFF27UL    // apply a translation to the current matrix 
#define CMD_SCALE               0xFFFFFF28UL    // apply a scale to the current matrix 
#define CMD_ROTATE              0xFFFFFF29UL    // apply a rotation to the current matrix 
#define CMD_ROTATEAROUND        0xFFFFFF51UL    // apply a rotation and scale around the specified pixel 
#define CMD_SETMATRIX           0xFFFFFF2AUL    // write the current matrix as a bitmap transform 
#define CMD_GETMATRIX           0xFFFFFF33UL    // retrieves the current matrix coefficients

// Commands for flash operation: 
#define CMD_FLASHERASE          0xFFFFFF44UL    // Erase all of flash 
#define CMD_FLASHWRITE          0xFFFFFF45UL    // Write data to flash 
#define CMD_FLASHUPDATE         0xFFFFFF47UL    // write data to flash, erasing if necessary
#define CMD_FLASHDETACH         0xFFFFFF48UL    // detach from flash 
#define CMD_FLASHATTACH         0xFFFFFF49UL    // attach to flash 
#define CMD_FLASHFAST           0xFFFFFF4AUL    // enter full-speed mode 
#define CMD_FLASHSPIDESEL       0xFFFFFF4BUL    // SPI bus: deselect device 
#define CMD_FLASHTX             0x00UL          // SPI bus: write bytes ???
#define CMD_FLASHRX             0x00UL          // SPI bus: read bytes ???
#define CMD_CLEARCACHE          0xFFFFFF4FUL    // clear the flash cache 
#define CMD_FLASHSOURCE         0xFFFFFF4EUL    // specify the flash source address for the following coprocessor commands 
#define CMD_VIDEOSTARTF         0xFFFFFF5FUL    // initialize video frame decoder 
#define CMD_APPENDF             0xFFFFFF59UL    // Read data from flash to RAM_DL

// Commands for video playback: 
#define CMD_VIDEOSTART          0xFFFFFF40UL    // Initialize the video frame decoder 
#define CMD_VIDEOSTARTF         0xFFFFFF5FUL    // Initialize the video frame decoder for video data in flash  
#define CMD_VIDEOFRAME          0xFFFFFF41UL    // Load video frame data  
#define CMD_PLAYVIDEO           0xFFFFFF3AUL    // play back motion-JPEG encoded AVI video

// Commands for animation: 
#define CMD_ANIMFRAME           0xFFFFFF5AUL    // render one frame of an animation 
#define CMD_ANIMFRAMERAM        0xFFFFFF6DUL    // render one frame in RAM_G of an animation 
#define CMD_ANIMSTART           0xFFFFFF53UL    // start an animation 
#define CMD_ANIMSTOP            0xFFFFFF54UL    // stop animation 
#define CMD_ANIMXY              0xFFFFFF55UL    // set the (x,y) coordinates of an animation 
#define CMD_ANIMDRAW            0xFFFFFF56UL    // draw active animation 

// Other commands: 
#define CMD_COLDSTART           0xFFFFFF32UL    // set coprocessor engine state to default values 
#define CMD_INTERRUPT           0xFFFFFF02UL    // trigger interrupt INT_CMDFLAG 
#define CMD_REGREAD             0xFFFFFF19UL    // read a register value 
#define CMD_CALIBRATE           0xFFFFFF15UL    // execute the touch screen calibration routine 
#define CMD_ROMFONT             0xFFFFFF3FUL    // load a ROM font into bitmap handle 
#define CMD_SETROTATE           0xFFFFFF36UL    // Rotate the screen and set up transform matrix accordingly 
#define CMD_SETBITMAP           0xFFFFFF43UL    // Set up display list commands for specified bitmap 
#define CMD_SPINNER             0xFFFFFF16UL    // start an animated spinner 
#define CMD_STOP                0xFFFFFF17UL    // stop any spinner, screensaver or sketch 
#define CMD_SCREENSAVER         0xFFFFFF2FUL    // start an animated screensaver 
#define CMD_SKETCH              0xFFFFFF30UL    // start a continuous sketch update 
#define CMD_SNAPSHOT            0xFFFFFF1FUL    // take a snapshot of the current screen 
#define CMD_SNAPSHOT2           0xFFFFFF37UL    // take a snapshot of part of the current screen with more format option 
#define CMD_LOGO                0xFFFFFF31UL    // play device logo animation 

#define CMD_APILEVEL            0xFFFFFF63UL    //
#define CMD_GRAGIENT            0xFFFFFF0BUL    //
#define CMD_GRADIENTA           0xFFFFFF57UL    //
#define CMD_NOP                 0xFFFFFF53UL    // This command is a placeholder command and does nothing. 
#define CMD_GETPTR              0xFFFFFF23UL    // This command returns the first unallocated memory location.
#define CMD_GETPROPS            0xFFFFFF25UL    // This command returns the source address and size of the bitmap loaded by the previous CMD_LOADIMAGE. 
#define CMD_CALIBRATESUB        0xFFFFFF60UL    // This command is used to execute the touch screen calibration routine for a sub-window.
#define CMD_SETFONT             0xFFFFFF2BUL    // CMD_SETFONT is used to register one custom defined bitmap font into the coprocessor engine.
#define CMD_SETFONT2            0xFFFFFF3BUL    // This command is used to set up a custom font.
#define CMD_SETSCRATCH          0xFFFFFF3CUL    // This command is used to set the scratch bitmap for widget use.
#define CMD_RESETFONTS          0xFFFFFF52UL    // This command loads bitmap handles 16-31 with their default fonts.
#define CMD_TRACK               0xFFFFFF2CUL    // This command is used to track touches for a graphics object.
#define CMD_FLASHEWRTITE        0xFFFFFF45UL    // This command writes the following inline data to flash storage.
#define CMD_FLASHEPROGRAM       0xFFFFFF70UL    // This command writes the data to blank flash.
#define CMD_FLASHEREAD          0xFFFFFF46UL    // This command reads data from flash into main memory.
#define CMD_FLASHSPITX          0xFFFFFF4CUL    // This command transmits the following bytes over the flash SPI interface.
#define CMD_FLASHSPIRX          0xFFFFFF4DUL    // This command receives bytes from the flash SPI interface, and writes them to main memory.
#define CMD_ANIMSTARTRAM        0xFFFFFF6EUL    // This command is used to start an animation in RAM_G.
#define CMD_RUNANIM             0xFFFFFF6FUL    // This command is used to Play/run animations until complete.
#define CMD_SYNC                0xFFFFFF42UL    // This command waits for the end of the video scan out period, then it returns immediately.
#define CMD_BITMAP_TRANSFORM    0xFFFFFF21UL    // This command computes a bitmap transform and appends commands BITMAP_TRANSFORM_A – BITMAP_TRANSFORM_F to the display list. 
#define CMD_TESTCARD            0xFFFFFF61UL    // The testcard command loads a display list with a testcard graphic, and executes CMD_SWAP - swap the current display list to display it. 
#define CMD_WAIT                0xFFFFFF65UL    // This command waits for a specified number of microseconds. Delays of more than 1s (1000000 µs) are not supported.
#define CMD_FONTCACHE           0xFFFFFF6BUL    // This command enables the font cache, which loads all the bitmaps (glyph) used by a flash-based font into a RAM_G area.
#define CMD_FONTCACHEQUERY      0xFFFFFF6CUL    // This command queries the capacity and utilization of the font cache.
#define CMD_GETIMAGE            0xFFFFFF64UL    // This command returns all the attributes of the bitmap made by the previous CMD_LOADIMAGE, CMD_PLAYVIDEO, CMD_VIDEOSTART or CMD_VIDEOSTARTF.
#define CMD_HSF                 0xFFFFFF62UL    // void cmd_hsf( uint32_t w );
#define CMD_PCLKFREQ            0xFFFFFF6AUL    // This command sets REG_PCLK_FREQ to generate the closest possible frequency to the one requested.

