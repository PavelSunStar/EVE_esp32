#pragma once;

#include <stdint.h>

// https://brtchip.com/wp-content/uploads/2023/12/BT81X-Series-Programming-Guide.pdf
#define DEGREES(n) ((65536UL * (n)) / 360)

// ============================================================================
// Alpha function
// ============================================================================
#define NEVER               0
#define LESS                1
#define LEQUAL              2
#define GREATER             3
#define GEQUAL              4
#define EQUAL               5
#define NOTEQUAL            6
#define ALWAYS              7

// ============================================================================
// Graphics primitives
// ============================================================================
#define BITMAPS             1
#define POINTS              2
#define LINES               3
#define LINE_STRIP          4
#define EDGE_STRIP_R        5
#define EDGE_STRIP_L        6
#define EDGE_STRIP_A        7
#define EDGE_STRIP_B        8
#define RECTS               9

// ============================================================================
// Bitmap formats
// ============================================================================
#define ARGB1555                        0       // 16 
#define L1                              1       // 1 
#define L4                              2       // 4 
#define L8                              3       // 8 
#define RGB332                          4       // 8 
#define ARGB2                           5       // 8 
#define ARGB4                           6       // 16 
#define RGB565                          7       // 16 
#define TEXT8X8                         9       // 8 
#define TEXTVGA                         10      // 8 
#define BARGRAPH                        11      // 8 
#define PALETTED565                     14      // 8 
#define PALETTED4444                    15      // 8 
#define PALETTED8                       16      // 8 
#define L2                              17      // 2 
#define COMPRESSED_RGBA_ASTC_4x4_KHR    37808   // 8.00 
#define COMPRESSED_RGBA_ASTC_5x4_KHR    37809   // 6.40 
#define COMPRESSED_RGBA_ASTC_5x5_KHR    37810   // 5.12 
#define COMPRESSED_RGBA_ASTC_6x5_KHR    37811   // 4.27 
#define COMPRESSED_RGBA_ASTC_6x6_KHR    37812   // 3.56 
#define COMPRESSED_RGBA_ASTC_8x5_KHR    37813   // 3.20 
#define COMPRESSED_RGBA_ASTC_8x6_KHR    37814   // 2.67 
#define COMPRESSED_RGBA_ASTC_8x8_KHR    37815   // 2.00 
#define COMPRESSED_RGBA_ASTC_10x5_KHR   37816   // 2.56 
#define COMPRESSED_RGBA_ASTC_10x6_KHR   37817   // 2.13 
#define COMPRESSED_RGBA_ASTC_10x8_KHR   37818   // 1.60 
#define COMPRESSED_RGBA_ASTC_10x10_KHR  37819   // 1.28 
#define COMPRESSED_RGBA_ASTC_12x10_KHR  37820   // 1.07 
#define COMPRESSED_RGBA_ASTC_12x12_KHR  37821   // 0.89 

// ============================================================================
// Bitmap filtering mode
// ============================================================================
#define NEAREST     0
#define BILINEAR    1

// ============================================================================
// Bitmap wrap mode
// ============================================================================
#define BORDER      0
#define REPEAT      1

// ============================================================================
// Blend function
// ============================================================================
#define ZERO                0 
#define ONE                 1 
#define SRC_ALPHA           2 
#define DST_ALPHA           3 
#define ONE_MINUS_SRC_ALPHA 4 
#define ONE_MINUS_DST_ALPHA 5

// ============================================================================
// STENCIL_OP
// ============================================================================
#define ZERO                0
#define KEEP                1
#define REPLACE             2
#define INCR                3
#define DECR                4
#define INVERT              5

// ============================================================================
// VERTEX_FORMAT
// ============================================================================
#define PX_PREC_1           0   // 1 pixel precision
#define _1_2                1   // 1/2 pixel
#define _1_4                2   // 1/4 pixel
#define _1_8                3   // 1/8 pixel
#define _1_16               4   // 1/16 pixel (default)

#ifndef DL_CMD
#define DL_CMD(op) ((uint32_t)(op) << 24)
#endif

//=====================================================================================
constexpr inline uint32_t ALPHA_FUNC(uint8_t func, uint8_t ref){
    return DL_CMD(0x09) | (((uint32_t)func & 7UL) << 8) | (uint32_t)ref;
} // Specify the alpha test function

constexpr inline uint32_t BEGIN(uint8_t prim){ 
    return DL_CMD(0x1F) | ((uint32_t)prim & 0x0FUL);
} // Begin drawing a graphics primitive

constexpr inline uint32_t BITMAP_HANDLE(uint8_t handle){
    return DL_CMD(0x05) | ((uint32_t)handle & 0x1FUL);
} // Specify the bitmap handle

constexpr inline uint32_t BITMAP_LAYOUT(uint8_t format, uint16_t linestride, uint16_t height){
    return DL_CMD(0x07) | (((uint32_t)format & 0x1FUL) << 19) | (((uint32_t)linestride & 0x3FFUL) << 9) | ((uint32_t)height & 0x1FFUL);
} // Specify the source bitmap memory format and layout for the current handle. 

constexpr inline uint32_t BITMAP_LAYOUT_H(uint8_t linestride, uint8_t height){
    return DL_CMD(0x28) | (((uint32_t)linestride & 3UL) << 2) | ((uint32_t)height & 3UL);
} // Specify the 2 most significant bits of the source bitmap memory format and layout for the current handle.

constexpr inline uint32_t BITMAP_SIZE(bool filter, bool wrapx, bool wrapy, uint16_t width, uint16_t height){
    return DL_CMD(0x08) | ((uint32_t)filter << 20) | ((uint32_t)wrapx << 19) | ((uint32_t)wrapy << 18) | (((uint32_t)width & 0x1FFUL) << 9) | ((uint32_t)height & 0x1FFUL);
} // Specify the screen drawing of bitmaps for the current handle

constexpr inline uint32_t BITMAP_SIZE_H(uint8_t width, uint8_t height){
    return DL_CMD(0x29) | (((uint32_t)width & 3UL) << 2) | ((uint32_t)height & 3UL);
} // Specify the 2 most significant bits of bitmaps dimension for the current handle. 

constexpr inline uint32_t BITMAP_SOURCE(uint32_t addr){
    return DL_CMD(0x01) | ((uint32_t)addr & 0x3FFFFFUL);
} // Specify the source address of bitmap data in RAM_G

constexpr inline uint32_t BITMAP_TRANSFORM_A(uint32_t a){
    return DL_CMD(0x15) | ((uint32_t)a & 0x1FFFFUL);
} // Specify the 𝐴 coefficient of the bitmap transform matrix.

constexpr inline uint32_t BITMAP_TRANSFORM_B(uint32_t b){
    return DL_CMD(0x16) | (((uint32_t)b & 0x1FFFFUL));
} // Specify the 𝐵 coefficient of the bitmap transform matrix

constexpr inline uint32_t BITMAP_TRANSFORM_C(uint32_t c){
    return DL_CMD(0x17) | ((uint32_t)c & 0xFFFFFFUL);
} // Specify the 𝐶 coefficient of the bitmap transform matrix

constexpr inline uint32_t BITMAP_TRANSFORM_D(uint32_t d){
    return DL_CMD(0x18) | ((uint32_t)d & 0x1FFFFUL);
} // Specify the D coefficient of the bitmap transform matrix

constexpr inline uint32_t BITMAP_TRANSFORM_E(uint32_t e){
    return DL_CMD(0x19) | ((uint32_t)e & 0x1FFFFUL);
} // Specify the 𝐸 coefficient of the bitmap transform matrix.

constexpr inline uint32_t BITMAP_TRANSFORM_F(uint32_t f){
    return DL_CMD(0x1A) | ((uint32_t)f & 0xFFFFFFUL);
} // Specify the f coefficient of the bitmap transform matrix. 

constexpr inline uint32_t BLEND_FUNC(uint8_t src, uint8_t dst){
    return DL_CMD(0x0B) | (((uint32_t)src & 7UL) << 3) | ((uint32_t)dst & 7UL);
} // Specify pixel arithmetic

constexpr inline uint32_t CALL(uint16_t dest){
    return DL_CMD(0x1D) | ((uint32_t)dest);
} // Execute a sequence of commands at another location in the display list

constexpr inline uint32_t CELL(uint8_t cell){
    return DL_CMD(0x06) | ((uint32_t)cell & 0x7FUL);
} // Specify the bitmap cell number for the VERTEX2F command.

constexpr inline uint32_t CLEAR(bool c, bool s, bool t){
    return DL_CMD(0x26) | ((uint32_t)c << 2) | ((uint32_t)s << 1) | ((uint32_t)t);
} // Clear buffers to preset values

constexpr inline uint32_t CLEAR_COLOR_A(uint8_t alpha){
    return DL_CMD(0x0F) | ((uint32_t)alpha);
} // Specify clear value for the alpha channel

constexpr inline uint32_t CLEAR_COLOR_RGB(uint8_t r, uint8_t g, uint8_t b){
    return DL_CMD(0x02) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b);
} // Specify clear values for red, green and blue channels

constexpr inline uint32_t CLEAR_STENCIL(uint8_t s){
    return DL_CMD(0x11) | ((uint32_t)s);
} // Specify clear value for the stencil buffer

constexpr inline uint32_t CLEAR_TAG(uint8_t t){
    return DL_CMD(0x12) | ((uint32_t)t);
} // Specify clear value for the tag buffer

constexpr inline uint32_t COLOR_A(uint8_t alpha){
    return DL_CMD(0x10) | ((uint32_t)alpha);
} // Set the current color alpha

constexpr inline uint32_t COLOR_MASK(bool r, bool g, bool b, bool a){
    return DL_CMD(0x20) | ((uint32_t)r << 3) | ((uint32_t)g << 2) | ((uint32_t)b << 1) | ((uint32_t)a);
} // Enable or disable writing of color components

constexpr inline uint32_t COLOR_RGB(uint8_t r, uint8_t g, uint8_t b){
    return DL_CMD(0x04) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b);
} // Set the current color red, green and blue.

constexpr inline uint32_t  DL_DISPLAY(){
    return DL_CMD(0x00);
} // End the display list.  All the commands following this command will be ignored.

constexpr inline uint32_t END(){
    return DL_CMD(0x21);
} // End drawing a graphics primitive. 

constexpr inline uint32_t JUMP(uint16_t dest){
    return DL_CMD(0x1E) | ((uint32_t)dest);
} // Execute commands at another location in the display list

constexpr inline uint32_t LINE_WIDTH(uint16_t width){
    return DL_CMD(0x0E) | ((uint32_t)width & 0xFFFUL);
} // Specify the width of lines to be drawn with primitive LINES in 1/16-pixel precision.

constexpr inline uint32_t MACRO(bool m){
    return DL_CMD(0x25) | ((uint32_t)m);
} // Execute a single command from a macro register.

constexpr inline uint32_t DL_NOP(){
    return DL_CMD(0x2D);
} // No operation.

constexpr inline uint32_t PALETTE_SOURCE(uint32_t addr){
    return DL_CMD(0x2A) | (addr & 0x3FFFFFUL);
} // Specify the base address of the palette.

constexpr inline uint32_t POINT_SIZE(uint16_t size){
    return DL_CMD(0x0D) | ((uint32_t)size & 0x1FFFUL);
} // Specify the radius of points

constexpr inline uint32_t RESTORE_CONTEXT(){
    return DL_CMD(0x23);
} // Restore the current graphics context from the context stack.

constexpr inline uint32_t RETURN(){
    return DL_CMD(0x24);
} // Return from a previous CALL command.

constexpr inline uint32_t SAVE_CONTEXT(){
    return DL_CMD(0x22);
} // Push the current graphics context on the context stack

constexpr inline uint32_t SCISSOR_SIZE(uint16_t width, uint16_t height){
    return DL_CMD(0x1C) | (((uint32_t)width & 0xFFFUL) << 11) | (uint32_t)(height & 0xFFFUL);
} // Specify the size of the scissor clip rectangle.

constexpr inline uint32_t SCISSOR_XY(uint16_t x, uint16_t y){
    return DL_CMD(0x1B) | ((x & 0x7FFUL) << 11) | (y & 0x7FFUL);
} // Specify the top left corner of the scissor clip rectangle.

constexpr inline uint32_t STENCIL_FUNC(uint8_t func, uint8_t ref, uint8_t mask){
    return DL_CMD(0x0A) | (((uint32_t)func & 0x0FUL) << 16) | ((uint32_t)ref << 8) | ((uint32_t)mask);
} // Set function and reference value for stencil testing.

constexpr inline uint32_t STENCIL_MASK(uint8_t mask){
    return DL_CMD(0x13) | ((uint32_t)mask);
} // Control the writing of individual bits in the stencil planes

constexpr inline uint32_t STENCIL_OP(uint8_t sfail, uint8_t spass){
    return DL_CMD(0x0C) | (((uint32_t)sfail & 7UL) << 3) | ((uint32_t)spass & 7UL);
} // Set stencil test actions.

constexpr inline uint32_t TAG(uint8_t s){
    return DL_CMD(0x03) | ((uint32_t)s);
} // Attach the tag value for the following graphics objects drawn on the screen. The tag buffer default value is 255. 

constexpr inline uint32_t TAG_MASK(bool mask){
    return DL_CMD(0x14) | ((uint32_t)mask);
} // Control the writing of the tag buffer

constexpr inline uint32_t VERTEX2F(uint16_t x, uint16_t y){
    return ((uint32_t)1UL << 30) | (((uint32_t)x & 0x7FFFUL) << 15) | ((uint32_t)y & 0x7FFFUL);
} // Start the operation of graphics primitives at the specified screen coordinate, in the pixel precision defined by VERTEX_FORMAT.

constexpr inline uint32_t VERTEX2II(uint16_t x, uint16_t y, uint8_t handle, uint8_t cell){
    return ((uint32_t)2UL << 30) | (((uint32_t)x & 0x1FFUL) << 21) | (((uint32_t)y & 0x1FFUL) << 12) | (((uint32_t)handle & 0x1FUL) << 7) | ((uint32_t)cell & 0x7FUL);
} // Start the operation of graphics primitive at the specified coordinates in pixel precision.

constexpr inline uint32_t VERTEX_FORMAT(uint8_t frac){
    return DL_CMD(0x27) | ((uint32_t)frac & 7UL);
} // Set the precision of VERTEX2F coordinates.

constexpr inline uint32_t VERTEX_TRANSLATE_X(uint16_t x){
    return DL_CMD(0x2B) | ((uint32_t)x & 0x1FFFFUL);
} // Specify the vertex transformations X translation component. 

constexpr inline uint32_t VERTEX_TRANSLATE_Y(uint16_t y){
    return DL_CMD(0x2C) | ((uint32_t)y & 0x1FFFFUL);
} // Specify the vertex transformation’s Y translation component.

/*
constexpr inline uint32_t dlHead[] = {
    CLEAR_COLOR_RGB(0,0,0),
    CLEAR(true,true,true),
    DL_DISPLAY()
};
*/