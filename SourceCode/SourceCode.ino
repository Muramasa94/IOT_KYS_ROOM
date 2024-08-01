#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);
int dhtPin = 23;
DHT dht(dhtPin, DHT11);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.init();
  dht.begin();
  lcd.backlight();
}

void loop() {
  // put your main code here, to run repeatedly:
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  lcd.setCursor(0, 0);
  lcd.print("Humidity:" + String(h));
  Serial.print("Humidity: ");
  Serial.println(String(h));
  Serial.print(h);
  lcd.setCursor(0, 1);
  lcd.print("Temperature:" + String(t));
  Serial.print("Temperature (C): ");
  Serial.println(t);
  Serial.print("Temperature (F): ");
  Serial.println(f);
  delay(1000);
}
