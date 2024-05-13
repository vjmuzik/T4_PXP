
#include "T4_PXP.h"

//Select One
//#include "little_joe.h"
//#include "ida.h"
//#include "flexio_teensy_mm.h"
#include "testPattern.h"

/************************************************/
//Specify the pins used for Non-SPI functions of display
#if defined(ARDUINO_TEENSY_MICROMOD)
#define TFT_DC 4
#define TFT_CS 5
#define TFT_RST 2
#else
#define TFT_DC 9
#define TFT_CS 7
#define TFT_RST 8
#endif

#define use9488


uint8_t bytesperpixel = 2;
float ScaleFact = 0.9f;

/************************************************/

#if defined(use9488)
#include <ILI9488_t3.h>
ILI9488_t3 tft = ILI9488_t3(TFT_CS, TFT_DC, TFT_RST);
#define TFT_BLACK ILI9488_BLACK
#define TFT_YELLOW ILI9488_YELLOW
#define TFT_RED ILI9488_RED
#define TFT_GREEN ILI9488_GREEN
#define TFT_BLUE ILI9488_BLUE
#define CENTER ILI9488_t3::CENTER

#else
#include <ILI9341_t3n.h>
ILI9341_t3n tft = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST);
#define TFT_BLACK ILI9341_BLACK
#define TFT_YELLOW ILI9341_YELLOW
#define TFT_RED ILI9341_RED
#define TFT_GREEN ILI9341_GREEN
#define TFT_BLUE ILI9341_BLUE
#define CENTER ILI9341_t3n::CENTER
#endif
/************************************************/


// NOTE: THIS SHOULD BE THE SIZE OF THE DISPLAY
DMAMEM uint16_t s_fb[480 * 320] __attribute__((aligned(64)));
uint16_t d_fb[480 * 320] __attribute__((aligned(64)));
const uint32_t sizeof_s_fb = sizeof(s_fb);
const uint32_t sizeof_d_fb = sizeof(d_fb);


bool pxpStarted = 0;
uint16_t outputWidth, outputHeight;
uint16_t FRAME_HEIGHT, FRAME_WIDTH;
/************************************************/

void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 5000) {}

  if (CrashReport) {
    Serial.print(CrashReport);
    Serial.println("Press any key to continue");
    while (Serial.read() != -1) {}
    while (Serial.read() == -1) {}
    while (Serial.read() != -1) {}
  }

  // Start ILI9341
  tft.begin();
  // Must be set to HW rotation 0 for PXP to work properly
  tft.setRotation(0);
  test_display();

  /******************************************
   * Runs PXP_init and memset buffers to 0.
   ******************************************/
  start_pxp();
  
  showCommands();
}


void loop() {
  int ch;
  if (Serial.available()) {
    uint8_t command = Serial.read();
    switch (command) {
      case '0':
        PXP_ps_output(0, false, 0.0f);
        break;
      case '1':
        PXP_ps_output(1, 0, 0.0f);
        break;
      case '2':
        PXP_ps_output(2, 0, 0.0f);
        break;
      case '3':
        PXP_ps_output(3, 0, 0.0f);
        break;
      case '4':
        PXP_ps_output(0, 1, 0.0f);
        break;
      case '5':
        PXP_ps_output(2, 0, ScaleFact);
        start_pxp();
        break;
      case '6':
        PXP_ps_output(3, 0, ScaleFact);
        start_pxp();
        break;
      case 'f':
        tft.setRotation(0);
        capture_frame(false);
        draw_frame(image_width, image_height, s_fb);
        //tft.writeRect(CENTER, CENTER, image_width, image_height, (uint16_t *)s_fb);
        tft.setRotation(0);
        break;
      case 's':
        Serial.println("Starting PXP.....");
        start_pxp();
        break;
      case 't':
        test_display();
        break;
      case 'x':
        draw_source();
        break;
      case 'p':
        PXPShow();
        break;
      case '?':
        showCommands();
      default:
        break;
    }
  }
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
void PXP_ps_output(uint8_t rotation, bool flip, float scaling) {
  
  
  uint32_t psUlcX = 0;
  uint32_t psUlcY = 0;
  uint32_t psLrcX, psLrcY;
  if(image_width > image_height) {
    psLrcY = psUlcX + tft.width() - 1U;
    psLrcX = psUlcY + tft.height() - 1U;
  } else {
    psLrcX = psUlcX + tft.width() - 1U;
    psLrcY = psUlcY + tft.height() - 1U;
  }
  uint32_t out_width, out_height;

  //memset((uint8_t *)d_fb, 0, sizeof_d_fb);
  tft.fillScreen(TFT_BLACK);
  PXP_input_background_color(0, 0, 153);

  /*************************************************************
   * Configures the input buffer to image width and height.
   * 
   **************************************************************/
  PXP_input_buffer(s_fb /* s_fb */, bytesperpixel, image_width, image_height);
  
  /**************************************************************
   * sets the output format to RGB565
   * 
   ****************************************************************/
  PXP_input_format(PXP_RGB565);
  
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

  // Generic function to capture an image and put it in the source buffer
  capture_frame(false);

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
  PXP_output_buffer(d_fb, bytesperpixel, out_width, out_height);
  
  /**************************************************************
   * sets the output format to RGB565
   * 
   ****************************************************************/
  PXP_output_format(PXP_RGB565);

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
  
  // Performs the actual rotation specified
  PXP_rotate(rotation);
 
  // flip - pretty straight forward
  PXP_flip(flip);

  /************************************************************
   * if performing scaling we call out to the scaling function
   * which will perform remaining scaling and send to display.
   ************************************************************/
  if(scaling > 0.0f){
    PXP_scaling(scaling, out_width, out_height, rotation);
  } else {
    PXP_process();
    draw_frame(out_width, out_height, d_fb);
  }

}


void PXP_scaling(float downScaleFact, uint16_t width, uint16_t height, uint8_t rotation) {
  uint16_t IMG_WIDTH  = width;
  uint16_t IMG_HEIGHT = height;
  outputWidth = (uint16_t)((float)(IMG_WIDTH) / downScaleFact);
  outputHeight = (uint16_t)((float)(IMG_HEIGHT) / downScaleFact);

  //capture_frame(false);
  PXP_input_background_color(0, 153, 0);

  PXP_output_buffer(d_fb, bytesperpixel, outputWidth, outputHeight);
  if(rotation == 1 || rotation == 3) {
    PXP_output_clip( outputHeight - 1, outputWidth - 1);
  } else {
    PXP_output_clip( outputWidth - 1, outputHeight - 1);
  }
  PXP_setScaling( IMG_WIDTH, IMG_HEIGHT, outputWidth, outputHeight);

  PXP_process();
  tft.fillScreen(TFT_GREEN);
  draw_frame(outputWidth, outputHeight, d_fb );

}

void PXP_flip(bool flip) {
  /* there are 3 flip commands that you can use           *
   * PXP_flip_vertically                                  *
   * PXP_flip_horizontally                                *
   * PXP_flip_both                                        *
   */
  PXP_flip_both(flip);
} 

void start_pxp() {
  PXP_init();

  memset(s_fb, 0, sizeof_s_fb);
  memset(d_fb, 0, sizeof_d_fb);

  pxpStarted = true;
}

void draw_frame(uint16_t width, uint16_t height, const uint16_t *buffer) {
  tft.writeRect(CENTER, CENTER, width, height, buffer );
}

void showCommands() {
  Serial.println("\n============  Command List ============");
  Serial.println("\ts => Reset/Start PXP");
  Serial.println("\tf => Capture normal frame");
  Serial.println("\tt => Test display");
  Serial.println("\tp => Print Registers");
  Serial.println("\t0 => Display PXP Rotation 0");
  Serial.println("\t1 => Display PXP Rotation 1");
  Serial.println("\t2 => Display PXP Rotation 2");
  Serial.println("\t3 => Display PXP Rotation 3");
  Serial.println("\t4 => PXP rotation 0 w/flip both axes");
  Serial.println("\t5 => PXP Scaling Test");
  Serial.println("\t6 => PXP Scaling w/Rotation 3 Test");


  Serial.println("\t? => Show menu.");
}

void capture_frame(bool show_debug_info) {
  tft.fillScreen(TFT_BLACK);
  #if defined(has_ida)
  memcpy((uint16_t *)s_fb, ida, sizeof(ida));
  #elif defined(has_littlejoe)
  memcpy((uint16_t *)s_fb, little_joe, sizeof(little_joe));
  #elif defined(has_teensymm)
  memcpy((uint16_t *)s_fb, flexio_teensy_mm, sizeof(flexio_teensy_mm));
  #elif defined(has_testpattern)
  memcpy((uint16_t *)s_fb, testPattern, sizeof(testPattern));
  #endif
}

void test_display() {
  tft.setRotation(3);
  tft.fillScreen(TFT_RED);
  delay(500);
  tft.fillScreen(TFT_GREEN);
  delay(500);
  tft.fillScreen(TFT_BLUE);
  delay(500);
  tft.fillScreen(TFT_BLACK);
  delay(500);
  tft.setRotation(0);
}

// This  function prints a nicely formatted output of the PXP register settings
// The formatting does require using a monospaced font, like Courier
void PXPShow(void) {
  Serial.printf("CTRL:         %08X       STAT:         %08X\n", PXP_CTRL, PXP_STAT);
  Serial.printf("OUT_CTRL:     %08X       OUT_BUF:      %08X    \nOUT_BUF2:     %08X\n", PXP_OUT_CTRL, PXP_OUT_BUF, PXP_OUT_BUF2);
  Serial.printf("OUT_PITCH:    %8lu       OUT_LRC:       %3u,%3u\n", PXP_OUT_PITCH, PXP_OUT_LRC >> 16, PXP_OUT_LRC & 0xFFFF);

  Serial.printf("OUT_PS_ULC:    %3u,%3u       OUT_PS_LRC:    %3u,%3u\n", PXP_OUT_PS_ULC >> 16, PXP_OUT_PS_ULC & 0xFFFF,
                PXP_OUT_PS_LRC >> 16, PXP_OUT_PS_LRC & 0xFFFF);
  Serial.printf("OUT_AS_ULC:    %3u,%3u       OUT_AS_LRC:    %3u,%3u\n", PXP_OUT_AS_ULC >> 16, PXP_OUT_AS_ULC & 0xFFFF,
                PXP_OUT_AS_LRC >> 16, PXP_OUT_AS_LRC & 0xFFFF);
  Serial.println();  // section separator
  Serial.printf("PS_CTRL:      %08X       PS_BUF:       %08X\n", PXP_PS_CTRL, PXP_PS_BUF);
  Serial.printf("PS_UBUF:      %08X       PS_VBUF:      %08X\n", PXP_PS_UBUF, PXP_PS_VBUF);
  Serial.printf("PS_PITCH:     %8lu       PS_BKGND:     %08X\n", PXP_PS_PITCH, PXP_PS_BACKGROUND_0);
  Serial.printf("PS_SCALE:     %08X       PS_OFFSET:    %08X\n", PXP_PS_SCALE, PXP_PS_OFFSET);
  Serial.printf("PS_CLRKEYLOW: %08X       PS_CLRKEYLHI: %08X\n", PXP_PS_CLRKEYLOW_0, PXP_PS_CLRKEYHIGH_0);
  Serial.println();
  Serial.printf("AS_CTRL:      %08X       AS_BUF:       %08X    AS_PITCH: %6u\n", PXP_AS_CTRL, PXP_AS_BUF, PXP_AS_PITCH & 0xFFFF);
  Serial.printf("AS_CLRKEYLOW: %08X       AS_CLRKEYLHI: %08X\n", PXP_AS_CLRKEYLOW_0, PXP_AS_CLRKEYHIGH_0);
  Serial.println();
  Serial.printf("CSC1_COEF0:   %08X       CSC1_COEF1:   %08X    \nCSC1_COEF2:   %08X\n",
                PXP_CSC1_COEF0, PXP_CSC1_COEF1, PXP_CSC1_COEF2);
  Serial.println();  // section separator
  Serial.printf("POWER:        %08X       NEXT:         %08X\n", PXP_POWER, PXP_NEXT);
  Serial.printf("PORTER_DUFF:  %08X\n", PXP_PORTER_DUFF_CTRL);
}

void draw_source() {
  tft.fillScreen(TFT_BLACK);
  draw_frame(image_width, image_height, s_fb);
  //tft.writeRect(CENTER, CENTER, image_width, image_height, (uint16_t *)s_fb);
}