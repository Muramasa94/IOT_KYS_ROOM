#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include "ThingSpeak.h"
#include <FirebaseESP32.h>

// High security mode toggle
bool HIGH_SECURITY_MODE = false;

//***Set server***
const char* mqttServer = "broker.mqtt-dashboard.com"; 
int port = 1883;

//***Set WiFi credentials***
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// firebase api
const char* firebaseHost = "https://kys-database-837f1-default-rtdb.firebaseio.com/";
const char* firebaseSecret = "ivEgKvsKpJbuNI8yjxvLfmNLVs9Q3ulkJIeyxml0"

LiquidCrystal_I2C lcd(0x27, 16, 2);
// SDA: 21
// SCL: 22
// DHT11: 23
int dhtPin = 23;
DHT dht(dhtPin, DHT11);

// LED: 5
int ledPin = 5;
// photoRes: 34
int photoresistorPin = 34;

// PIR: 27
int pirPin = 27;
// Buzzer: 4
int buzzerPin = 4;

// relay: 19
int relayPin = 19;

// flame sensor: 35
int flamePin = 13;

// MQ2: 35
int mq2Pin = 35;
// RGB LED
int redPin = 25;
int greenPin = 26;
int bluePin = 33;

// reed: 14
int reedPin = 14;

// cloud api
const char* writeAPIKey = "HNLSDKYBBINAXE4A";
const char* readAPIKey = "PTEXQU2IMROB7A5L";
const unsigned long channelID = 2621663;

void mqttConnect() {
  while(!mqttClient.connected()) {
    Serial.println("Attemping MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if(mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");

      //***Subscribe all topic you need***
      mqttClient.subscribe("/22127131/highSecurityToggle");
    }
    else {
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

//MQTT Receiver
void callback(char* topic, byte* message, unsigned int length) {
  Serial.println(topic);
  String strMsg;
  for(int i=0; i<length; i++) {
    strMsg += (char)message[i];
  }
  Serial.println(strMsg);

  // Process the received message
  if (strcmp(topic, "/22127131/highSecurityToggle") == 0) {
    if (strMsg == "High security mode: ON") {
      HIGH_SECURITY_MODE = true;
    } else if (strMsg == "High security mode: OFF") {
      HIGH_SECURITY_MODE = false;
    }
  }
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.init();
  dht.begin();
  lcd.backlight();

  Serial.println();
  Serial.print("Connecting to WiFi");

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("KYS_DEVICE"); // password protected ap
  if(!res) {
      Serial.println("Failed to connect");
      // ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("connected...yeey :)");
  }

  // activate cloud api
  ThingSpeak.begin(wifiClient);

  //Set mqtt server
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive( 90 );

  pinMode(ledPin, OUTPUT);
  pinMode(photoresistorPin, INPUT);
  pinMode(pirPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pirPin), detect_pir, RISING);

  pinMode(relayPin, OUTPUT);
  pinMode(flamePin, INPUT);
  pinMode(mq2Pin, INPUT);
  pinMode(reedPin, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:

  if(!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();

  //***Publish data to MQTT Server***
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Only upload to ThingSpeak every 10 seconds (adjust as needed)
  static unsigned long lastUploadTime = 0;
  if (millis() - lastUploadTime > 10000) { // 10 seconds interval
    ThingSpeak.setField(1, h);
    ThingSpeak.setField(2, t);
    int uploadRes = ThingSpeak.writeFields(channelID, writeAPIKey);
    if (uploadRes == 200) {
      Serial.println("Upload successful");
    } else {
      Serial.println("Upload failed");
    }
    lastUploadTime = millis(); // Update the last upload time
  }

  // control relay based on h value
  if (h < 60)
    digitalWrite(relayPin, HIGH);
  else digitalWrite(relayPin, LOW);

  // temp and humidity
  lcd.setCursor(0, 0);
  // Display data on LCD
  lcd.setCursor(0, 0);
  lcd.print("Humidity: " + String(h) + "%");
  lcd.setCursor(0, 1);
  lcd.print("Temp: " + String(t) + "C");

  // Debug output to Serial Monitor
  Serial.print("Humidity: ");
  Serial.println(h);
  Serial.print("Temperature: ");
  Serial.println(t);

  // Prepare data for MQTT publishing
  char h_buffer[10];
  char t_buffer[10];
  dtostrf(h, 6, 2, h_buffer); // Convert float to string with 2 decimal places
  dtostrf(t, 6, 2, t_buffer); // Convert float to string with 2 decimal places

  // Publish data to MQTT topics
  mqttClient.publish("/22127131/Humidity", h_buffer);
  mqttClient.publish("/22127131/Temperature", t_buffer);
  
  // read brightness and adjust led output
  photores_led(ledPin, photoresistorPin);

  // detect flame
  int flame = digitalRead(flamePin);
  if (flame == HIGH) {
    Serial.println("no fire");
    mqttClient.publish("/22127131/Fire", "no");
  } else {
    Serial.println("FIRE!!!!!");
    tone(buzzerPin, 1500, 750);
    mqttClient.publish("/22127131/Fire", "YES!!!!!");
  }

  // read gas concentration value
  int gasVal = analogRead(mq2Pin);
  Serial.print("Toxic gas concentration: ");
  Serial.println(gasVal);
  // set rgb led color based on gas concentration
  if (gasVal > 3000) {
    analogWrite(redPin, 255);
    analogWrite(greenPin, 0);
    analogWrite(bluePin, 0);
  } else if (gasVal > 1500) {
    analogWrite(redPin, 255);
    analogWrite(greenPin, 255);
    analogWrite(bluePin, 0);
  } else {
    analogWrite(redPin, 0);
    analogWrite(greenPin, 255);
    analogWrite(bluePin, 0);
  }
  char g_buffer[10];
  dtostrf(gasVal, 6, 0, g_buffer);
  mqttClient.publish("/22127131/Gas", g_buffer);

  // detect opened door
  int door = digitalRead(reedPin);
  if (door == HIGH) {
    Serial.println("Door opened");
    if (HIGH_SECURITY_MODE)
      tone(buzzerPin, 2000, 750);
    //mqttClient.publish("/22127131/Door", "closed");
  } else {
    Serial.println("Door closed");
    //mqttClient.publish("/22127131/Door", "opened");
  }

  delay(1000);
  // test comment
}

void photores_led(int ledPin, int photoresistorPin) {
  int sensorValue = analogRead(photoresistorPin); // Read the photoresistor value
  int ledBrightness = map_brightness(sensorValue); // Map the sensor value to LED brightness

  analogWrite(ledPin, ledBrightness); // Set the LED brightness

  // Print the sensor value and the mapped brightness for debugging
  Serial.print("Sensor Value: ");
  Serial.print(sensorValue);
  Serial.print("\tLED Brightness: ");
  Serial.println(ledBrightness);
}

void detect_pir() {
  if (HIGH_SECURITY_MODE) {
    Serial.println("Motion detected!");
    tone(buzzerPin, 1000, 2000);
  }
}

int map_brightness(int sensorValue) {
  if (sensorValue > 200) {
    return 0;
  } else {
    return map(sensorValue, 0, 200, 255, 0);
  }
}