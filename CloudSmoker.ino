// CloudSmoker

// Board Generic ESP8266 Module
// Flash Size 512kB (32k SPIFFS)

#define PROJECT "SMOKER"
#define VERSION "0.2"

#define XOLED 1
#define XTHERMO 1
#define XSERIAL 1
#define XCLOUD 1

#define AUTO_TEMP -2
#define AUTO_TIME -1

const byte RELAY_PIN = 13; // D7

#if XTHERMO
#include <SPI.h>
#include "max6675.h"

const byte THERMO_SO = 12; // D6
const byte THERMO_CS = 15; // D8
const byte THERMO_CLK = 14; // D5

MAX6675 thermocouple;
#endif

#if XOLED
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

#define SMOKER_FONT u8g2_font_crox5hb_tf
#define FONT u8g2_font_pcsenior_8u
#define FONT_SIZE 10

const byte OLED_CLK = 5; // D1
const byte OLED_DATA = 4; // D2, SDA
// TTGO embedded
//const byte OLED_RESET 16 // D0 == LED_BUILDIN
const byte OLED_RESET = -1;
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R2, /* reset=*/ OLED_RESET, /* clock=*/ OLED_CLK, /* data=*/ OLED_DATA);

#undef LED_BUILTIN
#endif

#if XCLOUD
#include <CloudIoTCore.h>
#include "esp8266_mqtt.h"
#endif

#include <ArduinoJson.h>
#include <FS.h>

String config = "{}";
int16_t minC = 60;
int16_t maxC = 90;
unsigned int maxMins = 0; //3 * 3600;
sint8_t mode = AUTO_TEMP;
uint8_t onMins = 5;
uint8_t offMins = 10;

unsigned long lastMillis = 0;
static uint8_t power;
static uint8_t lastPower;
boolean doSendState;

void setup()
{
#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);
#endif

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, 0);

#if XSERIAL
  Serial.begin(115200);
  Serial.println(PROJECT " v" VERSION);
#endif

#if XOLED
  u8g2.begin();
  u8g2.setContrast(10);
  u8g2.clearBuffer ();
  u8g2.setFont(FONT);
  u8g2.setCursor(10, FONT_SIZE);
  u8g2.print(PROJECT);
  u8g2.setCursor(10, FONT_SIZE * 2);
  u8g2.print("V " VERSION);
  u8g2.sendBuffer ();
#endif

  readConfig();

#if XCLOUD
  setupCloudIoT(); // Creates globals for MQTT
#endif

#if XTHERMO
  thermocouple.begin(THERMO_CLK, THERMO_CS, THERMO_SO);
#endif

#ifdef LED_BUILTIN
  digitalWrite(LED_BUILTIN, 1);
#endif
}

static void readConfig() {
  if (SPIFFS.begin()) {
    File cf = SPIFFS.open("/config.json", "r");
    if (!cf) {
#if XSERIAL
      Serial.println("Failed to open config.json file");
#endif
    } else {
      String config = cf.readString();
      String topic = String("/config.json");
      messageReceived(topic, config);
    }
    cf = SPIFFS.open("/config2.json", "r");
    if (!cf) {
#if XSERIAL
      Serial.println("Failed to open config2.json file");
#endif
    } else {
      String config = cf.readString();
      String topic = String("/config2.json");
      messageReceived(topic, config);
    }
  }

}

static void setOn() {
  digitalWrite(RELAY_PIN, HIGH);
}

static void setOff() {
  digitalWrite(RELAY_PIN, LOW);
}

void loop()
{
#if XCLOUD
  mqttClient->loop();
  delay(10);

  if (!mqttClient->connected())
  {
#if XOLED
    u8g2.setCursor(11, FONT_SIZE * 3);
    u8g2.print("CONNECTING");
    u8g2.sendBuffer ();
#endif
    ESP.wdtDisable();
    connect();
    ESP.wdtEnable(0);

#if XOLED
    u8g2.clearBuffer ();
    u8g2.sendBuffer ();
#endif
    doSendState = true;
  }
#endif

  unsigned long currentMillis = millis();
  unsigned long secs = (unsigned long)(currentMillis / 1000); // !!
  unsigned int mins = secs / 60;

  static unsigned int lastSec;
  if (secs < lastSec + 1) {
    return;
  }
  // TODO handle clock overflow
  lastSec = secs;

#if XSERIAL
  if (mins == 0) {
    Serial.print("secs=");
    Serial.println(secs);
  }
#endif

  uint8_t min = mins % 60;
  uint8_t sec = secs % 60;

#if XCLOUD
  if (doSendState && sec == 0) {
    doSendState = false;
    ESP.wdtDisable();
    sendState();
    ESP.wdtEnable(0);
  }
#endif

#if XTHERMO
  double temp = thermocouple.readCelsius();
  boolean validTemp = !isnan(temp);
  int16_t c = (int16_t)temp;
  static int16_t lastC[2] = { -1000, -1000 };
  if (lastC[0] == -1000) {
    lastC[0] = lastC[1] = c;
  }
  int32_t sum = c + lastC[0] + lastC[1];
  c = sum / 3;
  lastC[1] = lastC[0];
  lastC[0] = c;

#if XSERIAL
  if (sec == 0 || mins == 0) {
    Serial.print("C=");
    if (!validTemp)
      Serial.print("NaN ");
    Serial.println(c);
  }
#endif
  if (mode == AUTO_TEMP && validTemp) {
    if (c < minC) {
      power = 100;
    }
    if (c > maxC) {
      power = 0;
    }
  }
#endif

  if (mode == AUTO_TIME) {
    power = ((mins % (onMins + offMins)) < onMins) * 100;
    if (maxMins > 0 && mins >= maxMins) {
      power = 0;
    }
#if XSERIAL
    if (sec == 0) {
      Serial.print("mins=");
      Serial.println(mins);
    }
  }
#endif

  if (maxMins > 0 && mins >= maxMins) {
    power = 0;
  }
  if (mode >= 0) {
    power = mode; // Force on or off
  }
  if (power != lastPower) {
#if XSERIAL
    Serial.print("power=");
    Serial.println(power);
#endif
    lastPower = power;
  }
  if (power) {
    setOn();
  } else {
    setOff();
  }

#if XOLED

  u8g2.clearBuffer ();
  u8g2.setCursor(11, FONT_SIZE);
  u8g2.print(power ? "ON " : "OFF");
  u8g2.print(' ');

  u8g2.print(mins / 3600, DEC);
  u8g2.print(':');
  if (min < 10) u8g2.print('0');
  u8g2.print(min, DEC);
  u8g2.print(':');
  if (sec < 10) u8g2.print('0');
  u8g2.print(sec, DEC);
  u8g2.print(' ');

  u8g2.setCursor(11, FONT_SIZE * 2);

#if XTHERMO
  if (validTemp) {
    u8g2.print((int)c);
    u8g2.print(' ');
    u8g2.print('C');
  } else {
    u8g2.print("ERROR");
  }
#endif

  u8g2.print(' ');

  if (mode == AUTO_TEMP && validTemp) {
    u8g2.print("TEMP");
  }
  if (mode == AUTO_TIME) {
    u8g2.print("TIME");
  }
  if (mode == 0) {
    u8g2.print("OFF");
  }
  if (mode > 0) {
    u8g2.print(mode, DEC);
  }

  u8g2.sendBuffer ();
#endif

#if XCLOUD
  if (currentMillis - lastMillis > 20000)
  {
    lastMillis = currentMillis;

    StaticJsonDocument<200> doc;
    doc["wifi"] = WiFi.RSSI();
    doc["timestamp"] = time(nullptr);
    doc["min"] = minC;
    doc["max"] = maxC;
    doc["mode"] = mode;
    doc["power"] = power;
    doc["onMins"] = onMins;
    doc["offMins"] = offMins;
#if XTHERMO
    if (validTemp) {
      doc["temperature"] = c;
    }
#endif
    String data;
    serializeJson(doc, data);

#if XSERIAL
    Serial.print("data: ");
    Serial.println(data);
#endif
    ESP.wdtDisable();
    publishTelemetry(data);
    ESP.wdtEnable(0);
  }
#endif
}


#if XCLOUD
void sendState() {
  StaticJsonDocument<200> doc;

  doc["firmware_version"] = PROJECT "v" VERSION;
  doc["min"] = minC;
  doc["max"] = maxC;
  doc["mode"] = mode;
  doc["maxMins"] = maxMins;
  doc["onMins"] = onMins;
  doc["offMins"] = offMins;

  String output;
  serializeJson(doc, output);
#if XSERIAL
  Serial.print("status: ");
  Serial.println(output);
#endif
  publishState(output);
  mqtt->logReturnCode();
}
#endif

void messageReceived(String &topic, String &payload)
{

#if XSERIAL
  Serial.println("incoming: " + topic + " - " + payload);
#endif
  StaticJsonDocument<400> doc;
  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
#if XSERIAL
    Serial.println("parseObject() failed");
    Serial.println(error.c_str());
#endif
    return;
  }

  if (doc.containsKey("min")) {
    minC = doc["min"];
  }
  if (doc.containsKey("max")) {
    maxC = doc["max"];
  }
  if (doc.containsKey("mode")) {
    mode = doc["mode"];
  }
  if (doc.containsKey("maxMins")) {
    maxMins = doc["maxMins"];
  }
  if (doc.containsKey("onMins")) {
    onMins = doc["onMins"];
  }
  if (doc.containsKey("offMins")) {
    offMins = doc["offMins"];
  }
#if XCLOUD
  static char s_ssid[32];
  static char s_password[32];
  static char s_project_id[32];
  static char s_location[32];
  static char s_registry_id[32];
  static char s_device_id[32];
  if (doc.containsKey("ssid")) {
    ssid = s_ssid;
    strncpy(s_ssid, doc["ssid"], 32);
  }
  if (doc.containsKey("password")) {
    strncpy(s_password, doc["password"], 32);
    password = s_password;
  }
  if (doc.containsKey("project_id")) {
    strncpy(s_project_id, doc["project_id"], 32);
    project_id = s_project_id;
  }
  if (doc.containsKey("location")) {
    strncpy(s_location, doc["location"], 32);
    location = s_location;
  }
  if (doc.containsKey("registry_id")) {
    strncpy(s_registry_id, doc["registry_id"], 32);
    registry_id = s_registry_id;
  }
  if (doc.containsKey("device_id")) {
    strncpy(s_device_id, doc["device_id"], 32);
    device_id = s_device_id;
  }
  s_ssid[31] = s_password[31] = s_project_id[31] = 
  s_location[31] = s_registry_id[31] = s_device_id[31] = 0;
#endif

#if 0
  if (!config.equals(payload)) {
    doc["min"] = minC;
    doc["max"] = maxC;
    doc["mode"] = mode;
    doc["maxMins"] = maxMins;
    doc["onMins"] = onMins;
    doc["offMins"] = offMins;
    doc["ssid"] = ssid;
    doc["password"] = password;
    doc["project_id"] = project_id ;
    doc["location"] = location;
    doc["registry_id"] = registry_id;
    doc["device_id"] = device_id;
    String data;
    serializeJson(doc, data);

    File cf = SPIFFS.open("/config2.json", "w");
    if (!cf)  {
#if XSERIAL
      Serial.println("Failed to open config.json file");
#endif
    } else {
      cf.print(data);
      cf.close();
    }
  }
#endif

  doSendState = true;
}
