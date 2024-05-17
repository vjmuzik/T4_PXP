#include "Camera.h"
#include "T4_PXP.h"

/************************************************/
//Specify the pins used for Non-SPI functions of display
//#define TFT_DC 4  
//#define TFT_CS 5 
//#define TFT_RST 2  
#define TFT_DC 9
#define TFT_CS 7
#define TFT_RST 8 
#define use9488

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
#include "TMM_HM0360/HM0360.h"
HM0360 himax;
Camera camera(himax);
#define CameraID 0x0360
#define SCREEN_ROTATION 0
#define MIRROR_FLIP_CAMERA

framesize_t camera_framesize = FRAMESIZE_QVGA;
bool useGPIO = false;
uint8_t bytesperpixel_in = 1;
uint8_t bytesperpixel_out = 2;

uint16_t FRAME_HEIGHT, FRAME_WIDTH;
bool pxpStarted = 0;
uint16_t outputWidth, outputHeight;


// Define Palette for Himax Cameras
#define MCP(m) (uint16_t)(((m & 0xF8) << 8) | ((m & 0xFC) << 3) | (m >> 3))

static const uint16_t mono_palette[256] PROGMEM = {
    MCP(0x00), MCP(0x01), MCP(0x02), MCP(0x03), MCP(0x04), MCP(0x05), MCP(0x06),
    MCP(0x07), MCP(0x08), MCP(0x09), MCP(0x0a), MCP(0x0b), MCP(0x0c), MCP(0x0d),
    MCP(0x0e), MCP(0x0f), MCP(0x10), MCP(0x11), MCP(0x12), MCP(0x13), MCP(0x14),
    MCP(0x15), MCP(0x16), MCP(0x17), MCP(0x18), MCP(0x19), MCP(0x1a), MCP(0x1b),
    MCP(0x1c), MCP(0x1d), MCP(0x1e), MCP(0x1f), MCP(0x20), MCP(0x21), MCP(0x22),
    MCP(0x23), MCP(0x24), MCP(0x25), MCP(0x26), MCP(0x27), MCP(0x28), MCP(0x29),
    MCP(0x2a), MCP(0x2b), MCP(0x2c), MCP(0x2d), MCP(0x2e), MCP(0x2f), MCP(0x30),
    MCP(0x31), MCP(0x32), MCP(0x33), MCP(0x34), MCP(0x35), MCP(0x36), MCP(0x37),
    MCP(0x38), MCP(0x39), MCP(0x3a), MCP(0x3b), MCP(0x3c), MCP(0x3d), MCP(0x3e),
    MCP(0x3f), MCP(0x40), MCP(0x41), MCP(0x42), MCP(0x43), MCP(0x44), MCP(0x45),
    MCP(0x46), MCP(0x47), MCP(0x48), MCP(0x49), MCP(0x4a), MCP(0x4b), MCP(0x4c),
    MCP(0x4d), MCP(0x4e), MCP(0x4f), MCP(0x50), MCP(0x51), MCP(0x52), MCP(0x53),
    MCP(0x54), MCP(0x55), MCP(0x56), MCP(0x57), MCP(0x58), MCP(0x59), MCP(0x5a),
    MCP(0x5b), MCP(0x5c), MCP(0x5d), MCP(0x5e), MCP(0x5f), MCP(0x60), MCP(0x61),
    MCP(0x62), MCP(0x63), MCP(0x64), MCP(0x65), MCP(0x66), MCP(0x67), MCP(0x68),
    MCP(0x69), MCP(0x6a), MCP(0x6b), MCP(0x6c), MCP(0x6d), MCP(0x6e), MCP(0x6f),
    MCP(0x70), MCP(0x71), MCP(0x72), MCP(0x73), MCP(0x74), MCP(0x75), MCP(0x76),
    MCP(0x77), MCP(0x78), MCP(0x79), MCP(0x7a), MCP(0x7b), MCP(0x7c), MCP(0x7d),
    MCP(0x7e), MCP(0x7f), MCP(0x80), MCP(0x81), MCP(0x82), MCP(0x83), MCP(0x84),
    MCP(0x85), MCP(0x86), MCP(0x87), MCP(0x88), MCP(0x89), MCP(0x8a), MCP(0x8b),
    MCP(0x8c), MCP(0x8d), MCP(0x8e), MCP(0x8f), MCP(0x90), MCP(0x91), MCP(0x92),
    MCP(0x93), MCP(0x94), MCP(0x95), MCP(0x96), MCP(0x97), MCP(0x98), MCP(0x99),
    MCP(0x9a), MCP(0x9b), MCP(0x9c), MCP(0x9d), MCP(0x9e), MCP(0x9f), MCP(0xa0),
    MCP(0xa1), MCP(0xa2), MCP(0xa3), MCP(0xa4), MCP(0xa5), MCP(0xa6), MCP(0xa7),
    MCP(0xa8), MCP(0xa9), MCP(0xaa), MCP(0xab), MCP(0xac), MCP(0xad), MCP(0xae),
    MCP(0xaf), MCP(0xb0), MCP(0xb1), MCP(0xb2), MCP(0xb3), MCP(0xb4), MCP(0xb5),
    MCP(0xb6), MCP(0xb7), MCP(0xb8), MCP(0xb9), MCP(0xba), MCP(0xbb), MCP(0xbc),
    MCP(0xbd), MCP(0xbe), MCP(0xbf), MCP(0xc0), MCP(0xc1), MCP(0xc2), MCP(0xc3),
    MCP(0xc4), MCP(0xc5), MCP(0xc6), MCP(0xc7), MCP(0xc8), MCP(0xc9), MCP(0xca),
    MCP(0xcb), MCP(0xcc), MCP(0xcd), MCP(0xce), MCP(0xcf), MCP(0xd0), MCP(0xd1),
    MCP(0xd2), MCP(0xd3), MCP(0xd4), MCP(0xd5), MCP(0xd6), MCP(0xd7), MCP(0xd8),
    MCP(0xd9), MCP(0xda), MCP(0xdb), MCP(0xdc), MCP(0xdd), MCP(0xde), MCP(0xdf),
    MCP(0xe0), MCP(0xe1), MCP(0xe2), MCP(0xe3), MCP(0xe4), MCP(0xe5), MCP(0xe6),
    MCP(0xe7), MCP(0xe8), MCP(0xe9), MCP(0xea), MCP(0xeb), MCP(0xec), MCP(0xed),
    MCP(0xee), MCP(0xef), MCP(0xf0), MCP(0xf1), MCP(0xf2), MCP(0xf3), MCP(0xf4),
    MCP(0xf5), MCP(0xf6), MCP(0xf7), MCP(0xf8), MCP(0xf9), MCP(0xfa), MCP(0xfb),
    MCP(0xfc), MCP(0xfd), MCP(0xfe), MCP(0xff)};

/************************************************/

uint8_t DMAMEM s_fb[(480) * 320] __attribute__((aligned(64)));
uint16_t d_fb[(480) * 320] __attribute__ ((aligned (64)));
const uint32_t sizeof_s_fb = sizeof(s_fb);
const uint32_t sizeof_d_fb = sizeof(d_fb);

uint16_t  image_width = 0;
uint16_t  image_height = 0;
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
  tft.setRotation(3);  // testing external rotation
  test_display();

  start_camera();
  uint32_t imagesize;
  imagesize = FRAME_WIDTH * FRAME_HEIGHT;
  //Note for rotation 0 width and height reversed
  image_width = FRAME_WIDTH;
  image_height = FRAME_HEIGHT;

  showCommands();
  start_pxp();
}

void loop() {
  int ch;
  if (Serial.available()) {
    uint8_t command = Serial.read();
    switch (command) {
      case '0':
        Serial.println(" PXP rotation 0....");
        PXP_ps_output(0, false, 0.0f);
        break;
      case '1':
        Serial.println(" PXP rotation 1....");
        PXP_ps_output(1, false, 0.0f);
        break;
      case '2':
        Serial.println(" PXP rotation 2....");
        PXP_ps_output(2, false, 0.0f);
        break;
      case '3':
        Serial.println(" PXP rotation 3....");
        PXP_ps_output(3, false, 0.8f);
        break;
      case 'd':
        camera.debug(!camera.debug());
        if (camera.debug()) Serial.println("Camera Debug turned on");
        else Serial.println("Camera debug turned off");
        break;
      case 'f':
        capture_frame(true);
        //tft.writeRect8BPP(0, 0, FRAME_WIDTH, FRAME_HEIGHT, s_fb,
        //                      mono_palette);
        tft.setRotation(0);
        break;
      case 's':
        Serial.println("Starting PXP.....");
        start_pxp();
        break;
      case 't':
        test_display();
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

  memset(s_fb, 0, sizeof_s_fb);
  memset(d_fb, 0, sizeof_d_fb);

  pxpStarted = true;
}


void draw_frame(uint16_t width, uint16_t height, const uint16_t *buffer) {
  tft.writeRect(CENTER, CENTER, width, height, buffer );
}


void capture_frame(bool show_debug_info){
  memset((uint8_t *)s_fb, 0, sizeof(s_fb));
  camera.setMode(HIMAX_MODE_STREAMING_NFRAMES, 1);
  camera.useDMA(false);
  camera.readFrame(s_fb , sizeof_s_fb);
  arm_dcache_flush((uint8_t*)s_fb, sizeof_s_fb); // always flush cache after writing to DMAMEM variable that will be accessed by DMA
  Serial.println("Finished reading frame");  

}

void start_camera(){
  // Setup for OV5640 Camera
  // CSI support
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  uint8_t reset_pin = 14;
  uint8_t powdwn_pin = 15;
  pinMode(powdwn_pin, INPUT);
  pinMode(reset_pin, INPUT_PULLUP);

  uint8_t status = 0;
  //camera.setPins(29, 10, 33, 32, 31, 40, 41, 42, 43);
  status = camera.begin(camera_framesize, 15, useGPIO);

  camera.setPixformat(YUV422);

  Serial.printf("Begin status: %d\n", status);
  if (!status) {
    Serial.println("Camera failed to start - try reset!!!");
    //Serial.printf("\tPin 30:%u 31:%u\n", digitalReadFast(30), digitalReadFast(31));
    Serial.printf("\tPin rst(%u):%u PowDn(%u):%u\n", reset_pin, digitalRead(reset_pin), powdwn_pin, digitalRead(powdwn_pin));
    pinMode(reset_pin, OUTPUT);
    digitalWriteFast(reset_pin, LOW);
    delay(500);
    pinMode(reset_pin, INPUT_PULLUP);
    delay(500);
    status = camera.begin(camera_framesize, 15, useGPIO);
    if (!status) {
      Serial.println("Camera failed to start again program halted");
      while (1) {}
    }
  }
  Serial.println("Camera settings:");
  Serial.print("\twidth = ");
  Serial.println(camera.width());
  Serial.print("\theight = ");
  Serial.println(camera.height());
  //Serial.print("\tbits per pixel = NA");
  //Serial.println(camera.bitsPerPixel());
  Serial.println();
  Serial.printf("TFT Width = %u Height = %u\n\n", tft.width(), tft.height());

  FRAME_HEIGHT = camera.height();
  FRAME_WIDTH = camera.width();
  Serial.printf("ImageSize (w,h): %d, %d\n", FRAME_WIDTH, FRAME_HEIGHT);
  // Lets setup camera interrupt priorities:
  // camera.setVSyncISRPriority(102); // higher priority than default
  camera.setDMACompleteISRPriority(192); // lower than default

  camera.setMode(HIMAX_MODE_STREAMING, 0); // turn on, continuous streaming mode

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

void showCommands(){
  Serial.println("\n============  Command List ============");
  Serial.println("\ts => Start PXP");
  Serial.println("\tf => Capture normal frame");
  Serial.println("\tt => Test display");
  Serial.println("\td => Debug camera off");
  Serial.println("\t0 => Display PXP Rotation 0");
  Serial.println("\t1 => Display PXP Rotation 1");
  Serial.println("\t2 => Display PXP Rotation 2");
  Serial.println("\t3 => Display PXP Rotation 3");
  Serial.println("\t? => Show menu.");
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
  PXP_input_buffer(s_fb /* s_fb */, bytesperpixel_in, image_width, image_height);

  /**************************************************************
   * sets the output format to RGB565
   * 
   ****************************************************************/
  // VYUY1P422, PXP_UYVY1P422
  PXP_input_format(PXP_Y8);
  PXP_SetCsc1Mode(0);
  
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
  PXP_output_buffer(d_fb, bytesperpixel_out, out_width, out_height);

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
  Serial.println("Rotating");
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
    Serial.println("Drawing frame");
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

  PXP_output_buffer(d_fb, bytesperpixel_out, outputWidth, outputHeight);
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