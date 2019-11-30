# CloudSmoker
Google Cloud IoT Smoker

Hardware
 * WeMos D1 Mini ESP8266
 * SD1306 128x32 OLED display
 * MAX6675 Thermocouple
 * SSR relay
 * FET
 * Case
 * Electric Smoker
 
Software
 * Arduino ESP8266 firmware
 * Google Clound IoT queues
 * InfluxDB and Grafana in Google Cloud Kubernetes cluster

Based on 
https://github.com/Nilhcem/esp32-cloud-iot-core-k8s

See also
https://github.com/SirUli/Grill-Temperature-ESP8266-MAX6675

Select Board Generic ESP8266 Module
Select Flash Size 512kB (32k SPIFFS)

Copy data/config.json.template to data/config.json and edit.
Write with Arduino -> Tools -> ES8266 Sketch Data Upload

Config data:
 * mode: 0 Smoker is off
 * mode: 100 Smoker is always on
 * mode: -2 Smoker is controlled by temperature
 * mode: -1 Smoker is controlled by time
 * min: In mode -2, the smoker is turned on if temperature falls below min
 * max: In mode -2, the smoker is turned off if temperature raises above max
 * maxMins: In mode 100, -2 and -1, the smoker is turned off after maxMins minutes
 * onMins: In mode -1, the smoker is on for onMins
 * offMins: In mode -1, the smoker is off for offMins
Initial config data: 
 * ssid: Wifi network ssid
 * password: Wifi network password
 * project_id: For example "smoker-123456"
 * location: For example "europe-west1",
 * registry_id: For example "smoker-registry",
 * device_id: For example "smoker-device"

Set config example:
gcloud iot devices configs update --region=europe-west1 --registry=smoker-registry --device=smoker-device --config-data="{min:60,max:80,mode:-2}"
