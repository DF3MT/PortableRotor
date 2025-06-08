#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ezButton.h>

#define VRX_PIN  35
#define VRY_PIN  34 
#define SW_PIN   33
// & 3.3V    & GND
ezButton button(SW_PIN);
#define LED_PIN  12

int xValue = 0;
int yValue = 0;
int xCal = 0;
int yCal = 0;
int bValue = 0; 
int flag = 0;

// REPLACE WITH YOUR RECEIVER MAC Address, find out with get_mac_address.ino
uint8_t broadcastAddress[] = {0x34,0x85,0x18,0x26,0xD7,0x34};  //   Rotor Device
//uint8_t broadcastAddress[] = {0xC0,0x5D,0x89,0xAC,0x7C,0xC0};  //c0:5d:89:ac:7c:c0 This Device => Remote Device


// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  int xVal;
  int yVal;
  int bVal;
} struct_message;

// Create a struct_message called myData & peer
struct_message myData;
esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  status == ESP_NOW_SEND_SUCCESS ? digitalWrite(LED_PIN, HIGH) : digitalWrite(LED_PIN, LOW);
}
 
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  button.setDebounceTime(100);
  
  pinMode(VRX_PIN, INPUT);
  pinMode(VRY_PIN, INPUT);
  pinMode(SW_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  xValue = analogRead(VRX_PIN);
  yValue = analogRead(VRY_PIN);
  
  xValue = map(xValue, 0, 4095, -128, 128);
  yValue = map(yValue, 0, 4095, -128, 128);
  
  xCal = xValue*-1;
  yCal = yValue*-1;
}
 
void loop() {
  bool send = false;
  // Make values to send
  button.loop(); // MUST call the loop() function first

  // read analog X and Y analog values
  xValue = analogRead(VRX_PIN);
  yValue = analogRead(VRY_PIN);

  myData.xVal = map(xValue, 0, 4095, -128, 128) + xCal;
  myData.yVal = map(yValue, 0, 4095, -128, 128) + yCal;

  if(myData.xVal > 5 || myData.xVal < -5){
    send = true;
  }

  if(myData.yVal > 5 || myData.yVal < -5){
    send = true;
  }

  // Read the button value
  bValue = button.getState();
  delay(20);
  myData.bVal = bValue;

  //Serial.print(myData.xVal);Serial.print("  ");Serial.print(myData.yVal);Serial.print("  ");Serial.println(myData.bVal);
  
  if (button.isPressed()) {
    delay(20);
    Serial.println("The button is pressed");
    send = true;
  }

  if (button.isReleased()) {
    Serial.println("The button is released");
    send = true;
  }

  if(send == true){
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
    Serial.print(myData.xVal);Serial.print("  ");Serial.print(myData.yVal);Serial.print("  ");Serial.println(myData.bVal);
  }
  delay(500);  
}
