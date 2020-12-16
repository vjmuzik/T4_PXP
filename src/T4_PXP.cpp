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

#include "T4_PXP.h"
typedef struct
{
    volatile uint32_t CTRL;
    volatile uint32_t STAT;
    volatile uint32_t OUT_CTRL;
    volatile void*    OUT_BUF;
    volatile void*    OUT_BUF2;
    volatile uint32_t OUT_PITCH;
    volatile uint32_t OUT_LRC;
    volatile uint32_t OUT_PS_ULC;
    volatile uint32_t OUT_PS_LRC;
    volatile uint32_t OUT_AS_ULC;
    volatile uint32_t OUT_AS_LRC;
    volatile uint32_t PS_CTRL;
    volatile void*    PS_BUF;
    volatile void*    PS_UBUF;
    volatile void*    PS_VBUF;
    volatile uint32_t PS_PITCH;
    volatile uint32_t PS_BACKGROUND;
    volatile uint32_t PS_SCALE;
    volatile uint32_t PS_OFFSET;
    volatile uint32_t PS_CLRKEYLOW;
    volatile uint32_t PS_CLRKEYHIGH;
    volatile uint32_t AS_CTRL;
    volatile void*    AS_BUF;
    volatile uint32_t AS_PITCH;
    volatile uint32_t AS_CLRKEYLOW;
    volatile uint32_t AS_CLRKEYHIGH;
    volatile uint32_t CSC1_COEF0;
    volatile uint32_t CSC1_COEF1;
    volatile uint32_t CSC1_COEF2;
    volatile uint32_t POWER;
    volatile uint32_t NEXT;
    volatile uint32_t PORTER_DUFF_CTRL;
} IMXRT_NEXT_PXP_t;

volatile IMXRT_NEXT_PXP_t next_pxp __attribute__ ((aligned(32))) = {0};
volatile bool PXP_done = true;
//These are only used to flush cached buffers
uint16_t PS_BUF_width, PS_BUF_height, PS_BUF_bytesPerPixel,
         PS_UBUF_width, PS_UBUF_height, PS_UBUF_bytesPerPixel,
         PS_VBUF_width, PS_VBUF_height, PS_VBUF_bytesPerPixel,
         AS_BUF_width, AS_BUF_height, AS_BUF_bytesPerPixel,
         OUT_BUF_width, OUT_BUF_height, OUT_BUF_bytesPerPixel,
         OUT_BUF2_width, OUT_BUF2_height, OUT_BUF2_bytesPerPixel;
         


void PXP_isr(){
  if((PXP_STAT & PXP_STAT_LUT_DMA_LOAD_DONE_IRQ) != 0){
    PXP_STAT_CLR = PXP_STAT_LUT_DMA_LOAD_DONE_IRQ;
  }
  if((PXP_STAT & PXP_STAT_NEXT_IRQ) != 0){
    PXP_STAT_CLR = PXP_STAT_NEXT_IRQ;
  }
  if((PXP_STAT & PXP_STAT_AXI_READ_ERROR) != 0){
    PXP_STAT_CLR = PXP_STAT_AXI_READ_ERROR;
  }
  if((PXP_STAT & PXP_STAT_AXI_WRITE_ERROR) != 0){
    PXP_STAT_CLR = PXP_STAT_AXI_WRITE_ERROR;
  }
  if((PXP_STAT & PXP_STAT_IRQ) != 0){
    PXP_STAT_CLR = PXP_STAT_IRQ;
    PXP_done = true;
  }
#if defined(__IMXRT1062__) // Teensy 4.x
  asm("DSB");
#endif
}

void PXP_init(){
  attachInterruptVector(IRQ_PXP, PXP_isr);
  
  CCM_CCGR2 |= CCM_CCGR2_PXP(CCM_CCGR_ON);

  PXP_CTRL_SET = PXP_CTRL_SFTRST; //Reset
  
  PXP_CTRL_CLR = PXP_CTRL_SFTRST | PXP_CTRL_CLKGATE; //Clear reset and gate

  PXP_CTRL_SET = PXP_CTRL_IRQ_ENABLE | PXP_CTRL_NEXT_IRQ_ENABLE | PXP_CTRL_ENABLE;
//  PXP_CTRL_SET = PXP_CTRL_IRQ_ENABLE | PXP_CTRL_NEXT_IRQ_ENABLE;

  PXP_CSC1_COEF0 |= PXP_COEF0_BYPASS;

//  memcpy(&next_pxp, (const void*)&PXP_CTRL, sizeof(IMXRT_PXP_t)/4);

  next_pxp.CTRL = PXP_CTRL_IRQ_ENABLE | PXP_CTRL_NEXT_IRQ_ENABLE | PXP_CTRL_ENABLE;
  next_pxp.OUT_CTRL = PXP_OUT_CTRL;
  next_pxp.PS_SCALE = PXP_PS_SCALE;
  next_pxp.PS_CLRKEYLOW = PXP_PS_CLRKEYLOW_0;
  next_pxp.AS_CLRKEYLOW = PXP_AS_CLRKEYLOW_0;
  next_pxp.CSC1_COEF0 = PXP_COEF0_BYPASS;
    
  PXP_block_size(true);

//  PXP_process();
  
  NVIC_ENABLE_IRQ(IRQ_PXP);
}

void PXP_flip_vertically(bool flip){
  if(flip) next_pxp.CTRL |= PXP_CTRL_VFLIP;
  else next_pxp.CTRL &= ~PXP_CTRL_VFLIP;
}

void PXP_flip_horizontally(bool flip){
  if(flip) next_pxp.CTRL |= PXP_CTRL_HFLIP;
  else next_pxp.CTRL &= ~PXP_CTRL_HFLIP;
}

void PXP_flip_both(bool flip){
  if(flip) next_pxp.CTRL |= (PXP_CTRL_VFLIP | PXP_CTRL_HFLIP);
  else next_pxp.CTRL &= ~(PXP_CTRL_VFLIP | PXP_CTRL_HFLIP);
}

void PXP_block_size(bool size){
  if(size) next_pxp.CTRL |= PXP_CTRL_BLOCK_SIZE;
  else next_pxp.CTRL &= ~PXP_CTRL_BLOCK_SIZE;
}

void PXP_enable_repeat(bool repeat){
  if(repeat) next_pxp.CTRL |= PXP_CTRL_EN_REPEAT;
  else next_pxp.CTRL &= ~PXP_CTRL_EN_REPEAT;
}

void PXP_rotate(uint8_t r){
  next_pxp.CTRL &= ~PXP_CTRL_ROTATE(0xF);
  next_pxp.CTRL |= PXP_CTRL_ROTATE(r);
}

void PXP_rotate_position(bool pos){
  if(pos) next_pxp.CTRL |= PXP_CTRL_ROT_POS;
  else next_pxp.CTRL &= ~PXP_CTRL_ROT_POS;
}

void PXP_output_format(uint8_t format, uint8_t interlaced, bool alpha_override, uint8_t alpha){
  next_pxp.OUT_CTRL &= ~(PXP_OUT_CTRL_FORMAT(0xFF) | PXP_OUT_CTRL_INTERLACED_OUTPUT(0xF) | PXP_OUT_CTRL_ALPHA_OUTPUT | PXP_OUT_CTRL_ALPHA(0xFF));
  next_pxp.OUT_CTRL |= (PXP_OUT_CTRL_FORMAT(format) | PXP_OUT_CTRL_INTERLACED_OUTPUT(interlaced) | (alpha_override ? PXP_OUT_CTRL_ALPHA_OUTPUT : 0) | PXP_OUT_CTRL_ALPHA(alpha));
}

void PXP_output_buffer(void* buf, uint16_t bytesPerPixel, uint16_t width, uint16_t height){
  if(!buf) return;
  next_pxp.OUT_BUF = buf;
  next_pxp.OUT_PITCH = PXP_PITCH(bytesPerPixel * width);
  next_pxp.OUT_LRC = PXP_XCOORD(width-1) | PXP_YCOORD(height-1);
  
  OUT_BUF_width = width;
  OUT_BUF_height = height;
  OUT_BUF_bytesPerPixel = bytesPerPixel;
}

void PXP_output_buffer2(void* buf, uint16_t bytesPerPixel, uint16_t width, uint16_t height){ //Only for interlaced or YUV formats
  next_pxp.OUT_BUF2 = buf;
  
  OUT_BUF2_width = width;
  OUT_BUF2_height = height;
  OUT_BUF2_bytesPerPixel = bytesPerPixel;
}

void PXP_output_clip(uint16_t x, uint16_t y){
  next_pxp.OUT_LRC = PXP_XCOORD(x) | PXP_YCOORD(y);
}

void PXP_input_format(uint8_t format, uint8_t decimationScaleX, uint8_t decimationScaleY, bool wb_swap){
  next_pxp.PS_CTRL = PXP_PS_CTRL_FORMAT(format) | PXP_PS_CTRL_DECX(decimationScaleX) | PXP_PS_CTRL_DECY(decimationScaleY) | (wb_swap ? PXP_PS_CTRL_WB_SWAP : 0);
}

void PXP_input_buffer(void* buf, uint16_t bytesPerPixel, uint16_t width, uint16_t height){
  if(!buf) return;
  next_pxp.PS_BUF = buf;
  next_pxp.PS_PITCH = PXP_PITCH(bytesPerPixel * width);
  PXP_input_position(0, 0, width - 1, height - 1);
  
  PS_BUF_width = width;
  PS_BUF_height = height;
  PS_BUF_bytesPerPixel = bytesPerPixel;
}

void PXP_input_u_buffer(void* buf, uint16_t bytesPerPixel, uint16_t width, uint16_t height){ //Only for YCBCR or YUV formats
  if(!buf) return;
  next_pxp.PS_UBUF = buf;
  
  PS_UBUF_width = width;
  PS_UBUF_height = height;
  PS_UBUF_bytesPerPixel = bytesPerPixel;
}

void PXP_input_v_buffer(void* buf, uint16_t bytesPerPixel, uint16_t width, uint16_t height){ //Only for YCBCR or YUV formats
  if(!buf) return;
  next_pxp.PS_VBUF = buf;
  
  PS_VBUF_width = width;
  PS_VBUF_height = height;
  PS_VBUF_bytesPerPixel = bytesPerPixel;
}

void PXP_input_position(uint16_t x, uint16_t y, uint16_t x1, uint16_t y1){
  next_pxp.OUT_PS_ULC = PXP_XCOORD(x) | PXP_YCOORD(y);
  next_pxp.OUT_PS_LRC = PXP_XCOORD(x1) | PXP_YCOORD(y1);
}

void PXP_input_background_color(uint8_t r8, uint8_t g8, uint8_t b8){
  next_pxp.PS_BACKGROUND = PXP_COLOR((r8 << 16) | (g8 << 8) | (b8));
}

void PXP_input_background_color(uint32_t rgb888){
  next_pxp.PS_BACKGROUND = PXP_COLOR(rgb888);
}

void PXP_input_color_key_low(uint8_t r8, uint8_t g8, uint8_t b8){
  next_pxp.PS_CLRKEYLOW = PXP_COLOR((r8 << 16) | (g8 << 8) | (b8));
}

void PXP_input_color_key_low(uint32_t rgb888){
  next_pxp.PS_CLRKEYLOW = PXP_COLOR(rgb888);
}

void PXP_input_color_key_high(uint8_t r8, uint8_t g8, uint8_t b8){
  next_pxp.PS_CLRKEYLOW = PXP_COLOR((r8 << 16) | (g8 << 8) | (b8));
}

void PXP_input_color_key_high(uint32_t rgb888){
  next_pxp.PS_CLRKEYHIGH = PXP_COLOR(rgb888);
}

void PXP_input_scale(uint16_t x_scale, uint16_t y_scale, uint16_t x_offset, uint16_t y_offset){ //Scale should be reciprocal of desired factor, eg. 4x larger is 0.25e0
  if(x_scale > 0x2.000p0) x_scale = 0x2.000p0; //Maximum downscale of 2
  if(y_scale > 0x2.000p0) y_scale = 0x2.000p0; //Maximum downscale of 2
  next_pxp.PS_SCALE = PXP_XSCALE(x_scale) | PXP_YSCALE(y_scale);
  next_pxp.PS_OFFSET = PXP_XOFFSET(x_offset) | PXP_YOFFSET(y_offset);
}

void PXP_overlay_format(uint8_t format, uint8_t alpha_ctrl, bool colorKey, uint8_t alpha, uint8_t rop, bool alpha_invert){
  next_pxp.AS_CTRL = PXP_AS_CTRL_FORMAT(format) | PXP_AS_CTRL_ALPHA_CTRL(alpha_ctrl) | (colorKey ? PXP_AS_CTRL_ENABLE_COLORKEY : 0) | PXP_AS_CTRL_ALPHA(alpha) | PXP_AS_CTRL_ROP(rop) | (alpha_invert ? PXP_AS_CTRL_ALPHA_INVERT : 0);
}

void PXP_overlay_buffer(void* buf, uint16_t bytesPerPixel, uint16_t width, uint16_t height){
  if(!buf) return;
  next_pxp.AS_BUF = buf;
  next_pxp.AS_PITCH = PXP_PITCH(bytesPerPixel * width);
  PXP_overlay_position(0, 0, width - 1, height - 1);
  
  AS_BUF_width = width;
  AS_BUF_height = height;
  AS_BUF_bytesPerPixel = bytesPerPixel;
}

void PXP_overlay_position(uint16_t x, uint16_t y, uint16_t x1, uint16_t y1){
  next_pxp.OUT_AS_ULC = PXP_XCOORD(x) | PXP_YCOORD(y);
  next_pxp.OUT_AS_LRC = PXP_XCOORD(x1) | PXP_YCOORD(y1);
}

void PXP_overlay_color_key_low(uint8_t r8, uint8_t g8, uint8_t b8){
  next_pxp.AS_CLRKEYLOW = PXP_COLOR((r8 << 16) | (g8 << 8) | (b8));
}

void PXP_overlay_color_key_low(uint32_t rgb888){
  next_pxp.AS_CLRKEYLOW = PXP_COLOR(rgb888);
}

void PXP_overlay_color_key_high(uint8_t r8, uint8_t g8, uint8_t b8){
  next_pxp.AS_CLRKEYLOW = PXP_COLOR((r8 << 16) | (g8 << 8) | (b8));
}

void PXP_overlay_color_key_high(uint32_t rgb888){
  next_pxp.AS_CLRKEYHIGH = PXP_COLOR(rgb888);
}

void PXP_process(){
  while ((PXP_NEXT & PXP_NEXT_ENABLED) != 0){}
  PXP_done = false;
  
  if ((uint32_t)next_pxp.OUT_BUF >= 0x20200000u)  arm_dcache_flush_delete((void*)next_pxp.OUT_BUF, OUT_BUF_width * OUT_BUF_height * OUT_BUF_bytesPerPixel);
  if ((uint32_t)next_pxp.OUT_BUF2 >= 0x20200000u)  arm_dcache_flush_delete((void*)next_pxp.OUT_BUF2, OUT_BUF2_width * OUT_BUF2_height * OUT_BUF2_bytesPerPixel);
  if ((uint32_t)next_pxp.PS_BUF >= 0x20200000u)  arm_dcache_flush_delete((void*)next_pxp.PS_BUF, PS_BUF_width * PS_BUF_height * PS_BUF_bytesPerPixel);
  if ((uint32_t)next_pxp.PS_UBUF >= 0x20200000u)  arm_dcache_flush_delete((void*)next_pxp.PS_UBUF, PS_UBUF_width * PS_UBUF_height * PS_UBUF_bytesPerPixel);
  if ((uint32_t)next_pxp.PS_VBUF >= 0x20200000u)  arm_dcache_flush_delete((void*)next_pxp.PS_VBUF, PS_VBUF_width * PS_VBUF_height * PS_VBUF_bytesPerPixel);
  if ((uint32_t)next_pxp.AS_BUF >= 0x20200000u)  arm_dcache_flush_delete((void*)next_pxp.AS_BUF, AS_BUF_width * AS_BUF_height * AS_BUF_bytesPerPixel);
  
  
  PXP_NEXT = (uint32_t)&next_pxp;
}

void PXP_finish(){
//  while((PXP_STAT & PXP_STAT_IRQ) == 0){}
//  if((PXP_STAT & PXP_STAT_IRQ) != 0){
//    PXP_STAT_CLR = PXP_STAT_IRQ;
//  }
  while(!PXP_done){};
  PXP_CTRL_CLR =  PXP_CTRL_ENABLE;
  if ((uint32_t)next_pxp.OUT_BUF > 0x2020000) arm_dcache_flush_delete((void *)next_pxp.OUT_BUF, OUT_BUF_width * OUT_BUF_height * OUT_BUF_bytesPerPixel);
}
