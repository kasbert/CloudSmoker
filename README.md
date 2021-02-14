# CloudSmoker
Google Cloud IoT Smoker

Based on 
https://github.com/Nilhcem/esp32-cloud-iot-core-k8s

![setup](/img/IMG_20191130_160347.jpg)

Hardware
 * WeMos D1 Mini ESP8266
 * SD1306 128x32 OLED display
 * MAX6675 Thermocouple
 * SSR relay
 * FET for controlling relay, if 3.3V is not enough
 * Case
 * Electric Smoker
 
![schematic](/img/Schematic_smoker_CloudSmoker_20191201204111.svg)

Software
 * Arduino ESP8266 firmware
 * Google Clound IoT queues
 * InfluxDB and Grafana in Google Cloud Kubernetes cluster

![graph](/img/CloudSmoker.svg)

Grafana graph showing power, temperature & limits: 

![graph](/img/Screenshot_2019-12-01%20Grafana%20-%20Smoker.png)


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

Create private key
```
openssl ecparam -genkey -name prime256v1 -noout -out ec_private.pem
openssl ec -in ec_private.pem -pubout -out ec_public.pem
openssl ec -in ec_private.pem -noout -text | sed -n -e '/priv:/ {n;p;n;p;n;p}' | sed -e 's,^ *,",' -e 's,$,",' > ec_private.h
```

Set config example:
```
gcloud iot devices configs update --region=europe-west1 --registry=smoker-registry --device=smoker-device --config-data="{min:60,max:80,mode:-2}"
```

Pins
 * RELAY_PIN = 13; // D7
 * THERMO_SO = 12; // D6
 * THERMO_CS = 15; // D8
 * THERMO_CLK = 14; // D5
 * OLED_CLK = 5; // D1
 * OLED_DATA = 4; // D2, SDA
 
 # User interface
 User interface reads and writes config messages and fetches latest data from influxdb.
 https://github.com/kasbert/CloudSmoker-UI
