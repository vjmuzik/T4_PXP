#include "flexio_teensy_mm.c"
#include "T4_PXP.h"
#include <MemoryHexDump.h>

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

//#define ARDUCAM_CAMERA_OV2640
#define ARDUCAM_CAMERA_OV5640

uint16_t  cam_width = 480;
uint16_t  cam_height = 320;

float downScaleFact = 1.5f;
uint8_t bytesperpixel = 2;
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

//DMAMEM uint16_t s_fb[640 * 240] __attribute__ ((aligned (64)));
//uint16_t d_fb[640 * 240] __attribute__ ((aligned (64)));
DMAMEM uint16_t s_fb[480 * 320] __attribute__ ((aligned (64)));
uint16_t d_fb[480 * 320] __attribute__ ((aligned (64)));
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
  tft.setRotation(0);  // testing external rotation
  test_display();

  start_pxp();

  showCommands();
}

void loop() {
  int ch;
  if (Serial.available()) {
    uint8_t command = Serial.read();
    switch (command) {
      case '0':
        Serial.println(" PXP rotation 0....");
        pxp_rotation(0, false);
        break;
      case '1':
        Serial.println(" PXP rotation 1....");
        pxp_rotation(1, false);
        break;
      case '2':
        Serial.println(" PXP rotation 2....");
        pxp_rotation(2, false);
        break;
      case '3':
        Serial.println(" PXP rotation 3....");
        pxp_rotation(3, false);
        break;
      case '4':
        Serial.println(" PXP rotation 3 w/flip both axes....");
        pxp_rotation(3, true);
        break;
      case '5':
        Serial.println("Test 1/2 scaling - TBD");
        test_scaling();
        break;
      case '6':
        rotation_after_scaling();  //rotation to landscape
        break;
      case 'f':
        tft.setRotation(3);
        capture_frame(false);
        tft.writeRect(CENTER, CENTER, cam_width, cam_height, (uint16_t *)s_fb );
        tft.setRotation(0);
        break;
      case 's':
        Serial.println("Starting PXP.....");
        start_pxp();
        break;
      case 't':
        test_display();
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

void start_pxp(){
  PXP_init();

  Serial.printf("cam_width: %d, cam_height: %d\n", cam_width, cam_height);

  memset(s_fb, 0, sizeof_s_fb);
  memset(d_fb, 0, sizeof_d_fb);

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
   */
  PXP_SetOutputCorners(0, 0, (cam_width - 1), (cam_height - 1));

  /* Sets the Input buffer                      */
  PXP_input_buffer(s_fb, bytesperpixel, cam_width, cam_height);

  //PXP_input_format(PXP_RGB565);
  /* sets the input format
   * Using this format since the last postion will allow byte swappin if set to 1
   */
  PXP_input_format(PXP_RGB565, 0, 0, 0);

  /* sets the output buffer
   * swaps height and width since we are working in non-h/w rotation when using PXP
   */
  PXP_output_buffer(d_fb, bytesperpixel, cam_height, cam_width);
  PXP_output_format(PXP_RGB565);

  pxpStarted = true;
}

void pxp_rotation(int r, bool flip) {
  if(!pxpStarted) {
    Serial.println("You forgot to start PXP, use 's' to start.....");
  } else {
    memset((uint8_t *)d_fb, 0, sizeof_d_fb);

    /* Setting this bit to 1'b0 will place the rotationre sources at  *
     * the output stage of the PXP data path. Image compositing will  *
     * occur before pixels are processed for rotation.                *
     * Setting this bit to a 1'b1 will place the rotation resources   *
     * before image composition.                                      *
     */
    PXP_rotate_position(0);

    capture_frame(false);

    // PXP_output_clip sets OUT_LRC register
    /* according to the RM:                                           *
     * The PXP generates an output image in the resolution programmed *
     * by the OUT_LRCregister.                                        *
     * If an image is 480x320, then the for a rotation of 0 you it has*
     * to be reversed to 320x480 since you drawing on a screen that is*
     * 320x480 for the ILI8488.                                       *
     */
    if(r == 0 || r == 2) {
      PXP_output_clip(cam_height-1, cam_width-1); 
    } else {
      PXP_output_clip( cam_width-1, cam_height-1);
    }

    /* function to flip an image in the buffer         */
    test_flip(flip);

    /* Input function that setup the rotation:          *
     * 0 = 0 degrees, 1 = 90 degrees, 2 = 180, 3 = 270  *
     */
    PXP_rotate(r);

    /* Processes the frame and looks like it deletes cache */
    PXP_process();

    /* tft functions needed to draw the image on the screen*/
    draw_frame();
  }
}

void test_flip(bool flip) {
  Serial.println("Testing flip");
  /* there are 3 flip commands that you can use           *
   * PXP_flip_vertically                                  *
   * PXP_flip_horizontally                                *
   * PXP_flip_both                                        *
   */
  PXP_flip_both(flip);
} 

void rotation_after_scaling() {
  test_scaling();
  delay(1000);
  tft.fillScreen(TFT_BLACK);
    memset((uint8_t *)d_fb, 0, outputWidth*outputHeight*bytesperpixel);
    PXP_rotate_position(0);
    //capture_frame(false);
    //PXP_output_clip( outputWidth-1, outputHeight-1);
    PXP_rotate(3);
    PXP_process();

  draw_frame();
  //tft.writeRect(0, 0, outputHeight, outputWidth, (uint16_t *)d_fb );
  start_pxp();
}

void test_scaling() {
  uint16_t APP_IMG_WIDTH  = cam_width;
  uint16_t APP_IMG_HEIGHT = cam_height;
  outputWidth = (uint16_t)((float)(APP_IMG_WIDTH) / downScaleFact);
  outputHeight = (uint16_t)((float)(APP_IMG_HEIGHT) / downScaleFact);

  capture_frame(false);
  
  PXP_input_background_color(0, 0, 153);
  PXP_SetOutputCorners(0, 0,  outputWidth - 1, outputHeight - 1);
  PXP_output_buffer(d_fb, bytesperpixel, outputWidth, outputHeight);
  PXP_setScaling( APP_IMG_WIDTH, APP_IMG_HEIGHT, outputWidth, outputHeight);

  PXP_process();
  tft.writeRect(CENTER, CENTER, outputWidth, outputHeight, (uint16_t *)d_fb );
  //draw_frame();
}

void draw_frame() {
  //arm_dcache_flush(d_fb, sizeof(d_fb)); // always flush cache after writing to DMAMEM variable that will be accessed by DMA
  tft.writeRect(CENTER, CENTER, cam_height, cam_width, (uint16_t *)d_fb );
}

void capture_frame(bool show_debug_info){
  tft.fillScreen(TFT_BLACK);
  memcpy((uint16_t *)s_fb, flexio_teensy_mm, sizeof(flexio_teensy_mm));
 
}


void showCommands(){
  Serial.println("\n============  Command List ============");
  Serial.println("\ts => Reset/Start PXP");
  Serial.println("\tf => Capture normal frame");
  Serial.println("\tt => Test display");
  Serial.println("\t0 => Display PXP Rotation 0");
  Serial.println("\t1 => Display PXP Rotation 1");
  Serial.println("\t2 => Display PXP Rotation 2");
  Serial.println("\t3 => Display PXP Rotation 3");
  Serial.println("\t4 => PXP rotation 3 w/flip both axes");
  Serial.println("\t5 => PXP 1/2 Scaling Test");
  Serial.println("\t6 => PXP Scaling w/Rotation 3 Test");

  Serial.println("\t? => Show menu.");
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
  Serial.printf("OUT_CTRL:     %08X       OUT_BUF:      %08X    \nOUT_BUF2:     %08X\n", PXP_OUT_CTRL,PXP_OUT_BUF,PXP_OUT_BUF2);
  Serial.printf("OUT_PITCH:    %8lu       OUT_LRC:       %3u,%3u\n", PXP_OUT_PITCH, PXP_OUT_LRC>>16, PXP_OUT_LRC&0xFFFF);

  Serial.printf("OUT_PS_ULC:    %3u,%3u       OUT_PS_LRC:    %3u,%3u\n", PXP_OUT_PS_ULC>>16, PXP_OUT_PS_ULC&0xFFFF,
                                                               PXP_OUT_PS_LRC>>16, PXP_OUT_PS_LRC&0xFFFF);
  Serial.printf("OUT_AS_ULC:    %3u,%3u       OUT_AS_LRC:    %3u,%3u\n", PXP_OUT_AS_ULC>>16, PXP_OUT_AS_ULC&0xFFFF,
                                                               PXP_OUT_AS_LRC>>16, PXP_OUT_AS_LRC&0xFFFF);
  Serial.println();  // section separator
  Serial.printf("PS_CTRL:      %08X       PS_BUF:       %08X\n", PXP_PS_CTRL,PXP_PS_BUF);
  Serial.printf("PS_UBUF:      %08X       PS_VBUF:      %08X\n", PXP_PS_UBUF, PXP_PS_VBUF);
  Serial.printf("PS_PITCH:     %8lu       PS_BKGND:     %08X\n", PXP_PS_PITCH, PXP_PS_BACKGROUND_0);
  Serial.printf("PS_SCALE:     %08X       PS_OFFSET:    %08X\n", PXP_PS_SCALE,PXP_PS_OFFSET);
  Serial.printf("PS_CLRKEYLOW: %08X       PS_CLRKEYLHI: %08X\n", PXP_PS_CLRKEYLOW_0,PXP_PS_CLRKEYHIGH_0);
  Serial.println();
  Serial.printf("AS_CTRL:      %08X       AS_BUF:       %08X    AS_PITCH: %6u\n", PXP_AS_CTRL,PXP_AS_BUF, PXP_AS_PITCH & 0xFFFF);
  Serial.printf("AS_CLRKEYLOW: %08X       AS_CLRKEYLHI: %08X\n", PXP_AS_CLRKEYLOW_0,PXP_AS_CLRKEYHIGH_0);
  Serial.println();
  Serial.printf("CSC1_COEF0:   %08X       CSC1_COEF1:   %08X    \nCSC1_COEF2:   %08X\n", 
                                                                PXP_CSC1_COEF0,PXP_CSC1_COEF1,PXP_CSC1_COEF2);
  Serial.println();  // section separator
  Serial.printf("POWER:        %08X       NEXT:         %08X\n", PXP_POWER,PXP_NEXT);
  Serial.printf("PORTER_DUFF:  %08X\n", PXP_PORTER_DUFF_CTRL);
}

