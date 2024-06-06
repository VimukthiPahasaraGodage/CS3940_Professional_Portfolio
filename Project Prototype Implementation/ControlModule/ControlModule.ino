#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include "time.h"

#define SERIAL "CM01"
#define MANUAL_OVERRIDE_PIN 32
#define CONTROL_PIN 33

#define DAYTIME_MANUAL_IND 18
#define MANUAL_OVERRIDE_IND 19
#define CONNECTION_IND 21

const char* ssid = "SLT FIBRE";
const char* password = "wT6IJlUQVvtfJUoYHHgT";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 3600;

bool human_present = false;
bool low_brightness = false;
bool lamp_state = false;

String manual_override = "false";
String manual_control_in_daytime = "false";
String log_message = "";

String web = "<!DOCTYPE html><html lang='en'><head> <meta charset='UTF-8'> <meta name='viewport' content='width=device-width, initial-scale=1.0'> <title>Smart Home Automation System</title></head><body> <button type='button' id='b3'>Calibrate</button> <h2>Manual Control in Day Time</h2> <button type='button' id='b1'>OFF</button> <h2>Manual Override</h2> <button type='button' id='b2'>OFF</button> <h2>Logs</h2> <button type='button' id='b4'>Clear Logs</button> <p id='l1'></p></body><script> var Socket; var logs = ''; var b1_s = 'false'; var b2_s = 'false'; var b1 = document.getElementById('b1'); var b2 = document.getElementById('b2'); var b3 = document.getElementById('b3'); var b4 = document.getElementById('b4'); var l1 = document.getElementById('l1'); b1.addEventListener('click', b1_c); b2.addEventListener('click', b2_c); b3.addEventListener('click', b3_c); b4.addEventListener('click', b4_c); function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { proc_log(event); }; } function proc_log(event) { var obj = JSON.parse(event.data); console.log(obj); var serial = obj.Serial; if (serial == 'CM01'){ if (obj.B1_s != b1_s || obj.B2_s != b2_s){ Socket.send('6'); } b1_s = obj.B1_s; b2_s = obj.B2_s; if (b1_s == 'true'){ b1.innerHTML = 'ON'; }else if (b1_s == 'false'){ b1.innerHTML = 'OFF'; } if (b2_s == 'true'){ b2.innerHTML = 'ON'; }else if (b2_s == 'false'){ b2.innerHTML = 'OFF'; } if (obj.Log != ''){ logs += obj.Log + '<br />'; } l1.innerHTML = logs; } } function b1_c() { if (b1_s == 'false'){ Socket.send('2'); }else if (b1_s == 'true'){ Socket.send('1'); } } function b2_c() { if (b2_s == 'false'){ Socket.send('4'); }else if (b2_s == 'true'){ Socket.send('3'); } } function b3_c() { Socket.send('5'); } function b4_c() { logs = ''; l1.innerHTML = logs; } window.onload = function(event) { init(); } </script></html>";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void con_ind(bool status){
  if (status){
    digitalWrite(CONNECTION_IND, HIGH);
  }else{
    digitalWrite(CONNECTION_IND, LOW);
  }
}

void day_manual_ind(bool status){
  if (status){
    digitalWrite(DAYTIME_MANUAL_IND, HIGH);
  }else{
    digitalWrite(DAYTIME_MANUAL_IND, LOW);
  }
}

void manual_ind(bool status){
  if (status){
    digitalWrite(MANUAL_OVERRIDE_IND, HIGH);
  }else{
    digitalWrite(MANUAL_OVERRIDE_IND, LOW);
  }
}

void setup() {
  pinMode(MANUAL_OVERRIDE_PIN, OUTPUT);
  pinMode(CONTROL_PIN, OUTPUT);
  pinMode(DAYTIME_MANUAL_IND, OUTPUT);
  pinMode(MANUAL_OVERRIDE_IND, OUTPUT);
  pinMode(CONNECTION_IND, OUTPUT);

  digitalWrite(MANUAL_OVERRIDE_PIN, LOW);
  digitalWrite(CONTROL_PIN, LOW);
  digitalWrite(DAYTIME_MANUAL_IND, LOW);
  digitalWrite(MANUAL_OVERRIDE_IND, LOW);
  digitalWrite(CONNECTION_IND, LOW);

  Serial.begin(115200);

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

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  server.on("/", []() {
    server.send(200, "text\html", web);
  });
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}


void loop() {
  server.handleClient();
  webSocket.loop();
}


bool isEvening(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain the time.");
    return true;
  }
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  String time_hour_str = String(timeHour);
  Serial.println("Hour of day: " + time_hour_str);
  int hour_of_day = time_hour_str.toInt();
  if (hour_of_day >= 18){
    return true;
  }else{
    return false;
  }
}


void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      con_ind(false);
      Serial.println("Web Socket Disconnected");
      break;
    case WStype_CONNECTED:
      con_ind(true);
      Serial.println("Web Socket Connected");
      update_webpage();         
      break;
    case WStype_TEXT:
      String payload_str = String((char*)payload);
      Serial.println("Payload: " + payload_str);
      if (payload[0] == '1') {
        manual_control_in_daytime = "false";
        day_manual_ind(false);
        log_message = "Manual control in daytime disabled";              
      }
      if (payload[0] == '2') {
        manual_control_in_daytime = "true";   
        day_manual_ind(true);
        log_message = "Manual control in daytime enabled";                  
      }
      if (payload[0] == '3') {
        manual_override = "false";
        digitalWrite(MANUAL_OVERRIDE_PIN, LOW);
        digitalWrite(CONTROL_PIN, LOW);
        manual_ind(false);
        log_message = "Manual override disabled";         
      }
      if (payload[0] == '4') {
        manual_override = "true"; 
        digitalWrite(MANUAL_OVERRIDE_PIN, HIGH);
        digitalWrite(CONTROL_PIN, LOW);
        manual_ind(true);
        log_message = "Manual override enabled";                  
      }
      if (payload[0] == '1' || payload[0] == '2' || payload[0] == '3' || payload[0] == '4'){
        Serial.println("Manual control in daytime: " + manual_control_in_daytime + ", Manual override: " + manual_override);
        update_webpage();
      }

      if (payload[0] == '5'){
        webSocket.broadcastTXT("5");
      }
      if (payload[0] == '6'){
        webSocket.broadcastTXT("6");
      }

      StaticJsonDocument<200> doc;
      deserializeJson(doc, payload_str);
      if (doc["Serial"] == "SM01"){
        String operation = doc["Operation"];
        if (operation == "DATA"){
          String human = doc["Human"];
          if (human == "true"){
            human_present = true;
          }else if (human == "false"){
            human_present = false;
          }

          String brightness = doc["Low_Brightness"];
          if (brightness == "true"){
            low_brightness = true;
          }else if (brightness == "false"){
            low_brightness = false;
          }
          log_message = String(doc["Log"]);
          update_webpage();
          automate();
        }else if (operation == "CALIBRATE"){
          uint8_t progress = uint8_t(doc["Progress"]);
          log_message = String(doc["Log"]);
          update_webpage();
        }
      }
      break;
  }
}


void update_webpage()
{
  StaticJsonDocument<100> doc;

  String jsonString = "";

  JsonObject object = doc.to<JsonObject>();
  object["Serial"] = SERIAL;
  object["B1_s"] = manual_control_in_daytime;
  object["B2_s"] = manual_override;
  object["Log"] = log_message;
  serializeJson(doc, jsonString);
  Serial.println("Web update JSON: " + jsonString);
  webSocket.broadcastTXT(jsonString);
}

void automate(){
  bool new_lamp_state = false;
  if (manual_override == "false"){
    if (human_present && low_brightness){
      if (manual_control_in_daytime == "true"){
        if (isEvening()){
          digitalWrite(CONTROL_PIN, HIGH);
          new_lamp_state = true;
          log_message = "Lamp is ON";
        }
      }else if (manual_control_in_daytime == "false"){
        digitalWrite(CONTROL_PIN, HIGH);
        new_lamp_state = true;
        log_message = "Lamp is ON";
      }
    }else{
      digitalWrite(CONTROL_PIN, LOW);
      new_lamp_state = false;
      log_message = "Lamp is OFF";
    }

    if (new_lamp_state != lamp_state){
      lamp_state = new_lamp_state;
      update_webpage();
    }
  }
}
