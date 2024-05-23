#include "Teensy_Camera.h"
#include "T4_PXP.h"

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

#include "Teensy_HM0360/HM0360.h"
HM0360 himax;
Camera camera(himax);
#define CameraID 0x0360
#define SCREEN_ROTATION 0
#define MIRROR_FLIP_CAMERA

framesize_t camera_framesize = FRAMESIZE_QVGA;
bool useGPIO = false;

/************************************************/

#if !defined(ARDUINO_TEENSY_DEVBRD4)
uint8_t s_fb[(480) * 320] __attribute__((aligned(64)));
DMAMEM uint16_t d_fb[(480) * 320] __attribute__ ((aligned (64)));
const uint32_t sizeof_s_fb = sizeof(s_fb);
const uint32_t sizeof_d_fb = sizeof(d_fb);
#else
uint8_t *s_fb = nullptr;
uint16_t *d_fb = nullptr;
uint32_t sizeof_s_fb = 0;
uint32_t sizeof_d_fb = 0;
#endif

/************************************************/

uint16_t FRAME_HEIGHT, FRAME_WIDTH;
bool pxpStarted = 0;
uint16_t outputWidth, outputHeight;


/* timing */
uint32_t capture_time = 0;
uint32_t pxp_time = 0;
uint32_t display_time = 0;
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


#if defined(ARDUINO_TEENSY_DEVBRD4)
    // we need to allocate bufers
    // if (!sdram.begin()) {
    //  Serial.printf("SDRAM Init Failed!!!\n");
    //  while (1)
    //    ;
    //};
    s_fb = (uint8_t *)((((uint32_t)(sdram_malloc(tft.width() * tft.height()  * sizeof(uint8_t) + 32)) + 32) & 0xffffffe0));
    d_fb = (uint16_t *)((((uint32_t)(sdram_malloc(tft.width() * tft.height() * sizeof(uint16_t) + 32)) + 32) & 0xffffffe0));
    sizeof_s_fb = sizeof_d_fb = tft.width() * tft.height() * 2;
    Serial.printf("Buffers: %p %p\n", s_fb, d_fb);
#endif

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
        run_pxp(0, false, 0.0f);
        break;
      case '1':
        Serial.println(" PXP rotation 1....");
        run_pxp(1, false, 0.0f);
        break;
      case '2':
        Serial.println(" PXP rotation 2....");
        run_pxp(2, false, 0.0f);
        break;
      case '3':
        Serial.println(" PXP rotation 3....");
        run_pxp(3, false, 0.0f);
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
  tft.fillScreen(TFT_BLACK);
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
  #if defined(ARDUINO_TEENSY_DEVBRD4)
  camera.setPins(7, 8, 21, 46, 23, 40, 41, 42, 43, 44, 45, 6, 9);
  #endif
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
   camera.setVSyncISRPriority(102); // higher priority than default
  //camera.setDMACompleteISRPriority(192); // lower than default

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

void run_pxp(uint8_t rotation, uint8_t flip, float scaling){
        capture_time = millis();
        capture_frame(false);
        Serial.printf("Capture time (millis): %d, ", millis()-capture_time);

        pxp_time = micros();
        PXP_ps_output(tft.width(), tft.height(),      /* Display width and height */
                      FRAME_WIDTH, FRAME_HEIGHT,      /* Image width and height */
                      s_fb, PXP_Y8, 1, 0,             /* Input buffer configuration */
                      d_fb, PXP_RGB565, 2, 0,         /* Output buffer configuration */ 
                      rotation, flip, scaling,        /* Rotation, flip, scaling */
                      &outputWidth, &outputHeight);   /* Frame Out size for drawing */
        Serial.printf("PXP time(micros) : %d, ", micros()-pxp_time);

        display_time = millis();
        draw_frame(outputWidth, outputHeight, d_fb);
        Serial.printf("Display time: %d\n", millis()-display_time);

}