#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>

// High security mode toggle
bool HIGH_SECURITY_MODE = false;

// Wifi
const char* ssid = "Kzm168";
const char* password = "kzmdegozaru";

//***Set server***
const char* mqttServer = "broker.mqtt-dashboard.com"; 
int port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

LiquidCrystal_I2C lcd(0x27, 16, 2);
// SDA: 21
// SCL: 22
// DHT11: 23
int dhtPin = 23;
DHT dht(dhtPin, DHT11);

// LED: 4
int ledPin = 5;
// photoRes: 34
int photoresistorPin = 34;

// PIR: 27
int pirPin = 27;
// Buzzer: 4
int buzzerPin = 4;

void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {}
  Serial.println(" Connected!");
}

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
  wifiConnect();
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive( 90 );

  pinMode(ledPin, OUTPUT);
  pinMode(photoresistorPin, INPUT);
  pinMode(pirPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pirPin), detect_pir, RISING);
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

  delay(1000);
  // test comment
}

void photores_led(int ledPin, int photoresistorPin) {
  int sensorValue = analogRead(photoresistorPin); // Read the photoresistor value
  int ledBrightness = map(sensorValue, 0, 4095, 255, 0); // Map the sensor value to LED brightness (inverted)

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
    tone(buzzerPin, 1000, 1000);
  }
}