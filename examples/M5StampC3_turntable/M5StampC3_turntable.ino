#include <esp_now.h>
#include <WiFi.h>

//#define DEBUG

#define BtnA 3
#define PartyPin 10

#include <NeoPixelBus.h>

#define PixelPin 2
#define colorSaturation 64

const uint16_t PixelCount = 1;

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

// three element pixels, in different order and speeds
NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(PixelCount, PixelPin);

#include "Stepper_28BYJ_48.h"

// Stepper Pin
#define STP_IN1 4
#define STP_IN2 5
#define STP_IN3 6
#define STP_IN4 7

Stepper_28BYJ_48 stepper(STP_IN1, STP_IN2, STP_IN3, STP_IN4);

bool spinCWFlag = false;
bool spinCCWFlag = false;
bool spinCW180Flag = false;
bool spinCCW180Flag = false;
bool stepCWFlag = false;
bool stepCCWFlag = false;
bool spinSTOPFlag = false;
bool partyFlag = false;

/********************
 * ESPNow settings
 ********************/
esp_now_peer_info_t slave;

// command from Master
#define rNES_STOP 0
#define rNES_A 1
#define rNES_B 2
#define rNES_SELECT 3
#define rNES_START 4
#define rNES_UP 5
#define rNES_DOWN 6
#define rNES_LEFT 7
#define rNES_RIGHT 8

// Receive callback
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  #ifdef DEBUG
    Serial.printf("Last Packet Recv from: %s\n", macStr);
    Serial.printf("Last Packet Recv Data(%d): ", data_len);
    for ( int i = 0 ; i < data_len ; i++ ) {
      Serial.print(data[i]);
      Serial.print(" ");
    }
    Serial.println("");
  #endif

// Filtered by controller's Mac address
// Replace it with your controller's
  if((String)macStr == "70:04:1D:D4:A5:E0"){  // M5Dial
    if(data[0] == rNES_A){
      #ifdef DEBUG
        Serial.println("NES_A");
      #endif
      clearFlag();
      spinCWFlag = true;
    }else if(data[0] == rNES_B){
      #ifdef DEBUG
        Serial.println("NES_B");
      #endif
      clearFlag();
      spinCCWFlag = true;
    }else if(data[0] == rNES_SELECT){
      #ifdef DEBUG
        Serial.println("NES_SELECT");
      #endif
      partyFlag = true;
    }else if(data[0] == rNES_START){
      #ifdef DEBUG
        Serial.println("NES_START");
      #endif
      clearFlag();
      spinSTOPFlag = true;
      partyFlag = false;
    }else if(data[0] == rNES_UP){
      #ifdef DEBUG
        Serial.println("NES_UP");
      #endif
      clearFlag();
      spinCW180Flag = true;
    }else if(data[0] == rNES_DOWN){
      #ifdef DEBUG
        Serial.println("NES_DOWN");
      #endif
      clearFlag();
      spinCCW180Flag = true;
    }else if(data[0] == rNES_LEFT){
      #ifdef DEBUG
        Serial.println("NES_LEFT");
      #endif
      clearFlag();
      stepCCWFlag = true;
    }else if(data[0] == rNES_RIGHT){
      #ifdef DEBUG
        Serial.println("NES_RIGHT");
      #endif
      clearFlag();
      stepCWFlag = true;
    }else if(data[0] == rNES_STOP){
      #ifdef DEBUG
        Serial.println("NES_STOP");
      #endif
    }else{
      #ifdef DEBUG
        Serial.println("Unknown");
      #endif
    }
  }
}

void clearFlag(){
  spinCWFlag = false;
  spinCCWFlag = false;
  spinCW180Flag = false;
  spinCCW180Flag = false;
  stepCWFlag = false;
  stepCCWFlag = false;
  spinSTOPFlag = false;
}

/************************************
 * Arduino block
 ************************************/
void setup() {
  Serial.begin(115200);
  // this resets all the neopixels to an off state
  strip.Begin();
  strip.SetPixelColor(0, red);
  strip.Show();

  // ESP-NOW initialize
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  } else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }

// ESP-NOW callback
  esp_now_register_recv_cb(OnDataRecv);

  pinMode(BtnA, INPUT_PULLUP);
  pinMode(PartyPin, OUTPUT);
  digitalWrite(PartyPin, LOW);

  strip.SetPixelColor(0, blue);
  strip.Show();
}

void loop(){

  if(digitalRead(BtnA) == LOW){
    spinCWFlag = true;
  }

  if(partyFlag == true){
    digitalWrite(PartyPin, HIGH);
  }else if(partyFlag == false){
    digitalWrite(PartyPin, LOW);
  }

  if(spinCWFlag == true){
//    strip.SetPixelColor(0, green);
//    strip.Show();
    clearFlag();
    spinCWFlag = true;
    stepper.step(1);
  }

  if(spinCCWFlag == true){
//    strip.SetPixelColor(0, blue);
//    strip.Show();
    clearFlag();
    spinCCWFlag = true;
    stepper.step(-1);
  }

  if(stepCWFlag == true){
//    strip.SetPixelColor(0, green);
//    strip.Show();
    for(int i = 0; i < 8; i++){
      stepper.step(1);
      delay(10);
    }
    clearFlag();
  }

  if(stepCCWFlag == true){
//    strip.SetPixelColor(0, blue);
//    strip.Show();
    for(int i = 0; i < 8; i++){
      stepper.step(-1);
      delay(10);
    }
    clearFlag();
  }

  if(spinCW180Flag == true){
//    strip.SetPixelColor(0, green);
//    strip.Show();
    stepper.step(256);
    clearFlag();
  }

  if(spinCCW180Flag == true){
//    strip.SetPixelColor(0, blue);
//    strip.Show();
    stepper.step(-256);
    clearFlag();
  }


  if(spinSTOPFlag == true){
//    strip.SetPixelColor(0, black);
//    strip.Show();
    clearFlag();
  }
}
