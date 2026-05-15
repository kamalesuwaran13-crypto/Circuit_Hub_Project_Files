//Get Capacitive Soil Moisture Sensor V2.0 Reading (MAX & MIN)
//Provide 3.3V across VCC & GND, and connect AOUT pin with D34 of ESP32
//YouTube Video: 


#define SensorPin       34  //D34 
int     sensorVal;

void setup() {
  // Set up serial monitor
  Serial.begin(115200);
}

void loop() {
  sensorVal = analogRead(SensorPin);
  Serial.print("Moisture Value: ");
  Serial.println(sensorVal);
  delay(2000);
}
