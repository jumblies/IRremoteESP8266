//Requires PubSubClient found here: https://github.com/knolleary/pubsubclient
//
//ESP8266 Simple MQTT switch controller
// Revised 20170815 for double Wemos D1 Relay shield piggyback using pins D1 and D2

// MQTT adapted from garage relay to IR remote sender

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include "secrets.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>

// PANASONIC IR DATA
#define PanasonicAddress      0x4004     // Panasonic address (Pre data)
#define PanasonicPower        0x100BCBD  // Panasonic Power button

// General IRsend instantiation
IRsend irsend(4);  // An IR LED is controlled by GPIO pin 4 (D2)

byte payload = 0;

void callback(char* topic, byte* payload, unsigned int length);

//EDIT THESE LINES TO MATCH YOUR SETUP
//Credentials stored in secrets.h

const int switchPin1 = LED_BUILTIN;  //This is to indicate sending

char const* switchTopic1 = "/living/entertainIR1/";
char const* switchTopic1Confirm = "/living/entertainIR1Confirm/";

WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

//generate unique name from MAC addr
String macToStr(const uint8_t* mac) {

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5) {
      result += ':';
    }
  }
  return result;
}

void reconnect() {
  //attempt to connect to the wifi if connection is lost
  if (WiFi.status() != WL_CONNECTED) {
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if (WiFi.status() == WL_CONNECTED) {
    // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      //if connected, subscribe to the topic(s) we want to be notified about
      //EJ: Delete "mqtt_username", and "mqtt_password" here if you are not using any
      if (client.connect((char*) clientName.c_str(), "", "")) {
        //EJ: Update accordingly with your MQTT account
        Serial.println("\tMQTT Connected");
        client.subscribe(switchTopic1);
      }
      //otherwise print failed for debugging
      else {
        Serial.println("\tFailed.");
        abort();
      }
    }
  }
}

void setup() {

  //start the serial line for debugging
  Serial.begin(115200);
  delay(100);

  irsend.begin();
  pinMode(switchPin1, OUTPUT);

  ArduinoOTA.setHostname("entertainIR1"); // A name given to your ESP8266 module when discovering it as a port in ARDUINO IDE
  ArduinoOTA.begin(); // OTA initialization



  //start wifi subsystem
  WiFi.begin(ssid, password);
  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  //wait a bit before starting the main loop
  delay(2000);
  client.publish(switchTopic1Confirm, "0");
  client.publish(switchTopic1, "0");

}

void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic;
  //EJ: Note:  the "topic" value gets overwritten everytime it receives confirmation (callback) message from MQTT

  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);

  if (topicStr == switchTopic1)
  {

    //turn the switch on if the payload is '1' and publish to the MQTT server a confirmation message
    if (payload[0] == '1') {
      digitalWrite(switchPin1, LOW);
      irsend.sendPanasonic(PanasonicAddress, PanasonicPower);

      delayMicroseconds(50);
      client.publish(switchTopic1Confirm, "1");


      Serial.println("I'm activating!");
    }

    //turn the switch off if the payload is '0' and publish to the MQTT server a confirmation message
    else if (payload[0] == '0') {
      digitalWrite(switchPin1, HIGH);
      client.publish(switchTopic1Confirm, "0");
    }
  }

}




void loop() {

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {
    reconnect();
  }

  //maintain MQTT connection
  client.loop();

  //MUST delay to allow ESP8266 WIFI functions to run
  delay(100);
  ArduinoOTA.handle();
}

