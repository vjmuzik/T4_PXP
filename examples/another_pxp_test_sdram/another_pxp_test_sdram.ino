
#include "T4_PXP.h"

//Select One
//#include "little_joe.h"
//#include "ida.h"
#include "flexio_teensy_mm.h"
//#include "testPattern.h"

/************************************************/
#define USE_MMOD_ATP_ADAPTER
#define use9488

#ifdef ARDUINO_TEENSY_DEVBRD4
#undef USE_MMOD_ATP_ADAPTER

#define TFT_CS 10 // AD_B0_02
#define TFT_DC 25 // AD_B0_03
#define TFT_RST 24

#elif defined(ARDUINO_TEENSY41)
#undef USE_MMOD_ATP_ADAPTER

// My T4.1 Camera board
#define TFT_DC 9
#define TFT_CS 7
#define TFT_RST 8 

#elif defined(USE_MMOD_ATP_ADAPTER)
#define TFT_DC 4  // 0   // "TX1" on left side of Sparkfun ML Carrier
#define TFT_CS 5  // 4   // "CS" on left side of Sparkfun ML Carrier
#define TFT_RST 2 // 1  // "RX1" on left side of Sparkfun ML Carrier

#else
#define TFT_DC 0  // 20   // "TX1" on left side of Sparkfun ML Carrier
#define TFT_CS 4  // 5, 4   // "CS" on left side of Sparkfun ML Carrier
#define TFT_RST 1 // 2, 1  // "RX1" on left side of Sparkfun ML Carrier
#endif

/*******************
 * pxp values
 *******************/
uint8_t bytesperpixel = 2;
float ScaleFact = 1.5f; //0.9 for littlejoe and ida

/* timing */
uint32_t capture_time = 0;
uint32_t pxp_time = 0;
uint32_t display_time = 0;
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

#if !defined(ARDUINO_TEENSY_DEVBRD4)
uint16_t s_fb[(480) * 320] __attribute__((aligned(64)));
DMAMEM uint16_t d_fb[(480) * 320] __attribute__ ((aligned (64)));
const uint32_t sizeof_s_fb = sizeof(s_fb);
const uint32_t sizeof_d_fb = sizeof(d_fb);
#else
uint16_t *s_fb = nullptr;
uint16_t *d_fb = nullptr;
uint32_t sizeof_s_fb = 0;
uint32_t sizeof_d_fb = 0;
#endif


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

  #if defined(ARDUINO_TEENSY_DEVBRD4)
    // we need to allocate bufers
    // if (!sdram.begin()) {
    //  Serial.printf("SDRAM Init Failed!!!\n");
    //  while (1)
    //    ;
    //};
    s_fb = (uint16_t *)((((uint32_t)(sdram_malloc(tft.width() * tft.height()  * sizeof(uint16_t) + 32)) + 32) & 0xffffffe0));
    d_fb = (uint16_t *)((((uint32_t)(sdram_malloc(tft.width() * tft.height() * sizeof(uint16_t) + 32)) + 32) & 0xffffffe0));
    sizeof_s_fb = sizeof_d_fb = tft.width() * tft.height() * 2;
    Serial.printf("Buffers: %p %p\n", s_fb, d_fb);
#endif

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
        Serial.println("Rotation 0:");
        run_pxp(0, false, 0.0f);
        break;
      case '1':
        Serial.println("Rotation 1:");
        run_pxp(1, false, 0.0f);
        break;
      case '2':
        Serial.println("Rotation 2:");
        run_pxp(2, false, 0.0f);
        break;
      case '3':
        Serial.println("Rotation 3:");
        run_pxp(3, false, 0.0f);
        break;
      case '4':
        Serial.println("Rotation 0 w/flip:");
        run_pxp(0, true, 0.0f);
        break;
      case '5':
        Serial.println("Rotation 2 w/scaling:");
        run_pxp(2, false, ScaleFact);
        start_pxp();
        break;
      case '6':
        Serial.println("Rotation 3 w/Scaling:");
        run_pxp(3, false, ScaleFact);
        start_pxp();
        break;
      case 'f':
        tft.setRotation(0);
        capture_frame(false);
        draw_frame(image_width, image_height, s_fb);
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

void run_pxp(uint8_t rotation, bool flip, float scaling){
  capture_time = millis();
  capture_frame(false);
  Serial.printf("Capture time (millis): %d, ", millis()-capture_time);

  pxp_time = micros();
  PXP_ps_output(tft.width(), tft.height(),      /* Display width and height */
                image_width, image_height,      /* Image width and height */
                s_fb, PXP_RGB565, 2, 0,         /* Input buffer configuration */
                d_fb, PXP_RGB565, 2, 0,         /* Output buffer configuration */ 
                rotation, flip, scaling,        /* Rotation, flip, scaling */
                &outputWidth, &outputHeight);   /* Frame Out size for drawing */
  Serial.printf("PXP time(micros) : %d, ", micros()-pxp_time);

  display_time = millis();
  draw_frame(outputWidth, outputHeight, d_fb);
  Serial.printf("Display time: %d\n", millis()-display_time);

}

void start_pxp() {
  PXP_init();

  memset(s_fb, 0, sizeof_s_fb);
  memset(d_fb, 0, sizeof_d_fb);

  pxpStarted = true;
}

void draw_frame(uint16_t width, uint16_t height, const uint16_t *buffer) {
  tft.fillScreen(TFT_BLACK);
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