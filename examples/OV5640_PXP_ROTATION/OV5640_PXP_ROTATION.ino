#include "Camera.h"

#include "T4_PXP.h"

/************************************************/
//Specify the pins used for Non-SPI functions of display
#define TFT_DC 4  
#define TFT_CS 5 
#define TFT_RST 2  

//#define use9488

//#define ARDUCAM_CAMERA_OV2640
#define ARDUCAM_CAMERA_OV5640


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
#if defined(ARDUCAM_CAMERA_OV2640)
#include "TMM_OV2640/OV2640.h"
OV2640 omni;
Camera camera(omni);
#define CameraID OV2640a
#define MIRROR_FLIP_CAMERA
#elif defined(ARDUCAM_CAMERA_OV5640)
#include "TMM_OV5640/OV5640.h"
OV5640 omni;
Camera camera(omni);
#define CameraID OV5640a
#define MIRROR_FLIP_CAMERA
#else
#error "Camera not selected"
#endif

//set cam configuration - need to remember when saving jpeg
framesize_t camera_framesize = FRAMESIZE_QVGA;  //HVGA is 480x320
pixformat_t camera_format = RGB565;
bool useGPIO = false;

uint16_t FRAME_HEIGHT, FRAME_WIDTH;
/************************************************/

DMAMEM uint16_t s_fb[640 * 240] __attribute__ ((aligned (64)));
uint16_t d_fb[640 * 240] __attribute__ ((aligned (64)));
const uint32_t sizeof_s_fb = sizeof(s_fb);
const uint32_t sizeof_d_fb = sizeof(d_fb);



uint16_t  cam_width = 0;
uint16_t  cam_height = 0;
bool pxpStarted = 0;
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

  start_camera();

  cam_width = camera.width();
  cam_height = camera.height();

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
      case 'd':
        camera.debug(!camera.debug());
        if (camera.debug()) Serial.println("Camera Debug turned on");
        else Serial.println("Camera debug turned off");
        break;
      case 'f':
        tft.setRotation(3);
        capture_frame(false);
        tft.writeRect(CENTER, CENTER, camera.width(), camera.height(), (uint16_t *)s_fb );
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

  Serial.printf("bwitdh: %d, cam_height: %d\n", cam_width, cam_height);

  PXP_SetOutputCorners(0, 0, (cam_width - 1), (cam_height - 1));

  PXP_input_buffer(s_fb, 2, cam_width, cam_height);
  //PXP_input_format(PXP_RGB565);
  PXP_input_format(PXP_RGB565, 0, 0, 0);
  PXP_output_buffer(d_fb, 2, cam_height, cam_width);
  PXP_output_format(PXP_RGB565);

  pxpStarted = true;
}

void pxp_rotation(int r, bool flip) {
  if(!pxpStarted) {
    Serial.println("You forgot to start PXP, use 's' to start.....");
  } else {
    memset((uint8_t *)d_fb, 0, sizeof_d_fb);
    PXP_rotate_position(0);  //Rotation position, 1=input
    capture_frame(false);
    if(r == 0 || r == 2) {
      PXP_output_clip(cam_height-1, cam_width-1);
    } else {
      PXP_output_clip( cam_width-1, cam_height-1);
    }
    test_flip(flip);
    PXP_rotate(r);
    PXP_process();
    draw_frame();
  }
}


void test_flip(bool flip) {
  Serial.println("Testing flip");
  PXP_flip_both(flip);
} 

void rotation_after_scaling() {
  float downScaleFact = 2.0f;
  uint16_t outputWidth, outputHeight;

  uint16_t APP_IMG_WIDTH  = cam_width;
  uint16_t APP_IMG_HEIGHT = cam_height;
  outputWidth = (uint16_t)((float)(APP_IMG_WIDTH) / downScaleFact);
  outputHeight = (uint16_t)((float)(APP_IMG_HEIGHT) / downScaleFact);

  tft.fillScreen(TFT_BLACK)
  test_scaling();

  PXP_rotate_position(0); 
  PXP_output_clip( outputHeight-1, outputWidth-1);
  PXP_rotate(3);
  PXP_process();
  delay(2000);
  tft.fillScreen(TFT_BLACK)
  tft.writeRect(CENTER, CENTER, outputWidth, outputHeight, (uint16_t *)d_fb );
}

void test_scaling() {
  float downScaleFact = 2.0f;
  uint16_t outputWidth, outputHeight;

/* PS input buffer is not square. */
/*  good for rotation = 0
#if APP_IMG_WIDTH > APP_IMG_HEIGHT
#define APP_PS_WIDTH APP_IMG_HEIGHT      //272
#define APP_PS_HEIGHT (APP_IMG_HEIGHT/2+8)      //272
#else
#define APP_PS_SIZE APP_IMG_WIDTH
#endif
*/
  uint16_t APP_IMG_WIDTH  = cam_width;
  uint16_t APP_IMG_HEIGHT = cam_height;
  outputWidth = (uint16_t)((float)(APP_IMG_WIDTH) / downScaleFact);
  outputHeight = (uint16_t)((float)(APP_IMG_HEIGHT) / downScaleFact);

  capture_frame(false);

  //PXP_block_size(8);
  //PXP_setScaling(APP_IMG_HEIGHT, APP_IMG_WIDTH, outputWidth, outputHeight );
  PXP_SetOutputCorners(0, 0,  outputWidth - 1, outputHeight - 1);
  PXP_output_buffer(d_fb, 2, outputWidth, outputHeight);
  PXP_setScaling( APP_IMG_WIDTH, APP_IMG_HEIGHT, outputWidth, outputHeight);

  PXP_process();
  tft.writeRect(CENTER, CENTER, outputWidth, outputHeight, (uint16_t *)d_fb );
}

void draw_frame() {
  //arm_dcache_flush(d_fb, sizeof(d_fb)); // always flush cache after writing to DMAMEM variable that will be accessed by DMA
  tft.writeRect(CENTER, CENTER, camera.height(),camera.width(), (uint16_t *)d_fb );
}

void capture_frame(bool show_debug_info){
  tft.fillScreen(TFT_BLACK);
  memset((uint8_t *)s_fb, 0, sizeof_s_fb);
  camera.useDMA(true);
  camera.readFrame(s_fb , sizeof_s_fb);
  arm_dcache_flush((uint16_t*)s_fb, sizeof_s_fb); // always flush cache after writing to DMAMEM variable that will be accessed by DMA
  
  if (show_debug_info) {
      for (volatile uint16_t *pfb = s_fb;
            pfb < (s_fb + 4 * camera.width());
            pfb += camera.width()) {
          Serial.printf("\n%08x: ", (uint32_t)pfb);
          for (uint16_t i = 0; i < 8; i++)
              Serial.printf("%02x ", pfb[i]);
          Serial.print("..");
          Serial.print("..");
          for (uint16_t i = camera.width() - 8; i < camera.width(); i++)
              Serial.printf("%04x ", pfb[i]);
      }
      Serial.println("\n");

    // Lets dump out some of center of image.
    Serial.println("Show Center pixels\n");

    for (volatile uint16_t *pfb =
        s_fb + camera.width() * ((camera.height() / 2) - 8);
        pfb <
        (s_fb + camera.width() * (camera.height() / 2 + 8));
        pfb += camera.width()) {
        Serial.printf("\n%08x: ", (uint32_t)pfb);
        for (uint16_t i = 0; i < 8; i++)
            Serial.printf("%02x ", pfb[i]);
        Serial.print("..");
        for (uint16_t i = (camera.width() / 2) - 4;
            i < (camera.width() / 2) + 4; i++)
            Serial.printf("%02x ", pfb[i]);
        Serial.print("..");
        for (uint16_t i = camera.width() - 8; i < camera.width(); i++)
            Serial.printf("%02x ", pfb[i]);
    }
    Serial.println("\n...");
    Serial.printf("TFT(%u, %u) Camera(%u, %u)\n", tft.width(), tft.height(), camera.width(), camera.height());
  }
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
  camera.setPins(29, 10, 33, 32, 31, 40, 41, 42, 43, 44, 45, 6, 9);
  status = camera.begin(camera_framesize, camera_format, 15, CameraID, useGPIO);
  camera.setHmirror(true);
  camera.setVflip(true);

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
    status = camera.begin(camera_framesize, camera_format, 15, CameraID, useGPIO);
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
  Serial.println("\t4 => PXP rotation 3 w/flip both axes");
  Serial.println("\t5 => PXP 1/2 Scaling Test");
  Serial.println("\t6 => PXP Scaling w/Rotation 3 Test");

  Serial.println("\t? => Show menu.");
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

