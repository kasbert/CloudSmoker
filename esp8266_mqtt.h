/******************************************************************************
 * Copyright 2018 Google
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
// This file contains static methods for API requests using Wifi / MQTT
#ifndef __ESP8266_MQTT_H__
#define __ESP8266_MQTT_H__
#include <ESP8266WiFi.h>
#include "FS.h"
#include "WiFiClientSecureBearSSL.h"
#include <time.h>

#include <MQTT.h>

#include <CloudIoTCore.h>
#include <CloudIoTCoreMqtt.h>
#include "ciotc_config.h" // Wifi configuration here

// Initialize WiFi and MQTT for this board
MQTTClient *mqttClient;
BearSSL::WiFiClientSecure *netClient;
BearSSL::X509List certList;
CloudIoTCoreDevice *device;
CloudIoTCoreMqtt *mqtt;
unsigned long iss = 0;
String jwt;

///////////////////////////////
// Helpers specific to this board
///////////////////////////////


String getJwt()
{
  // Disable software watchdog as these operations can take a while.
  ESP.wdtDisable();
  iss = time(nullptr);
#if XSERIAL
  Serial.println("Refreshing JWT");
#endif
  jwt = device->createJWT(iss, jwt_exp_secs);
  ESP.wdtEnable(0);
  return jwt;
}

void setupCert()
{
  // Set CA cert on wifi client
  // If using a static (pem) cert, uncomment in ciotc_config.h:
  /*
   certList.append(primary_ca);
   certList.append(backup_ca);
   netClient->setTrustAnchors(&certList);
   return;
   */

  // If using the (preferred) method with the cert in /data (SPIFFS)

  File ca = SPIFFS.open("/primary_ca.pem", "r");
  if (!ca)
  {
#if XSERIAL
    Serial.println("Failed to open ca file");
#endif
  }
  else
  {
#if XSERIAL
    Serial.println("Success to open ca file");
#endif
    certList.append(strdup(ca.readString().c_str()));
  }

  ca = SPIFFS.open("/backup_ca.pem", "r");
  if (!ca)
  {
#if XSERIAL
    Serial.println("Failed to open ca file");
#endif
  }
  else
  {
#if XSERIAL
    Serial.println("Success to open ca file");
#endif
    certList.append(strdup(ca.readString().c_str()));
  }

  netClient->setTrustAnchors(&certList);
}

void setupWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
#if XSERIAL
  Serial.println("Connecting to WiFi");
#endif
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
  }

  configTime(0, 0, ntp_primary, ntp_secondary);
#if XSERIAL
  Serial.println("Waiting on time sync...");
#endif
  while (time(nullptr) < 1510644967)
  {
    delay(10);
  }
}

void connectWifi()
{
#if XSERIAL
  Serial.print("checking wifi..."); // TODO: Necessary?
#endif
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
}

///////////////////////////////
// Orchestrates various methods from preceeding code.
///////////////////////////////
void publishTelemetry(String data)
{
  mqtt->publishTelemetry(data);
}

void publishTelemetry(const char *data, int length)
{
  mqtt->publishTelemetry(data, length);
}

void publishTelemetry(String subfolder, String data)
{
  mqtt->publishTelemetry(subfolder, data);
}

void publishTelemetry(String subfolder, const char *data, int length)
{
  mqtt->publishTelemetry(subfolder, data, length);
}

void publishState(String data)
{
  mqtt->publishState(data);
}

void connect()
{
  mqtt->mqttConnect();
}

// TODO: fix globals
void setupCloudIoT()
{
  // Create the device
  device = new CloudIoTCoreDevice(
      project_id, location, registry_id, device_id,
      private_key_str);

  // ESP8266 WiFi setup
  netClient = new WiFiClientSecure();
  setupWifi();

  // ESP8266 WiFi secure initialization
  setupCert();

  mqttClient = new MQTTClient(512);
  mqttClient->setOptions(180, true, 1000); // keepAlive, cleanSession, timeout
  mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, device);
  mqtt->setUseLts(true);
  mqtt->setLogConnect(false);
  mqtt->startMQTT(); // Opens connection
  //mqtt->publishState("{\"hello\":\"world!\"}");
  //Serial.println(device->getStateTopic());
  //mqtt->logReturnCode();
}

#endif //__ESP8266_MQTT_H__
