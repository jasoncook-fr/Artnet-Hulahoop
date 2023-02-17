#include <ArtnetWifi.h>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//-------------------------- Wifi settings ----------------------------------
// Prepare your router for a static IP dedicated to our device, and ending with
// the following number (FinalByte)
#define finalByte 100 //example result: '192.168.43.100')
const char* ssid = "SSID";
const char* password = "PASSWORD";

//------------------------ Neopixel settings --------------------------------
const int numLeds = 136;                    // change for your setup
const int numberOfChannels = numLeds * 3;  // Total number of channels you want to receive (1 led = 3 channels: R,G,B)
const byte dataPin = 8;                    // QTPY Pin A3
Adafruit_NeoPixel leds = Adafruit_NeoPixel(numLeds, dataPin, NEO_GRB + NEO_KHZ800);

//------- Led Anim Vals --------
unsigned long animPrevMillis = 0;
const long animInterval = 20;
const long fadeInterval = 5;
unsigned long animCurrentMillis = 0;
int ledCount = 0;
bool animLumenFlag;
bool fadeDir = HIGH;
int fadeValue = 0;
int Position=0;
int runCount = 0;

//------------------------- Artnet settings ---------------------------------
ArtnetWifi artnet;
const int startUniverse = 0;  // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.
// Check if we got all universes
const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[maxUniverses];
bool sendFrame = 1;
int previousDataLength = 0;
unsigned long aNetPrevMillis = 0;
const long aNetInterval = 3000;
unsigned long aNetCurrentMillis = 0;

//------------------------ Battery Read Values ------------------------------
#define VBATPIN 9 //GPIO 9 is pin A2 on QTPY
const int numReadings = 10;
int readings[numReadings];  
int readIndex = 0;          
int total = 0;              
int average = 0;            
/*
If batteryFlag remains HIGH this means that the battery holds a charge higher
than the minimum of which we will react by giving a signal and, ideally,
shutting down. 
The minimum to maximum values read from the battery are between 4900 and 6300
*/
bool batteryFlag = HIGH;
int batteryMinVal = 4000;

//devFlag is reserved for programming and testing DMX software
//This value is changed with the first byte of universe 4.
//If we wish to program without risk of deepSleep triggering, we send a
//non-zero value at this channel
bool devFlag = false;
//Transmission Flag is required to be non-zero to begin DMX data reception.
//This value is changed with the second byte of universe 4
bool transmissionFlag = false;

void applyStaticIP()
{
    // Get current IP address
    IPAddress currentIP = WiFi.localIP();
    IPAddress gateway = WiFi.gatewayIP();
    IPAddress subnet = WiFi.subnetMask();
    IPAddress primaryDNS = WiFi.dnsIP();   //optional
    IPAddress secondaryDNS(8, 8, 4, 4); //optional

    Serial.print("Current IP: ");
    Serial.println(currentIP);

    IPAddress newIP;
    newIP[0] = currentIP[0];
    newIP[1] = currentIP[1];
    newIP[2] = currentIP[2];
    newIP[3] = finalByte;

    // Set the new IP address
    WiFi.config(newIP, gateway, subnet, primaryDNS, secondaryDNS);
    Serial.print("New IP: ");
    Serial.println(WiFi.localIP());
}

// Visual indication to show we are connected to Wifi, but waiting for Artnet signal
void animStandbyDmx(){
    animCurrentMillis = millis();
    int maxBright = 255;
    if (animCurrentMillis - animPrevMillis >= fadeInterval) 
    {
        animPrevMillis = animCurrentMillis;
        if(fadeDir == HIGH)
        {
            fadeValue++; 
            for(int pixCount = 0; pixCount <= numLeds; pixCount++)
            {
                leds.setPixelColor(pixCount, 0, fadeValue, 0);
            }
            leds.show();
            if(fadeValue == maxBright)
            {
                fadeDir = LOW;
            }
        }
        else
        {    
            fadeValue--;
            for(int pixCount = 0; pixCount <= numLeds; pixCount++)
            {
                leds.setPixelColor(pixCount, 0, fadeValue, 0);
            }
            leds.show();
            if(fadeValue == 0)
            {
                fadeDir = HIGH;
            }   
        }
    }
}

//Visual indication to show we are searching for the wifi network
void animSearchWifi(byte red, byte green, byte blue, int waveDelay) 
{
    //int Position=0;
    animCurrentMillis = millis();
    if (animCurrentMillis - animPrevMillis >= waveDelay) 
    {
        Position++;
        for(int i=0; i<numLeds; i++) 
        {
            leds.setPixelColor(i,((sin(i+Position) * 127 + 128)/255)*red,
                    ((sin(i+Position) * 127 + 128)/255)*green,
                    ((sin(i+Position) * 127 + 128)/255)*blue);
        }
        leds.show();

        if(runCount>=numLeds*2) runCount = 0;
        else runCount++;
        animPrevMillis = animCurrentMillis;
    }
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
    sendFrame = 1;
    /*
    if (universe == 3) {
        //if(data[0] == 0) devFlag = false;
        //else devFlag = true;
        if(data[1] == 0) transmissionFlag = false;
        else transmissionFlag = true;
        Serial.print("TxFlag is ");
        Serial.println(transmissionFlag);
    }
    */
    // Store which universe has got in
    if ((universe - startUniverse) < maxUniverses)
        universesReceived[universe - startUniverse] = 1;

    for (int i = 0; i < maxUniverses; i++) {
        if (universesReceived[i] == 0) {
            //Serial.println("Broke");
            sendFrame = 0;
            break;
        }
    }

    // read universe and put into the right part of the display buffer
    for (int i = 0; i < length / 3; i++) {
        int led = i + (universe - startUniverse) * (previousDataLength / 3);
        if (led < numLeds)
            leds.setPixelColor(led, data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
    }
    previousDataLength = length;

    if (sendFrame) {
        leds.show();
        transmissionFlag = true;
        aNetPrevMillis = aNetCurrentMillis;
        // Reset universeReceived to 0
        memset(universesReceived, 0, maxUniverses);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("READY TO GO!");

    //connect to wifi -----------------------------------
    WiFi.mode(WIFI_STA);  //Connect to your wifi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        animSearchWifi(0,0,120,50);
    }
    applyStaticIP();
    //init OTA wireless update requirements
    ArduinoOTA.onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
            else  // U_SPIFFS
            type = "filesystem";
            });
    ArduinoOTA.begin();

    artnet.begin();
    leds.begin();

    // this will be called for each artnet packet received
    artnet.setArtDmxCallback(onDmxFrame);

    // For battery readings, initialize analog smoothing
    for (int thisReading = 0; thisReading < numReadings; thisReading++)
    {
        readings[thisReading] = 0;
    }

}

void loop() {
    if (WiFi.status() != WL_CONNECTED) 
    {
        ESP.restart();
    }
    if(batteryFlag == HIGH) 
    {
        if(transmissionFlag == false) 
        {
            ArduinoOTA.handle();
            animStandbyDmx();
        }
        else if(aNetCurrentMillis - aNetPrevMillis >= aNetInterval) 
        {
            transmissionFlag = false; 
        }
        else aNetCurrentMillis = millis();
        artnet.read();
        readVbat();
    }
    else
    {
        Serial.print("batteryFlag is");
        Serial.println(batteryFlag);
        Serial.print("Value is ");
        Serial.println(average);
        //Blink RED
        for (int i = 0; i < numLeds; i++)
            leds.setPixelColor(i, 127, 0, 0);
        leds.show();
        delay(300);
        for (int i = 0; i < numLeds; i++)
            leds.setPixelColor(i, 0, 0, 0);
        leds.show();
        delay(300);

    }
}

void readVbat()
{
    total = total - readings[readIndex];
    readings[readIndex] = analogRead(VBATPIN);
    total = total + readings[readIndex];
    readIndex = readIndex + 1;
    if (readIndex >= numReadings)
    {
        readIndex = 0;
        average = total / numReadings;
    }
    if((average < batteryMinVal)&&(average > 1000)) 
    {
        batteryFlag = LOW;
        Serial.print("-----Average came out LOW -----");
    }
  Serial.println(average);
}
