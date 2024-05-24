/* MIT License
 *
 * Copyright (c) 2020 Tino Hernandez <vjmuzik1@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once
#include <Arduino.h>
#ifndef PXP_CTRL
    #error Device has no Pixel Pipeline support
#endif

//Start PXP
void PXP_init();

//Flip frame
void PXP_flip_vertically(bool flip);
void PXP_flip_horizontally(bool flip);
void PXP_flip_both(bool flip);

//Change block size, 0=8x8 1=16x16
void PXP_block_size(bool size);

//Enable repeat
void PXP_enable_repeat(bool repeat);

//Rotation direction, 0=0 1=90 2=180, 3=270
void PXP_rotate(uint8_t r);
//Rotation position, 0=output 1=input
void PXP_rotate_position(bool pos);

//Output buffer format
void PXP_output_format(uint8_t format = PXP_RGB565, uint8_t interlaced = 0, bool alpha_override = false, uint8_t alpha = 0);
//Output buffer
void PXP_output_buffer(void* buf, uint16_t pixelSizeInBytes, uint16_t width, uint16_t height);
//Output buffer for interlaced or YUV formats only
void PXP_output_buffer2(void* buf, uint16_t bytesPerPixel, uint16_t width, uint16_t height);
//Output buffer clip from coordinates 0,0 to x,y
void PXP_output_clip(uint16_t x, uint16_t y);

//Input buffer format
void PXP_input_format(uint8_t format = PXP_RGB565, uint8_t decimationScaleX = 0, uint8_t decimationScaleY = 0, bool wb_swap = 0);
//Input buffer
void PXP_input_buffer(void* buf, uint16_t pixelSizeInBytes, uint16_t width, uint16_t height);
//Input buffer for YUV formats only
void PXP_input_u_buffer(void* buf, uint16_t bytesPerPixel, uint16_t width, uint16_t height);
//Input buffer for YUV formats only
void PXP_input_v_buffer(void* buf, uint16_t bytesPerPixel, uint16_t width, uint16_t height);
//Input buffer position from coordinates x,y to x1,y1    x1,y1 can't be larger than output buffer
void PXP_input_position(uint16_t x, uint16_t y, uint16_t x1, uint16_t y1);
//Input buffer background color outside of position coordinates
void PXP_input_background_color(uint8_t r8, uint8_t g8, uint8_t b8);
void PXP_input_background_color(uint32_t rgb888);
//Input buffer color key range from low-high    set low to 0xFFFFFF and high to 0 to disable
void PXP_input_color_key_low(uint8_t r8, uint8_t g8, uint8_t b8);
void PXP_input_color_key_low(uint32_t rgb888);
void PXP_input_color_key_high(uint8_t r8, uint8_t g8, uint8_t b8);
void PXP_input_color_key_high(uint32_t rgb888);
//Input buffer rescale in 2 bit integer 12 bit fractional notation, offset for downscaling only in 12 bit fractional notation
//Scale should be the reciprocal of desired scale, 1/2 size is 0x2.000p0 double size is 0x0.800p0
//Maximum downscale value is 1/2 size, if smaller value is required use in combination with decimation scale in format
void PXP_input_scale(uint16_t x_scale = 0x1.000p0, uint16_t y_scale = 0x1.000p0, uint16_t x_offset = 0x.000p0, uint16_t y_offset = 0x.000p0);

//Overlay buffer format
void PXP_overlay_format(uint8_t format = PXP_RGB565, uint8_t alpha_ctrl = 0, bool colorKey = 0, uint8_t alpha = 0, uint8_t rop = 0, bool alpha_invert = 0);
//Overlay buffer
void PXP_overlay_buffer(void* buf, uint16_t pixelSizeInBytes, uint16_t width, uint16_t height);
//Overlay buffer position from coordinates x,y to x1,y1    x1,y1 can't be larger than output buffer
void PXP_overlay_position(uint16_t x, uint16_t y, uint16_t x1, uint16_t y1);
//Overlay buffer color key range from low-high    only active when turned on in format
void PXP_overlay_color_key_low(uint8_t r8, uint8_t g8, uint8_t b8);
void PXP_overlay_color_key_low(uint32_t rgb888);
void PXP_overlay_color_key_high(uint8_t r8, uint8_t g8, uint8_t b8);
void PXP_overlay_color_key_high(uint32_t rgb888);

void PXP_GetScalerParam(uint16_t inputDimension, uint16_t outputDimension, uint8_t *dec, uint32_t *scale);
void PXP_setScaling(uint16_t inputWidth, uint16_t inputHeight, uint16_t outputWidth, uint16_t outputHeight);
void PXP_SetCsc1Mode(uint8_t mode);
void PXP_set_csc_y8_to_rgb();

void PXP_flip(bool flip);
void PXP_scaling(void *buf_out, uint8_t bbp_out, float downScaleFact,
                  uint16_t width, uint16_t height, uint8_t rotation,
                  uint16_t* outputWidth, uint16_t* outputHeight);
void PXP_ps_output(uint16_t disp_width, uint16_t disp_height, uint16_t image_width, uint16_t image_height, 
                   void* buf_in, uint8_t format_in, uint8_t bpp_in, uint8_t byte_swap_in, 
                   void* buf_out, uint8_t format_out, uint8_t bpp_out, uint8_t byte_swap_out, 
                   uint8_t rotation, bool flip, float scaling, 
                   uint16_t* scr_width, uint16_t* scr_height);


//Call process to run the PXP with the current settings
void PXP_process();

//Call to make sure the last process is finished before using the buffer with a display
void PXP_finish();
