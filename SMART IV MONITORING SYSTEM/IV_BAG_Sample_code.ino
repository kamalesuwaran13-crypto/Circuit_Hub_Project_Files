#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
#include "HX711.h"
#define DOUT  23
#define CLK  19
#define BUZZER 25
HX711 scale(DOUT, CLK);
#define BLYNK_PRINT Serial

char auth[] = "*******";
char ssid[] = "*******";
char pass[] = "********";

int liter;
int val;
float weight; 
float calibration_factor = 102500; // change this value for your Load cell sensor 
 
void setup() {
  // Set up serial monitor
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  pinMode(BUZZER ,OUTPUT);
  Serial.println("Remove all weight from scale");
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
  Blynk.begin(auth, ssid, pass); 
}
 void loop() {
 Blynk.run();
 measureweight();
}
 
void measureweight(){
 scale.set_scale(calibration_factor); //Adjust to this calibration factor
  weight = scale.get_units(5); 
    if(weight<0)
  {
    weight=0.00;
    }
  liter = weight*1000;
  val = liter;
  val = map(val, 0, 505, 0, 100);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("IOT Based IV Bag");
  lcd.setCursor(2, 1);
  lcd.print("Monitering System");
  Serial.print("Kilogram: ");
  Serial.print(weight); 
  Serial.println(" Kg");
  lcd.setCursor(1, 2);
  lcd.print("IV Bottle = ");
  lcd.print(liter);
  lcd.print(" mL");
  Serial.print("IV BOTTLE: ");
  Serial.print(liter);
  Serial.println("mL");
  lcd.setCursor(1, 3);
  lcd.print("IV Bag Percent=");
  lcd.print(val);
  lcd.print("%");
  Serial.print("IV Bag Percent: ");
  Serial.print(val);
  Serial.println("%");
  Serial.println();
  delay(500);
  if (val <= 50 && val >= 40){
    Blynk.logEvent("iv_alert","IV Bottle is 50%");
    digitalWrite(BUZZER, HIGH);
    delay(50);
    digitalWrite(BUZZER, LOW);
    delay(50);
  }
  else if (val <= 20){
    Blynk.logEvent("iv_alert","IV Bottle is too LOW");
    digitalWrite(BUZZER, HIGH);
  }
  else{
    digitalWrite(BUZZER, LOW);
  }
  Blynk.virtualWrite(V0,liter);
  Blynk.virtualWrite(V1,val);
}
