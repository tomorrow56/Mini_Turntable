#include "M5Dial.h"
#include <esp_now.h>
#include <WiFi.h>

/* command from Master
#define rNES_STOP 0
#define rNES_A 1
#define rNES_B 2
#define rNES_SELECT 3
#define rNES_START 4
#define rNES_UP 5
#define rNES_DOWN 6
#define rNES_LEFT 7
#define rNES_RIGHT 8
*/

bool spinCWFlag = false;
bool spinCCWFlag = false;

/********************
 * ESPNow settings
 ********************/
esp_now_peer_info_t slave;
esp_err_t result;
boolean sending = false;

// Send Callback
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Receive Callback
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.printf("Last Packet Recv from: %s\n", macStr);
  Serial.printf("Last Packet Recv Data(%d): ", data_len);
  for ( int i = 0 ; i < data_len ; i++ ) {
    Serial.print(data[i]);
    Serial.print(" ");
  }
  Serial.println("");
}

void setup() {
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);
  M5Dial.Display.setTextColor(GREEN);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
  M5Dial.Display.setTextSize(2);

  Serial.println("M5Dial + ESP-NOW");

  // ESP-NOW initialize
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  } else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }

  // Slave Mac address
  // Replace it with your turntable's
  memset(&slave, 0, sizeof(slave));
  //TurnTable Mac Address
  slave.peer_addr[0] = (uint8_t)0x34;
  slave.peer_addr[1] = (uint8_t)0xB4;
  slave.peer_addr[2] = (uint8_t)0x72;
  slave.peer_addr[3] = (uint8_t)0x12;
  slave.peer_addr[4] = (uint8_t)0xA8;
  slave.peer_addr[5] = (uint8_t)0xF8;

  esp_err_t addStatus = esp_now_add_peer(&slave);
  if (addStatus == ESP_OK) {
    // Pair success
    Serial.println("Pair success");
  }

  // ESP-NOW Callback
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

long oldPosition = -999;

void loop() {
  uint8_t data[1] = {0};

  M5Dial.update();
  long newPosition = M5Dial.Encoder.read();

  if (newPosition != oldPosition) {
    M5Dial.Speaker.tone(8000, 20);
    M5Dial.Display.clear();
    if(spinCWFlag == false && spinCCWFlag == false){
      if(newPosition < oldPosition){
        data[0] = 8;
        result = esp_now_send(slave.peer_addr, data, sizeof(data));
        Serial.println("TURN CW");
      }else if(newPosition > oldPosition){
        data[0] = 7;
        result = esp_now_send(slave.peer_addr, data, sizeof(data));
        Serial.println("TURN CCW");
      }
      oldPosition = newPosition;
      Serial.println(newPosition);
      M5Dial.Display.drawString(String(newPosition), M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
    }

    if (result == ESP_OK) {
      Serial.println("Success");
    } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
      Serial.println("ESPNOW not Init.");
    } else if (result == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Invalid Argument");
    } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
      Serial.println("Internal Error");
    } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("ESP_ERR_ESPNOW_NO_MEM");
    } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
      Serial.println("Peer not found.");
    } else {
      Serial.println("Not sure what happened");
    }    
  }

  if (M5Dial.BtnA.wasPressed()) {
    if(spinCWFlag == false){
      data[0] = 2;
      result = esp_now_send(slave.peer_addr, data, sizeof(data));
      Serial.println("SPIN_CW");
      spinCWFlag = true;
      spinCCWFlag = false;
    }else if(spinCCWFlag == false){
      data[0] = 1;
      result = esp_now_send(slave.peer_addr, data, sizeof(data));
      Serial.println("SPIN_CCW");
      spinCWFlag = false;
      spinCCWFlag = true;
    }
    M5Dial.Encoder.readAndReset();
  }
  if (M5Dial.BtnA.pressedFor(2000)) {
      data[0] = 4;
      result = esp_now_send(slave.peer_addr, data, sizeof(data));
      Serial.println("STOP");
      spinCWFlag = false;
      spinCCWFlag = false;
//      M5Dial.Encoder.write(63);
  }

  delay(10);
}
