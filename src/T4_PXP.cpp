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
  /*
    * Set the process surface position in output buffer
    * x, y: Upper left corner
    * x1, y1: Lower right corners
    */
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


void PXP_GetScaleFactor(uint16_t inputDimension, uint16_t outputDimension, uint8_t *dec, uint32_t *scale)
{
    uint32_t scaleFact = ((uint32_t)inputDimension << 12U) / outputDimension;

    if (scaleFact >= (16U << 12U))
    {
        /* Desired fact is two large, use the largest support value. */
        *dec = 3U;
        *scale = 0x2000U;
    }
    else
    {
        if (scaleFact > (8U << 12U))
        {
            *dec = 3U;
        }
        else if (scaleFact > (4U << 12U))
        {
            *dec = 2U;
        }
        else if (scaleFact > (2U << 12U))
        {
            *dec = 1U;
        }
        else
        {
            *dec = 0U;
        }

        *scale = scaleFact >> (*dec);

        if (0U == *scale)
        {
            *scale = 1U;
        }
    }
}

void PXP_setScaling(uint16_t inputWidth, uint16_t inputHeight, uint16_t outputWidth, uint16_t outputHeight)
{
    uint8_t decX, decY;
    uint32_t scaleX, scaleY;

    PXP_GetScaleFactor(inputWidth, outputWidth, &decX, &scaleX);
    PXP_GetScaleFactor(inputHeight, outputHeight, &decY, &scaleY);

    //next_pxp.PS_CTRL = (next_pxp.CTRL & ~(0xC00U | 0x300U)) | PXP_PS_CTRL_DECX(decX) | PXP_PS_CTRL_DECY(decY);

    next_pxp.PS_SCALE = PXP_XSCALE(scaleX) | PXP_YSCALE(scaleY);
}

void PXP_SetCsc1Mode(uint8_t mode)
{
    //kPXP_Csc1YUV2RGB = 0U, /*!< YUV to RGB. */
    //kPXP_Csc1YCbCr2RGB,    /*!< YCbCr to RGB. */
  
    /*
     * The equations used for Colorspace conversion are:
     *
     * R = C0*(Y+Y_OFFSET)                   + C1(V+UV_OFFSET)
     * G = C0*(Y+Y_OFFSET) + C3(U+UV_OFFSET) + C2(V+UV_OFFSET)
     * B = C0*(Y+Y_OFFSET) + C4(U+UV_OFFSET)
     */

    if (mode == 0)
    {
        //next_pxp.CSC1_COEF0 = (next_pxp.CSC1_COEF0 &
        //                    ~(0x1FFC0000U | 0x1FFU | 0x3FE00U | 0x80000000U)) |
        //                   PXP_COEF0_C0(0x100U)         /* 1.00. */
        //                   | PXP_COEF0_Y_OFFSET(0x0U)   /* 0. */
        //                   | PXP_COEF0_UV_OFFSET(0x0U); /* 0. */
        //next_pxp.CSC1_COEF1 = PXP_COEF1_C1(0x0123U)        /* 1.140. */
        //                   | PXP_COEF1_C4(0x0208U);     /* 2.032. */
        //next_pxp.CSC1_COEF2 = PXP_COEF2_C2(0x076BU)        /* -0.851. */
        //                   | PXP_COEF2_C3(0x079BU);     /* -0.394. */
        next_pxp.CSC1_COEF0 = 0x84ab01f0;
        next_pxp.CSC1_COEF1 =  0;
        next_pxp.CSC1_COEF2 = 0;                   
                           
    }
    else
    {
        next_pxp.CSC1_COEF0 = (next_pxp.CSC1_COEF0 &
                            ~(0x1FFC0000U | 0x1FFU | 0x3FE00U)) |
                           0x80000000U | PXP_COEF0_C0(0x12AU) /* 1.164. */
                           | PXP_COEF0_Y_OFFSET(0x1F0U)                          /* -16. */
                           | PXP_COEF0_UV_OFFSET(0x180U);                        /* -128. */
        next_pxp.CSC1_COEF1 = PXP_COEF1_C1(0x0198U)                                 /* 1.596. */
                           | PXP_COEF1_C4(0x0204U);                              /* 2.017. */
        next_pxp.CSC1_COEF2 = PXP_COEF2_C2(0x0730U)                                 /* -0.813. */
                           | PXP_COEF2_C3(0x079CU);                              /* -0.392. */
    }
}

void PXP_set_csc_y8_to_rgb() {
  next_pxp.CSC1_COEF0 = 0x84ab01f0;
  next_pxp.CSC1_COEF1 =  0;
  next_pxp.CSC1_COEF2 = 0; 
}


/**************************************************************
 * Function that configures PXP for rotation, flip and scaling
 * and outputs to display.
 * rotation: 0 - 0 degrees, 1 - 90 degrees, 
 *           2 - 180 degrees, 3 - 270 degrees
 * flip: false - no flip, true - flip.
 *       control in filp function PXP_flip, currently configured
 *       for horizontal flip.
 * scaling: scaling factor:
 *          To scale up by a factor of 4, the value of 1/4
 *          Follows inverse of factor input so for a scale
 *          factor of 1.5 actual scaling is about 67%
 ***************************************************************/
void PXP_ps_output(uint16_t disp_width, uint16_t disp_height, uint16_t image_width, uint16_t image_height, 
                   void* buf_in, uint8_t format_in, uint8_t bpp_in, uint8_t byte_swap_in, 
                   void* buf_out, uint8_t format_out, uint8_t bpp_out, uint8_t byte_swap_out, 
                   uint8_t rotation, bool flip, float scaling, 
                   uint16_t* scr_width, uint16_t* scr_height)
{
  
  
  uint32_t psUlcX = 0;
  uint32_t psUlcY = 0;
  uint32_t psLrcX, psLrcY;
  if(image_width > image_height) {
    psLrcY = psUlcX + disp_width - 1U;
    psLrcX = psUlcY + disp_height - 1U;
  } else {
    psLrcX = psUlcX + disp_width - 1U;
    psLrcY = psUlcY + disp_height - 1U;
  }
  uint16_t out_width, out_height, output_Width, output_Height;

  //memset((uint8_t *)d_fb, 0, sizeof_d_fb);
  //tft.fillScreen(TFT_BLACK);
  PXP_input_background_color(0, 0, 153);

  /*************************************************************
   * Configures the input buffer to image width and height.
   * 
   **************************************************************/
  PXP_input_buffer(buf_in /* s_fb */, bpp_in, image_width, image_height);

  /**************************************************************
   * sets the output format to RGB565
   * 
   ****************************************************************/
  // VYUY1P422, PXP_UYVY1P422
  PXP_input_format(format_in, 0, 0, byte_swap_in);
  if(format_in == PXP_Y8 || format_in == PXP_Y4)
            PXP_set_csc_y8_to_rgb();
  
  /* sets image corners                                       *
   * ULC: contains the upper left coordinate of the Processed Surface in the output
   * frame buffer (in pixels). Values that are within the PXP_OUT_LRC X,Y extents are
   * valid. The lowest valid value for these fields is 0,0. If the value of the
   * PXP_OUT_PS_ULC is greater than the PXP_OUT_LRC, then no PS pixels will be
   * fetched from memory, but only PXP_PS_BACKGROUND pixels will be processed by
   * the PS engine. Pixel locations that are greater than or equal to the PS upper left
   * coordinates, less than or equal to the PS lower right coordinates, and within the
   * PXP_OUT_LRC extents will use the PS to render pixels into the output buffer.
   *
   * LRC:  contains the size, or lower right coordinate, of the output buffer NOT
   * rotated. It is implied that the upper left coordinate of the output surface is always [0,0].
   * When rotating the framebuffer, the PXP will automatically swap the X/Y, or WIDTH/HEIGHT
   * to accomodate the rotated size.
   *
   * currently configured to match TFT width and height for rotation 0! Note the -1 used with
   * psUlcX and psUlcY - this is per the manual.
   */
  PXP_input_position(psUlcX, psUlcY, psLrcX, psLrcY);  // need this to override the setup in pxp_input_buffer

  /*************************************************************
   * Configures the output buffer to image width and height.
   * width and height will be swapped depending on rotation.
   **************************************************************/
  if (rotation == 1 || rotation == 3) {
    out_width = image_height;
    out_height = image_width;
  } else {
    out_width = image_width;
    out_height = image_height;
  }
  PXP_output_buffer(buf_out, bpp_out, out_width, out_height);

  /**************************************************************
   * sets the output format to RGB565
   * 
   ****************************************************************/
  PXP_output_format(format_out, 0, 0, byte_swap_out);

  // PXP_output_clip sets OUT_LRC register
  /* according to the RM:                                           *
   * The PXP generates an output image in the resolution programmed *
   * by the OUT_LRCregister.                                        *
   * If an image is 480x320, then the for a rotation of 0 you it has*
   * to be reversed to 320x480 since you drawing on a screen that is*
   * 320x480 for the ILI8488.
   *  has to be configured after the output buffer since library
   *  configures it based on the config specied
   ******************************************************************/
  if (rotation == 1 || rotation == 3) {
    PXP_output_clip(out_height - 1, out_width - 1);
  } else {
    PXP_output_clip(out_width - 1, out_height - 1);
  }

  // Rotation
  /* Setting this bit to 1'b0 will place the rotationre sources at  *
   * the output stage of the PXP data path. Image compositing will  *
   * occur before pixels are processed for rotation.                *
   * Setting this bit to a 1'b1 will place the rotation resources   *
   * before image composition.                                      *
   */
  PXP_rotate_position(0);
  //Serial.println("Rotating");
  // Performs the actual rotation specified
  PXP_rotate(rotation);
 
  // flip - pretty straight forward
  PXP_flip(flip);

  /************************************************************
   * if performing scaling we call out to the scaling function
   * which will perform remaining scaling and send to display.
   ************************************************************/
  if(scaling > 0.0f){
    PXP_scaling(buf_out, bpp_out, scaling, out_width, out_height, rotation, &output_Width, &output_Height);
    *scr_width = output_Width;
    *scr_height = output_Height;
  } else {
  //  PXP_process();
  //  Serial.println("Drawing frame");
  //  draw_frame(out_width, out_height, buf_out);
    *scr_width = out_width;
    *scr_height = out_height;
  }
  
  PXP_process();

}


void PXP_scaling(void *buf_out, uint8_t bbp_out, float downScaleFact,
                  uint16_t width, uint16_t height, uint8_t rotation,
                  uint16_t* outputWidth, uint16_t* outputHeight) {
  uint16_t IMG_WIDTH  = width;
  uint16_t IMG_HEIGHT = height;
  uint16_t output_Width = (uint16_t)((float)(IMG_WIDTH) / downScaleFact);
  uint16_t output_Height = (uint16_t)((float)(IMG_HEIGHT) / downScaleFact);

  //capture_frame(false);
  PXP_input_background_color(0, 153, 0);

  PXP_output_buffer(buf_out, bbp_out, output_Width, output_Height);
  if(rotation == 1 || rotation == 3) {
    PXP_output_clip( output_Height - 1, output_Width - 1);
  } else {
    PXP_output_clip( output_Width - 1, output_Height - 1);
  }
  PXP_setScaling( IMG_WIDTH, IMG_HEIGHT, output_Width, output_Height);

  PXP_process();
  //tft.fillScreen(TFT_GREEN);
  //draw_frame(outputWidth, outputHeight, buf_out );
  *outputWidth = output_Width;
  *outputHeight = output_Height;

}

void PXP_flip(bool flip) {
  /* there are 3 flip commands that you can use           *
   * PXP_flip_vertically                                  *
   * PXP_flip_horizontally                                *
   * PXP_flip_both                                        *
   */
  PXP_flip_both(flip);
}