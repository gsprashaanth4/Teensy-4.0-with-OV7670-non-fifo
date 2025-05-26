#include <Wire.h>
#define OV7670_ADDR 0x42

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

inline byte readPixel() {
  uint32_t gpioState = GPIO6_PSR;
  byte pixD = 0;
  pixD |= ((gpioState >> 3) & 0x01) << 0; // Pin 0
  pixD |= ((gpioState >> 18) & 0x01) << 1; // Pin 14
  pixD |= ((gpioState >> 19) & 0x01) << 2; // Pin 15
  pixD |= ((gpioState >> 22) & 0x01) << 3; // Pin 17
  pixD |= ((gpioState >> 23) & 0x01) << 4; // Pin 16
  pixD |= ((gpioState >> 24) & 0x01) << 5; // Pin 22
  pixD |= ((gpioState >> 25) & 0x01) << 6; // Pin 23
  pixD |= ((gpioState >> 26) & 0x01) << 7; // Pin 20
  return pixD;
}

// The writing to OV7670 register function
void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(OV7670_ADDR >> 1);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(2000000);
  delay(500);
  Wire.begin();

  // Setup pins
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
  digitalWrite(RESET, LOW);
  delay(500);

  // Generate ~8MHz XCLK
  analogWriteFrequency(XCLK_PIN, 30000000);
  analogWrite(XCLK_PIN, 128); // was 128
  delay(100);

  digitalWrite(RESET, HIGH);
  delay(100);

  writeReg(0x12, 0x80);  delay(100);// Reset to default values
  writeReg(0x11, 0x80);
  writeReg(0x3B, 0x0A);
  writeReg(0x3A, 0x04);
  writeReg(0x12, 0x04); // Output format: rgb
  writeReg(0x8C, 0x00); // Disable RGB444
  writeReg(0x40, 0xD0); // Set RGB565
  writeReg(0x17, 0x16);
  writeReg(0x18, 0x04);
  writeReg(0x32, 0x24);
  writeReg(0x19, 0x02);
  writeReg(0x1A, 0x7A);
  writeReg(0x03, 0x0A);
  writeReg(0x15, 0x02);
  writeReg(0x0C, 0x04);
  writeReg(0x3E, 0x1A); // Divide by 4
  writeReg(0x1E, 0x27);
  writeReg(0x72, 0x22); // Downsample by 4
  writeReg(0x73, 0xF2); // Divide by 4
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
  writeReg(0x6B, 0x0A);
  writeReg(0x6C, 0x0A);
  writeReg(0x6D, 0x55);
  writeReg(0x6E, 0x11);
  writeReg(0x6F, 0x9F);
  writeReg(0xB0, 0x84);
  // writeReg(0x55, 0x00); // Brightness
  // writeReg(0x56, 0x40); // Contrast
  writeReg(0x55, 0x00); // Brightness neutral
  writeReg(0x56, 0x40); // Contrast neutral
  writeReg(0xFF, 0xFF);  // End marker

  byte reg;
  Wire.beginTransmission(OV7670_ADDR);
  Wire.write(0x11);
  Wire.endTransmission();

  Wire.requestFrom(OV7670_ADDR, 1);
  reg = Wire.read();
  Wire.endTransmission();

  byte data = (reg & 0b1000000) | 0b00011110;
  Wire.beginTransmission(OV7670_ADDR);
  Wire.write(0x11);
  Wire.write(data);
  Wire.endTransmission();


  Wire.beginTransmission(OV7670_ADDR);
  Wire.write(0x6B);
  Wire.endTransmission();

  Wire.requestFrom(OV7670_ADDR, 1);
  reg = Wire.read();
  Wire.endTransmission();

  data = (reg & 0b00111111) | 0b10000000;
  Wire.beginTransmission(OV7670_ADDR);
  Wire.write(0x6B);
  Wire.write(data);
  Wire.endTransmission();

  delay(100);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(3000);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {

  int i = 0;
  byte img[160*120*2];

  
  while(digitalReadFast(VSYNC_PIN));

  while(!digitalReadFast(VSYNC_PIN));

  while(digitalReadFast(VSYNC_PIN)) {
    while(digitalReadFast(VSYNC_PIN) && !digitalReadFast(HREF_PIN));

    if(!digitalReadFast(VSYNC_PIN)) {
      break;
    }

    while(digitalReadFast(HREF_PIN) && i< 19200*2) {
      
      while(!digitalReadFast(PCLK_PIN));
      
      img[i++] = readPixel();

      while(digitalReadFast(PCLK_PIN));
      
      
      while(!digitalReadFast(PCLK_PIN));

      img[i++] = readPixel();

      while(digitalReadFast(PCLK_PIN));
    }
  }

  byte imgF[160*120];
  int j = 0;
  for(int i = 0; i<19200*2; i+=2) {
    uint16_t pixel = img[i] << 8 | img[i+1];
    int r = ( pixel >> 11 ) & 0x1F;
    int g = ( pixel >> 5 ) & 0x3F;
    int b = pixel & 0x1F;

    r = (r * 255) / 31;
    g = (g * 255) / 63;
    b = (b * 255) / 31;

    int gray = (int)(0.299f * r + 0.587f * g + 0.114f * b);
    gray = constrain(gray, 0, 255);
    imgF[j++] = (uint8_t)gray;
  }

  Serial.write(imgF, 19200);
}


