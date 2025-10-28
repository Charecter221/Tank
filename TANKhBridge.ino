#include <Usb.h>
#include <hiduniversal.h>
#include <usbhid.h>

USB Usb;
HIDUniversal Hid(&Usb);

// Motor control pins (no PWM)
const int leftEnable   = 2;  // ENA
const int leftIn1      = 3;  // IN1
const int leftIn2      = 4;  // IN2
const int rightEnable  = 5;  // ENB
const int rightIn3     = 6;  // IN3
const int rightIn4     = 7;  // IN4

// Feeder motor pin
const int feederPin    = 8;

uint8_t lastBuf[32];
unsigned long lastPrintTime = 0;

class MyParser : public HIDReportParser {
public:
  void Parse(USBHID* hid, bool is_rpt_id, uint8_t len, uint8_t* buf) {
    bool changed = false;
    for (uint8_t i = 0; i < len; i++) {
      if (buf[i] != lastBuf[i]) {
        changed = true;
        break;
      }
    }
    if (!changed) return;
    memcpy(lastBuf, buf, len);

    // HID report dump every 500ms
    if (millis() - lastPrintTime > 500) {
      Serial.print("HID: ");
      for (uint8_t i = 0; i < len; i++) {
        Serial.print(buf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      lastPrintTime = millis();
    }

    // Movement triggers
    bool upActive     = (buf[9] == 0xFF);
    bool downActive   = (buf[10] == 0xFF);
    bool turnLeft     = (buf[8] == 0xFF);
    bool turnRight    = (buf[7] == 0xFF);
    bool feederBtn    = (buf[13] == 0xFF);  // X button

    // Movement logic
    if (turnLeft) {
      // Pivot left: left reverse, right forward
      digitalWrite(leftIn1, LOW);
      digitalWrite(leftIn2, HIGH);
      digitalWrite(rightIn3, HIGH);
      digitalWrite(rightIn4, LOW);
    } else if (turnRight) {
      // Pivot right: left forward, right reverse
      digitalWrite(leftIn1, HIGH);
      digitalWrite(leftIn2, LOW);
      digitalWrite(rightIn3, LOW);
      digitalWrite(rightIn4, HIGH);
    } else if (upActive) {
      // Forward: both motors forward
      digitalWrite(leftIn1, HIGH);
      digitalWrite(leftIn2, LOW);
      digitalWrite(rightIn3, HIGH);
      digitalWrite(rightIn4, LOW);
    } else if (downActive) {
      // Reverse: both motors reverse
      digitalWrite(leftIn1, LOW);
      digitalWrite(leftIn2, HIGH);
      digitalWrite(rightIn3, LOW);
      digitalWrite(rightIn4, HIGH);
    } else {
      // Stop motors
      digitalWrite(leftIn1, LOW);
      digitalWrite(leftIn2, LOW);
      digitalWrite(rightIn3, LOW);
      digitalWrite(rightIn4, LOW);
    }

    // Feeder motor logic
    digitalWrite(feederPin, feederBtn ? HIGH : LOW);
  }
};

MyParser parser;

void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(leftEnable, OUTPUT);
  pinMode(leftIn1, OUTPUT);
  pinMode(leftIn2, OUTPUT);
  pinMode(rightEnable, OUTPUT);
  pinMode(rightIn3, OUTPUT);
  pinMode(rightIn4, OUTPUT);

  // Feeder pin
  pinMode(feederPin, OUTPUT);
  digitalWrite(feederPin, LOW);  // Feeder OFF at boot

  // Enable motors (always HIGH since no PWM)
  digitalWrite(leftEnable, HIGH);
  digitalWrite(rightEnable, HIGH);

  // USB Host Shield
  Serial.println("Initializing USB Host Shield...");
  if (Usb.Init() == -1) {
    Serial.println("❌ USB Host Shield init failed");
    while (1);
  }
  Serial.println("✅ USB Host Shield initialized");
  Serial.println("Waiting for HID controller...");

  Hid.SetReportParser(0, &parser);
}

void loop() {
  Usb.Task();
}