//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//

// Wire Library for SCCB communication
#include <Wire.h>
#include <climits>

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//

// Address of the OV7670 camera module, may change based on manufacturer, try alternate 0x21
#define OV7670_ADDR 0x42

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//

// Pin definitions
#define VSYNC_PIN 4
#define HREF_PIN 5
#define PCLK_PIN 6
#define XCLK_PIN 3

#define D7_PIN 20
#define D6_PIN 23
#define D5_PIN 22
#define D4_PIN 16
#define D3_PIN 17
#define D2_PIN 15
#define D1_PIN 14
#define D0_PIN 0

#define RESET 21

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//

// Variable definitions
volatile bool frameAvailable = false;
volatile bool frameCapture = false;


// Format settings
const int windoRow = 60; // upto 120
const int windoCol = 60; // upto 160
const int startRow = (120-windoRow)/2;
const int startCol = (160-windoCol)/2;

byte imgF[windoRow*windoCol];

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//

// High-speed function to read the data registers
// The pins are wired to GPIO6, enabling simultaenous access of the data pins
// The wiring below is with a Teensy 4.0
inline byte readPixel() {
  uint32_t gpioState = GPIO6_PSR;
  byte pixD = 0;
  pixD |= ((gpioState >> 3) & 0x01) << 0;   // Pin 0
  pixD |= ((gpioState >> 18) & 0x01) << 1;  // Pin 14
  pixD |= ((gpioState >> 19) & 0x01) << 2;  // Pin 15
  pixD |= ((gpioState >> 22) & 0x01) << 3;  // Pin 17
  pixD |= ((gpioState >> 23) & 0x01) << 4;  // Pin 16
  pixD |= ((gpioState >> 24) & 0x01) << 5;  // Pin 22
  pixD |= ((gpioState >> 25) & 0x01) << 6;  // Pin 23
  pixD |= ((gpioState >> 26) & 0x01) << 7;  // Pin 20
  return pixD;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//

// The function to write the SCCB settings of the camera
void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(OV7670_ADDR >> 1);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//

void setupCamera() {
  writeReg(0x12, 0x80);  // Reset to default values
  delay(100);
  
  writeReg(0x6B, 0x80);
  writeReg(0x11, 0x00);
  
  writeReg(0x3B, 0x0A);
  writeReg(0x3A, 0x04);
  writeReg(0x12, 0x04);  // Output format: rgb
  writeReg(0x8C, 0x00);  // Disable RGB444
  writeReg(0x40, 0xD0);  // Set RGB565

  writeReg(0x17, 0x16);
  writeReg(0x18, 0x04);
  writeReg(0x32, 0x24);
  writeReg(0x19, 0x02);
  writeReg(0x1A, 0x7A);
  writeReg(0x03, 0x0A);
  writeReg(0x15, 0x02);
  writeReg(0x0C, 0x04);
  writeReg(0x3E, 0x1A);  // Divide by 4
  writeReg(0x1E, 0x27);
  writeReg(0x72, 0x22);  // Downsample by 4
  writeReg(0x73, 0xF2);  // Divide by 4

  writeReg(0x4F, 0x80);
  writeReg(0x50, 0x80);
  writeReg(0x51, 0x00);
  writeReg(0x52, 0x22);
  writeReg(0x53, 0x5E);
  writeReg(0x54, 0x80);
  writeReg(0x56, 0x40);
  writeReg(0x58, 0x9E);
  writeReg(0x59, 0x88);
  writeReg(0x5A, 0x88);
  writeReg(0x5B, 0x44);
  writeReg(0x5C, 0x67);
  writeReg(0x5D, 0x49);
  writeReg(0x5E, 0x0E);
  writeReg(0x69, 0x00);
  writeReg(0x6A, 0x40);

  writeReg(0x6C, 0x0A);
  writeReg(0x6D, 0x55);
  writeReg(0x6E, 0x11);
  writeReg(0x6F, 0x9F);
  writeReg(0xB0, 0x84);
  writeReg(0x55, 0x00);  // Brightness
  writeReg(0x56, 0x40);  // Contrast
  writeReg(0xFF, 0xFF);  // End marker
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//

void setup() {

  // Serial baud rate
  Serial.begin(2000000);
  delay(500);
  Wire.begin();

  //////////////////////////////////////////////////////////////////////////////////////////////

  // Pin configurations
  pinMode(PCLK_PIN, INPUT);
  pinMode(VSYNC_PIN, INPUT);
  pinMode(HREF_PIN, INPUT);

  pinMode(D0_PIN, INPUT);
  pinMode(D1_PIN, INPUT);
  pinMode(D2_PIN, INPUT);
  pinMode(D3_PIN, INPUT);
  pinMode(D4_PIN, INPUT);
  pinMode(D5_PIN, INPUT);
  pinMode(D6_PIN, INPUT);
  pinMode(D7_PIN, INPUT);

  pinMode(RESET, OUTPUT);

  //////////////////////////////////////////////////////////////////////////////////////////////

  // Enabling the camera reset
  digitalWrite(RESET, LOW);
  delay(500);

  // The external clock supplied to the OV7670
  // should support upto 48MHz, based on the wiring
  // advised upto 8MHz on breadboards
  analogWriteFrequency(XCLK_PIN, 10000000);
  analogWrite(XCLK_PIN, 128);  // 50% duty cycle
  delay(100);

  // Releasing reset on the camera
  digitalWrite(RESET, HIGH);
  delay(100);

  //////////////////////////////////////////////////////////////////////////////////////////////

  setupCamera();

  //////////////////////////////////////////////////////////////////////////////////////////////

  // Interrupt to indicate the start of a new frame
  attachInterrupt(digitalPinToInterrupt(VSYNC_PIN), doThis, RISING);
  delay(100);

  //////////////////////////////////////////////////////////////////////////////////////////////

  // showTime.exe
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(3000);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  //////////////////////////////////////////////////////////////////////////////////////////////

  // the marker that indicates the start of a new frame
  // and enters capture mode if currently nothing is being captured

  if (!frameAvailable || frameCapture) return;
  frameCapture = true;
  int i = 0;

  // RGB image buffer
  uint16_t img[19200];

  // clock start for FPS calculations
  unsigned long time0 = micros();

  //////////////////////////////////////////////////////////////////////////////////////////////

  // The main working principle is that the VSYNC pind goes HIGH
  // at the start of a new frame, HREF goes HIGH in the start of a new Line
  // and PCLK goes HIGH when D0-D7 are valid and hold the pixel data
  // Here, when capturing in RGB565, each pixel is 2-Bytes long,
  // Hence each pixel takes two PCLK cycles

  while (digitalReadFast(VSYNC_PIN)) {
    while (!digitalReadFast(HREF_PIN)) {
      if (!digitalReadFast(VSYNC_PIN)) break;
    }

    while (digitalReadFast(HREF_PIN)) 
    {
      if (!digitalReadFast(VSYNC_PIN) || i >= 19200) break;

      while (!digitalReadFast(PCLK_PIN));

      img[i] = (readPixel()) << 8;
      while (digitalReadFast(PCLK_PIN));

      while (!digitalReadFast(PCLK_PIN));

      img[i++] |= readPixel();
      while (digitalReadFast(PCLK_PIN));
      
    }
  }

  //////////////////////////////////////////////////////////////////////////////////////////////
  
  unsigned long time1 = micros();

  unsigned long time = time1-time0;
  float fps = 1000000.0f / (float)(time); // FPS calculated

  // SoftWare-Subscaling paraeters
  int k = 0;
  int startRow = (120-windoRow)/2;
  int startCol = (160-windoCol)/2;

  // this function caters to further software downscaling if required
  for(int row  = startRow; row < (startRow+windoRow); row++) {
    for (int col = startCol; col < (startCol+windoCol); col++) {
      int index = (row * 160 + col);
      int r = ((img[index] >> 11) & 0x1F) * 255 / 31;
      int g = ((img[index] >> 5) & 0x3F) * 255 / 63;
      int b = (img[index] & 0x1F) * 255 / 31;
      int gray = (int)(0.299f * r + 0.587f * g + 0.114f * b);
      imgF[k++] = (uint8_t)(constrain(gray, 0, 255));
    }
  }

  //////////////////////////////////////////////////////////////////////////////////////////////

  /* Eaither Print the FPS or Send out the image data */
  Serial.write(imgF, windoRow*windoCol);
  // Serial.println(fps, 10);

  // Reset to default
  frameAvailable = false;
  frameCapture = false;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//

// THe interrupt function
inline void doThis() {
  frameAvailable = true;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//
