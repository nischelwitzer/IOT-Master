# IOT Codemaster (IMA)

![IOT HW AUfbau](/assets/iot.jpg)

## Arduino IDE

### USB Driver
Wenn nötig USB Driver **CP210X** Driver 
* https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads 

### Arduino > Preferences > Board Manager
* http://arduino.esp8266.com/stable/package_esp8266com_index.json,https://dl.espressif.com/dl/package_esp32_index.json
* http://arduino.esp8266.com/stable/package_esp8266com_index.json
* https://dl.espressif.com/dl/package_esp32_index.json

### ESP Libs for Demonstrator
* #include <Wire.h>
* #include <U8g2lib.h>
* #include <ESP8266WiFi.h>
* #include <ESP8266WiFiMulti.h>
* #include <PubSubClient.h>
* #include "DHT.h"
> Install  in Lib for Arduino: C:\Users\nisch\OneDrive\Dokumente\Arduino\libraries

## MQTT topic

```
const char* MQTT_TOPIC_PUB_INFO      = "dmtXX/info";       
const char* MQTT_TOPIC_PUB_IMPORTANT = "dmtXX/important";   
const char* MQTT_TOPIC_PUB_TEMP      = "dmtXX/temp";       
const char* MQTT_TOPIC_PUB_AIR       = "dmtXX/air";        
const char* MQTT_TOPIC_PUB_LWT       = "dmtXX/lwt"; 
```
