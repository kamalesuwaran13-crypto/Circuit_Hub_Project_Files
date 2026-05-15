int IRSensor1 = 3; 
int IRSensor2 = 2;

void setup() 
{
  pinMode(IRSensor1, INPUT); 
  pinMode(IRSensor2, INPUT);
  pinMode(4, OUTPUT); 
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
}

void loop()
{
  int statusSensor1 = digitalRead(IRSensor1);
  int statusSensor2 = digitalRead(IRSensor2);
  
  // If IRSensor1 is triggered, turn on LEDs sequentially from pin 4 to pin 11
  if (statusSensor1 == 0)
  {
    // Turn on LEDs sequentially
    digitalWrite(4, HIGH);
    delay(500);
    digitalWrite(5, HIGH);
    delay(500);
    digitalWrite(6, HIGH);
    delay(500);
    digitalWrite(7, HIGH);
    delay(500);
    digitalWrite(8, HIGH);
    delay(500);
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(10, HIGH);
    delay(500);
    digitalWrite(11, HIGH);
    delay(500);
    
    // Wait until IRSensor2 is triggered
    while (digitalRead(IRSensor2) == 1) 
    {
      // Do nothing, just wait
    }
    
    // Wait until IRSensor2 is no longer triggered (passed through it)
    while (digitalRead(IRSensor2) == 0) 
    {
      // Do nothing, just wait
    }

    delay(2000);

    // Turn off LEDs sequentially from pin 11 to pin 4
    digitalWrite(4, LOW);
    delay(500);
    digitalWrite(5, LOW);
    delay(500);
    digitalWrite(6, LOW);
    delay(500);
    digitalWrite(7, LOW);
    delay(500);
    digitalWrite(8, LOW);
    delay(500);
    digitalWrite(9, LOW);
    delay(500);
    digitalWrite(10, LOW);
    delay(500);
    digitalWrite(11, LOW);
    delay(500);
  }

  // If IRSensor2 is triggered, turn on LEDs sequentially from pin 11 to pin 4
  if (statusSensor2 == 0)
  {
    // Turn on LEDs sequentially
    digitalWrite(11, HIGH);
    delay(500);
    digitalWrite(10, HIGH);
    delay(500);
    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(8, HIGH);
    delay(500);
    digitalWrite(7, HIGH);
    delay(500);
    digitalWrite(6, HIGH);
    delay(500);
    digitalWrite(5, HIGH);
    delay(500);
    digitalWrite(4, HIGH);
    delay(500);
    
    // Wait until IRSensor1 is triggered
    while (digitalRead(IRSensor1) == 1) 
    {
      // Do nothing, just wait
    }
    
    // Wait until IRSensor1 is no longer triggered (passed through it)
    while (digitalRead(IRSensor1) == 0) 
    {
      // Do nothing, just wait
    }

    delay(2000);

    // Turn off LEDs sequentially from pin 4 to pin 11
    digitalWrite(11, LOW);
    delay(500);
    digitalWrite(10, LOW);
    delay(500);
    digitalWrite(9, LOW);
    delay(500);
    digitalWrite(8, LOW);
    delay(500);
    digitalWrite(7, LOW);
    delay(500);
    digitalWrite(6, LOW);
    delay(500);
    digitalWrite(5, LOW);
    delay(500);
    digitalWrite(4, LOW);
    delay(500);
  }
}