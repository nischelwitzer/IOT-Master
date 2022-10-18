/*
** NIS NODEMCU MQTT WiFi - Version 3.2
** LV Master IOT 
** ------------------------------------
** author: AK Nischelwitzer, FH JOANNEUM, IMA, DMT, NIS - Wirtschaftsinformatik
** start coding: 24.06.2022
** last update:  18.10.2022 MultiWifi, MQTT LastWill, DHT11 Sensor & SSID as char
** CodeName: MasterIOT MasterTemplate
**
** MQTT Topics
** subscribed
** * espXX/ctrl 0  --> OFF 
** * espXX/ctrl 1  --> ON 
**
** published
** * espXX/info
** * espXX/button --> contact 0 or 1
** * espXX/important 
**
** * esp/info --> for all ESPs
**
** Multi WiFi
** https://www.embedded-robotics.com/esp8266-wifi/#programming-the-esp8266-multi-wifi
**
*/

#include <Wire.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
#include "DHT.h"

#include "wlan_secrets.h"
#include "mqtt_secrets.h"

const char* ID = "dmt00"; 
const char* HOT_CHILI_VERSION        = "Master IoT V3.2 NIS"; 

const char* MQTT_TOPIC_SUB_CTRL      = "esp/ctrl";        // Control Signal - fixed Time
const char* MQTT_TOPIC_PUB_BUTTON    = "esp/button";      // D2 button contact

const char* MQTT_TOPIC_PUB_INFO      = "dmt00/info";       // for status infos and debugging
const char* MQTT_TOPIC_PUB_IMPORTANT = "esp/important";   // ALL ESPs for status infos and debugging

const char* MQTT_TOPIC_PUB_TEMP      = "esp/temp";        // Temperature DTH22
const char* MQTT_TOPIC_PUB_AIR       = "esp/air";         // Huminity DTH22
const char* MQTT_TOPIC_PUB_LWT       = "esp/lwt";         // Last Will and Testament

const char* MQTT_TOPIC_PUB_ALL       = "esp/info";        // esp/info ALL ESPs for status infos and debugging

// -----------------------------------------------------------------------------------

//Defining the instance of ESP8266 Multi WiFi
ESP8266WiFiMulti wifi_multi;
 
// Multi WiFis 
const char* ssid1     = SECRET_SSID1;
const char* password1 = SECRET_PASS1;
const char* ssid2     = SECRET_SSID2;
const char* password2 = SECRET_PASS2;
const char* ssid3     = SECRET_SSID3;
const char* password3 = SECRET_PASS3;
const char* ssid4     = SECRET_SSID4;
const char* password4 = SECRET_PASS4;
const char* ssid5     = SECRET_SSID5;
const char* password5 = SECRET_PASS5;

String mySSID     = "no SSID";
char   chSSID[60] = "no SSID";

uint16_t    connectTimeOutPerAP = 5000;

// DHT Sensor DHT11 https://wiki.seeedstudio.com/Grove-TemperatureAndHumidity_Sensor/ 
#define FIRST        0  
#define DHTPIN       D7       // what pin we're connected to
#define DHTTYPE DHT11         // DHT11 DHT22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

WiFiClient espClient;
PubSubClient client(espClient);

char msg[70];
int reConnect = 0;
int mqttCnt   = 0;

float  temp  = 0;
float  humid = 0;

// --------------------------------------------------------------------------------
// ###################################
// button

const int ledCtrlPin    = D5;  // *D6*  D5   yellow cable - BLUE LED
const int buttonCtrlPin = D6;  //  D6  *D5*  white 

int buttonCtrlState = 0;
int buttonCtrlPrev  = 0;

int  butCount = 0;

const int BuildInLed1 = D0; // LED_BUILTIN D4 or D0
const int BuildInLed2 = D4; // LED_BUILTIN D4 or D0 - start NOT LOW > internal LED (D4 ok for output)

// ###################################

bool   infoLed = false;

#define seconds() (millis()/1000)
#define minutes() (millis()/60000)
#define hours()   (millis()/3600000)

int run=0;
int oldSec = 99; // check if sec has changed
char   timeString[21] = "Time: init";
char   dataInfo[60]   = "Data init";
char   oledInfo[40]   = "OLED init";
 
// ################################################################################
// ################################################################################
// ##
// ## SETUP
// ##
// ################################################################################
// ################################################################################

void setup() {

  Serial.begin(115200); // 9600 115200
  // Serial.begin(9600); // fastest communication for hottest chilis - 115200*20 SHU scoville heat units
  Serial.println("\n\n"); 
  
  Serial.println("[1/6] Starting NODE MCU Esp8266 UP..."); 
  Serial.println("[2/6] Setting up Sensors and PINs"); 

  pinMode(LED_BUILTIN,   OUTPUT);
  pinMode(BuildInLed1,  OUTPUT);
  pinMode(BuildInLed2,  OUTPUT);

  pinMode(ledCtrlPin,    OUTPUT);
  pinMode(buttonCtrlPin, INPUT); 
  
  Serial.println("[3/6] Setting up WiFi"); 
  setup_wifi(); // WiFi connection - see wlan_secrets.h
  delay(1000);

  Serial.println("[4a/6] Setting up WIRE"); 
  Wire.begin(); 

  Serial.println("[4b/6] Setting up Display u8g2"); 
  u8g2.begin();  // OLED 128 x 64
  showScreen();

  Serial.println("[5/6] Setting up MQTT"); 
  client.setServer(MQTT_BROKER, 1883); // MQTT
  client.setCallback(callback);        // MQTT subscribtions
  setup_mqtt();
  delay(1000);
      
  Serial.println("[6/6] Setup finished - EOS - End of Setup");
  sprintf(dataInfo,"%s WiFi>%s MQTT>%s", ID, WiFi.SSID().c_str(), MQTT_BROKER); 
  client.publish(MQTT_TOPIC_PUB_IMPORTANT, dataInfo, true);
  // client.publish(MQTT_TOPIC_PUB_ALL, dataInfo);
  Serial.println(dataInfo); 

  sprintf(dataInfo,"%s living", ID); 
  client.publish(MQTT_TOPIC_PUB_LWT, dataInfo, true);

  sprintf(dataInfo,"ESP-%s setup finished",ID);
  // client.publish(MQTT_TOPIC_PUB_IMPORTANT, dataInfo);
  Serial.println(dataInfo); 
  Serial.println("=NIS==================================================================EOS=");    
}

// ################################################################################
// ################################################################################
// ##
// ## MAIN LOOP
// ##
// ################################################################################
// ################################################################################
  
void loop() {

  String dataSend;

  // button pressed
  buttonCtrlState = !digitalRead(buttonCtrlPin);
  if (buttonCtrlState != buttonCtrlPrev) {
    dataSend = ">>>Button "+String(buttonCtrlState); // Button  
    Serial.println(dataSend);  // send to pc/unity
    sprintf(dataInfo,"%d",buttonCtrlState);
    client.publish(MQTT_TOPIC_PUB_BUTTON, dataInfo);
    buttonCtrlPrev = buttonCtrlState;
    if (buttonCtrlState) butCount++;
  }

  // ###################################################
  // Timing functions 
  // ###################################################
  
  int mySec = seconds() % 60;
  int myMin = minutes() % 60;
  int myHou = hours(); 
  
  sprintf(timeString,"Time: %02d:%02d.%02d",myHou,myMin,mySec); 
  
  if (mySec != oldSec)  // only when sec changed
  {
    oldSec = mySec;

    if (mySec%2) // alle 2 sec wechsel
    {
      digitalWrite(BuildInLed1, HIGH); 
      digitalWrite(BuildInLed2, LOW);  
      digitalWrite(ledCtrlPin, LOW);
    }
    else
    {
      digitalWrite(BuildInLed1, LOW); 
      digitalWrite(BuildInLed2, HIGH);   
      digitalWrite(ledCtrlPin, HIGH);
    }

    if ((mySec%10) == 0)  // alle 10 sec
    {
      mqttCnt++;
      sprintf(dataInfo,"Uptime %s %02d:%02d.%02d [%d] ",ID, myHou,myMin,mySec, mqttCnt); 
      sprintf(oledInfo,"MQTT:%s - Cnt:%04d",ID, mqttCnt); 
      client.publish(MQTT_TOPIC_PUB_INFO, dataInfo);
    }

    if (mySec%10) // alle 10 sec check
    {

    }

    if ((mySec%30) == 0)  // alle 30 sec send
    {
      humid = dht.readHumidity();     // DHT11 update - slow sensor needs 2 sec
      temp  = dht.readTemperature();
      
      Serial.print("DHT11: Humidity: "); 
      Serial.print(humid);
      Serial.print(" %\t");
      Serial.print("Temperature: "); 
      Serial.print(temp);
      Serial.println(" *C");

      if (!isnan(humid) && (humid > 0.0) && (humid < 100.0))
      {
        sprintf(dataInfo,"%4d",(int)humid);
        client.publish(MQTT_TOPIC_PUB_AIR, dataInfo);
      }
      else
      {
        Serial.println("Humidity Sensor Error");
        humid = -99;
      }


      if (!isnan(temp) && (temp > 0.0) && (temp < 50.0))
      {
        sprintf(dataInfo,"%4d",(int)temp);
        client.publish(MQTT_TOPIC_PUB_TEMP, dataInfo);
      }
      else
      {
        Serial.println("Temperature Sensor Error");
        temp = -99;
      }
    }
  }

  if(wifi_multi.run()!=WL_CONNECTED)
  {
    Serial.print("WiFi Disconnected!!! Try new WiFi connection. ");
    setup_wifi(); 
  }

  if (!client.connected()) {  
    Serial.print("MQTT Disconnected!!! Try new MQTT connection. ");
    setup_mqtt();  // MQTT reconnection
  }
  client.loop();

  showScreen();
  delay(10);
}

// ################################################################################
// ################################################################################

/****************************************
 * WiFi connect and reconnect
 ****************************************/

void setup_wifi() {
    delay(10);
    Serial.println();
    WiFi.mode(WIFI_STA);

    wifi_multi.addAP(ssid1,password1);
    wifi_multi.addAP(ssid2,password2);
    wifi_multi.addAP(ssid3,password3);  
    wifi_multi.addAP(ssid4,password4);
    wifi_multi.addAP(ssid5,password5);  

    Serial.print("Connecting to MultiWiFis... ");
    // single WiFi
    // Serial.println(SECRET_SSID);
    // WiFi.begin(SECRET_SSID, SECRET_PASS);
 
    while(wifi_multi.run(connectTimeOutPerAP)!=WL_CONNECTED)
    {
        digitalWrite(LED_BUILTIN, LOW); // ON short
        Serial.print(".");
        delay(300);
        digitalWrite(LED_BUILTIN, HIGH); // OFF long
        Serial.print("-");
        delay(1000);
    }
 
    Serial.println();
    Serial.println("WiFi connection successful");
    Serial.print("Connected to SSID: ");
    mySSID = WiFi.SSID().c_str();
    sprintf(chSSID,"%s",WiFi.SSID().c_str());
    
    Serial.println(mySSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("=NIS=WIFI=............................................................=OK=");    
}
 
void setup_mqtt() 
{
  long randNumber = random(10000000);
  char mqttCLIENT[50];
  sprintf(mqttCLIENT,"%s_%08d_%s",MQTT_CLIENT, randNumber, ID);
  reConnect++;

    while (!client.connected()) {

        if (reConnect == 1) 
          sprintf(dataInfo,"MQTT first connection... Client: %s",mqttCLIENT);       
        else
          sprintf(dataInfo,"MQTT reconnecting... [%04d] Client: %s ",reConnect ,mqttCLIENT);
        Serial.print(dataInfo);
        
        // boolean connect (clientID, [username, password], [willTopic, willQoS, willRetain, willMessage], [cleanSession])
        sprintf(dataInfo,"MQTT died: %s",ID);
        if (!client.connect(mqttCLIENT, MQTT_USER, MQTT_PASS, MQTT_TOPIC_PUB_LWT, 2, true, dataInfo)) {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying in some seconds");
            digitalWrite(LED_BUILTIN, HIGH); 
            delay(500);
            digitalWrite(LED_BUILTIN, LOW);  
            delay(1500);
          }
    }
    Serial.println();
    
    client.subscribe(MQTT_TOPIC_SUB_CTRL, 1);

    // now when WiFi and MQTT works > Info also to MQTT
    sprintf(dataInfo,"WiFi [%s]: %s @ %s", ID, WiFi.localIP().toString().c_str(), mySSID); 
    client.publish(MQTT_TOPIC_PUB_IMPORTANT, dataInfo, true); 
    client.publish(MQTT_TOPIC_PUB_ALL, dataInfo, true); 
    Serial.println(dataInfo); 
    
    sprintf(dataInfo,"MQTT connected OK: %s - #%d", MQTT_BROKER, reConnect); 
    client.publish(MQTT_TOPIC_PUB_IMPORTANT, dataInfo); 
    Serial.println(dataInfo);  
     
    Serial.println("=NIS=MQTT=............................................................=OK=");    
}

/****************************************
 * MQTT GOT Subscribe
 ****************************************/

void callback(char* topic, byte* payload, unsigned int length) 
{ 
  Serial.println();
  Serial.println("--MQTT---------------------------------");
  // memset(msg, 0, 70);

  sprintf(dataInfo,"MQTT Got Msg: %s [%d] >> ", MQTT_BROKER, length); 
  Serial.print(dataInfo);

  char msg[length+1];
  String mqttStr;

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg[i] = (char)payload[i];
    mqttStr = mqttStr + msg[i];
  }
  Serial.println();

  if(strcmp(topic,MQTT_TOPIC_SUB_CTRL)==0) 
  { 
    Serial.println("CTRL command: " + mqttStr);
    int ctrlGot = mqttStr.toInt();
    
    if (ctrlGot == 1) 
    {
      digitalWrite(ledCtrlPin,HIGH); // ON 
      digitalWrite(LED_BUILTIN, LOW); // ON
    }

    if (ctrlGot == 0) 
    {
      digitalWrite(ledCtrlPin,LOW);  // OFF 
      digitalWrite(LED_BUILTIN, HIGH); // OFF
    }

    sprintf(dataInfo,"Control command: %s turn %d -> GPIO0 done", mqttStr, ctrlGot); 
    Serial.print(dataInfo);
  }

  sprintf(dataInfo,"MQTT got: >%s< >%s< ",topic, mqttStr); 
  Serial.println(dataInfo); 
  Serial.println("--MQTT----------------------------EOM--");
 }

// ################################################################################


/****************************************
 * Auxiliar Functions for OLED
 ****************************************/

void showScreen()
{
  char charRow2[25];
  char charRow3[25];
  char charRow4[25];
  char charRow5[25];
  char charRow6[25];

  // sprintf(charRow2,"SSID: %s ", String(mySSID)); // WiFi.SSID().c_str()
  sprintf(charRow2,"SSID: %s ", chSSID); 
  IPAddress ip;  
  ip = WiFi.localIP();
  sprintf(charRow3,"IP: %s",ip.toString().c_str()); 
  sprintf(charRow4,"%s B:%04d",timeString, butCount); 
  sprintf(charRow5,"%s", oledInfo); 
  sprintf(charRow6,"Temp:%03d*C Humi:%03d%%",(int)temp, (int)humid); 
  // sprintf(charRow6,"012345678901234567890"); 

  // u8g2.clearDisplay();
  u8g2.setDisplayRotation(U8G2_R0);
  u8g2.firstPage();
  do {
    // u8g2.setFont(u8g2_font_ncenB08_tr);
    // u8g2.setFont(u8g2_font_t0_16b_mr);
    u8g2.setFont(u8g2_font_t0_11_mr);   // choose a suitable font https://github.com/olikraus/u8g2/wiki/fntlist8
    //                 012345678901234567890
    u8g2.drawStr(0,10,HOT_CHILI_VERSION);    
    u8g2.drawStr(0,20,charRow2);   
    u8g2.drawStr(0,30,charRow3); 
    u8g2.drawStr(0,40,charRow4);    
    u8g2.drawStr(0,50,charRow5);    
    u8g2.drawStr(0,60,charRow6);    
    u8g2.drawHLine(0,0, 128);
    u8g2.drawHLine(0,63, 128);
  } while (u8g2.nextPage());
}

void showOLED(int x, int y, String str)
{
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_t0_16b_mr);
    u8g2.setCursor(x, y);
    u8g2.print(str);
  } while (u8g2.nextPage());
}

// ################################################################################
// EOF HOT CHILI (C)NIS 2022
// ################################################################################




