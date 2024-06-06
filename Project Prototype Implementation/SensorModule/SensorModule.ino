#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Tsl2561.h>

#define SERIAL "SM01"
#define OP_DATA "DATA"
#define OP_CALIBRATE "CALIBRATE"

//HLK-LD2450
#define RXD 16
#define TXD 17
//TSL2561
#define SDA 33
#define SCL 32
//Indicators
#define HUMAN_PRESENCE_IND 18
#define LOW_BRIGHT_IND 19
#define CONNECTION_IND 21

const char* ssid = "SLT FIBRE";
const char* password = "wT6IJlUQVvtfJUoYHHgT";

typedef struct RadarTarget
{
    uint16_t id;
    int16_t x;
    int16_t y;
    int16_t speed;
    uint16_t resolution;
} RadarTarget_t;

RadarTarget_t nowTargets[3];

Tsl2561 Tsl(Wire);
WebSocketsClient webSocket;

String human_present = "false";
String low_brightness = "false";
uint16_t brightness_threshold = 70;

int readRadarData(byte rec_buf[], int len, RadarTarget targets[], uint16_t numTargets) {
    for (int i = 0; i < len; i++) {
        if (rec_buf[i] == 0xAA && rec_buf[i + 1] == 0xFF && rec_buf[i + 2] == 0x03 && rec_buf[i + 3] == 0x00 && rec_buf[i + 28] == 0x55 && rec_buf[i + 29] == 0xCC) { // 检查帧头和帧尾
            int index = i + 4;

            for (int targetCounter = 0; targetCounter < numTargets; targetCounter++) {
                if (index + 7 < len) {
                    RadarTarget target;
                    target.x = (int16_t)(rec_buf[index] | (rec_buf[index + 1] << 8));
                    target.y = (int16_t)(rec_buf[index + 2] | (rec_buf[index + 3] << 8));
                    target.speed = (int16_t)(rec_buf[index + 4] | (rec_buf[index + 5] << 8));
                    target.resolution = (uint16_t)(rec_buf[index + 6] | (rec_buf[index + 7] << 8));

                    if (rec_buf[index + 1] & 0x80) target.x -= 0x8000;
                    else target.x = -target.x;
                    if (rec_buf[index + 3] & 0x80) target.y -= 0x8000;
                    else target.y = -target.y;
                    if (rec_buf[index + 5] & 0x80) target.speed -= 0x8000;
                    else target.speed = -target.speed;

                    targets[targetCounter].id = targetCounter + 1;
                    targets[targetCounter].x = target.x;
                    targets[targetCounter].y = target.y;
                    targets[targetCounter].speed = target.speed;
                    targets[targetCounter].resolution = target.resolution;

                    index += 8;
                }
            }
            i = index;
        }
    }
    return 1;
}

void con_ind(bool status){
  if (status){
    digitalWrite(CONNECTION_IND, HIGH);
  }else{
    digitalWrite(CONNECTION_IND, LOW);
  }
}

void human_ind(bool status){
  if (status){
    digitalWrite(HUMAN_PRESENCE_IND, HIGH);
  }else{
    digitalWrite(HUMAN_PRESENCE_IND, LOW);
  }
}

void bright_ind(bool status){
  if (status){
    digitalWrite(LOW_BRIGHT_IND, HIGH);
  }else{
    digitalWrite(LOW_BRIGHT_IND, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(256000, SERIAL_8N1, RXD, TXD);
  Wire.begin(SDA, SCL);

  pinMode(HUMAN_PRESENCE_IND, OUTPUT);
  pinMode(LOW_BRIGHT_IND, OUTPUT);
  pinMode(CONNECTION_IND, OUTPUT);

  digitalWrite(HUMAN_PRESENCE_IND, LOW);
  digitalWrite(LOW_BRIGHT_IND, LOW);
  digitalWrite(CONNECTION_IND, LOW);

  Serial.print("Connecting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    con_ind(true);
    delay(250);
    con_ind(false);
    delay(250);
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  con_ind(true);

  webSocket.begin("192.168.1.25", 81, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}


void loop() {
  webSocket.loop();

  bool env_changed = false;

  if (Serial2.available() > 0) {
    byte rec_buf[256] = "";
    int len = Serial2.readBytes(rec_buf, sizeof(rec_buf));

    int radar_flag = readRadarData(rec_buf, len, nowTargets, 3);
    
    if ((nowTargets[0].x != 0 && nowTargets[0].y != 0) || (nowTargets[1].x != 0 && nowTargets[1].y != 0) || (nowTargets[2].x != 0 && nowTargets[2].y != 0)){
      if (human_present == "false"){
        human_present = "true";
        env_changed = true;
        human_ind(true);
        Serial.println("Exists");
      }
    }else{
      if (human_present == "true"){
        human_present = "false";
        env_changed = true;
        human_ind(false);
        Serial.println("Gone");
      }
    }
  }

  Tsl.begin();
  if( Tsl.available() ) {
    Tsl.on();

    Tsl.setSensitivity(true, Tsl2561::EXP_14);
    delay(16);

    uint16_t full;
    Tsl.fullLuminosity(full);

    if (full < brightness_threshold){
      if (low_brightness == "false"){
        low_brightness = "true";
        env_changed = true;
        bright_ind(true);
        Serial.println("Brightness " + String(full) + " , Switch the light ON");
      }
    }else{
      if (low_brightness == "true"){
        low_brightness = "false";
        env_changed = true;
        bright_ind(false);
        Serial.println("Brightness " + String(full)  + " , Switch the light OFF");
      }
    }

    Tsl.off();
  }

  if (env_changed){
    update_webpage(OP_DATA, 0);
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("Web Socket Disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("Web Socket Connected");        
      break;
    case WStype_TEXT:
      Serial.println("Payload: " + String((char*)payload));
      if (payload[0] == '5') {
        Serial.println("Calibration starting...");
        calibrate();       
      }
      if (payload[0] == '6') {
        update_webpage(OP_DATA, 0);
      }
      break;
  }
}


void calibrate(){
  int iterations = 0;
  int total_illuminance = 0;

  while (iterations < 10){
    Tsl.begin();
    if( Tsl.available() ) {
      Tsl.on();

      Tsl.setSensitivity(true, Tsl2561::EXP_14);
      delay(16);

      uint16_t full;
      Tsl.fullLuminosity(full);

      total_illuminance += full;
      iterations += 1;
      update_webpage(OP_CALIBRATE, iterations*10);

      Tsl.off();
    }
  }

  brightness_threshold = int((total_illuminance / 10) * 1.2);
  Serial.println("New brightness threshold: " + String(brightness_threshold));
}


void update_webpage(String operation, int progress)
{
  StaticJsonDocument<100> doc;

  String jsonString = "";

  JsonObject object = doc.to<JsonObject>();
  object["Serial"] = SERIAL;

  String log_message = "";
  if (operation == OP_DATA){
    object["Operation"] = OP_DATA;
    object["Human"] = human_present;
    object["Low_Brightness"] = low_brightness;

    if (human_present == "true"){
      log_message = "Human present. ";
    }else if (human_present == "false"){
      log_message = "Human not present. ";
    }

    if (low_brightness == "true"){
      log_message += "Low brightness in the space.";
    }else{
      log_message += "Sufficient brightness in the space.";
    }

    object["Log"] = log_message;
  }else if (operation == OP_CALIBRATE){
    object["Operation"] = OP_CALIBRATE;

    if (progress < 100){
      log_message = "Calibration in progress. Progress = " + String(progress) + "%";
    }else{
      log_message = "Calibration done.";
    }
    
    object["Log"] = log_message;
  }
  
  serializeJson(doc, jsonString);
  Serial.println("Web update JSON: " + jsonString);
  webSocket.sendTXT(jsonString);
}