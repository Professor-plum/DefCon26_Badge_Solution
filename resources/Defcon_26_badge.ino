// Defcon 26 badge emulator
// Written by Glenn Anderson
// Badge Bus is documented here https://github.com/Wireb/badge_bus/wiki
// Written for SparkFun Pro Micro 5V, Pin 0 TX, Pin 1 RX, Pin 2 GND

unsigned readState;
unsigned long lastTransmissionTime;

void setup() {
  // put your setup code here, to run once:
  while (!Serial)
    ;
  Serial.begin(115200);
  Serial.println("Hello World");
  Serial1.begin(9600, SERIAL_8N1);
  pinMode(0, INPUT_PULLUP);
  readState = 0;
  lastTransmissionTime = micros();
}

//FIXME: checksum code isn't working, but checksums are optional, just don't forget to clear the checksum bit in header flags
#define POLY 0x1021

uint16_t checksumByte(uint16_t crc, uint8_t data) {
  uint8_t i;

  crc = crc ^ ((uint16_t)data << 8);
  for (i = 0; i < 8; i++) {
    if (crc & 0x8000)
      crc = (crc << 1) ^ POLY;
    else
      crc <<= 1;
  }
  return crc;
}

static uint8_t dataBuffer[255 + 7];
static uint16_t dataCheck;

static const uint8_t requestAF[] = {0x55, 0x55, 0x42, 0x42, 0x80, 0xAF, 0x00, 0x11, 0xCB};
//static const uint8_t requestAF[] = {0x55, 0x55, 0x42, 0x42, 0x00, 0xAF, 31, 0x2B, 0xFC, 0x8E, 0x2B, 0x35, 0x61, 0xC0, 0x4F, 0xBB, 0xC7, 0x3F,
//    0xA4, 0x3D, 0x5D, 0x96, 0x54, 0x0D, 0x0A, 0xA0, 0x08, 0xB3, 0x09, 0x24, 0xCE, 0x47, 0xDA, 0x0E, 0xC6, 0x75, 0x30, 0xD3, 0, 0};

//static uint8_t requestAF[] = {0x55, 0x55, 0x42, 0x42, 0x00, 0xAF, 0x00, 0, 0};

//This is a response from a Human badge, N is red
//static const uint8_t responseAF[] = {0x55, 0x55, 0x42, 0x42, 0xC0, 0xAF, 0x05, 0x00, 0x01, 0x00, 0x01, 0x00, 0xF3, 0xE4};
//Connecting to other badges, badge type seems to be 1 through 8 in the 4th data byte, N is green is a 1 in the 5th data byte
static uint8_t responseAF[] = {0x55, 0x55, 0x42, 0x42, 0x40, 0xAF, 0x05, 0x00, 0x01, 0x00, 0x01, 0x01, 0, 0};
//static const uint8_t responseAF[] = {0x55, 0x55, 0x42, 0x42, 0x40, 0xAF, 8, 0xFE, 0xED, 0xB0, 0xB0, 0xDE, 0xAD, 0xBE, 0xEF, 0, 0};

void loop() {
  if (Serial1.available() > 0) {
    uint8_t dataByte = Serial1.read();
    dataBuffer[readState] = dataByte;
    if (readState == 0) {
      if (dataByte == 0x42) {
        readState = 1;
      }
    } else if (readState == 1) {
      if (dataByte == 0x42) {
        readState = 2;
        dataCheck = checksumByte(0x1D0F, dataByte);
        dataCheck = checksumByte(dataCheck, dataByte);
      } else {
        readState = 0;
      }
    } else {
        readState++; 
        dataCheck = checksumByte(dataCheck, dataByte);
        if (readState >= 5 && readState >= (dataBuffer[4] + 7)) {
          //Process packet
          Serial.print("packet:");
          for (unsigned i = 2; i < readState - 2;i++) {
            Serial.print(" ");
            Serial.print(dataBuffer[i], HEX);
          }
          Serial.print(":");
          Serial.print((dataBuffer[readState - 2] << 8) | dataBuffer[readState - 1], HEX);
          Serial.print(" ");
          Serial.println(dataCheck, HEX);
          if (dataBuffer[2] == 0x80 && dataBuffer[3] == 0xAF) {
            Serial.print("Transmitting badge 0x");
            Serial.println(responseAF[10], HEX);
            Serial1.write(responseAF, sizeof(responseAF));
            responseAF[10]++;
            if (responseAF[10] > 8)
              responseAF[10] = 1;
          }
          //Reset state for next packet
          readState = 0;
        }
      }
  }
  unsigned long newTime = micros();
  if ((newTime - lastTransmissionTime) > 1000000) {
    Serial.print("Transmitting: ");
    Serial.println(requestAF[5], HEX);
    Serial1.write(requestAF, sizeof(requestAF));
    lastTransmissionTime = newTime;
  }
}

