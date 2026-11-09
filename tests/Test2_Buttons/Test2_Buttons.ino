// TEST 2: Buttons (open Serial Monitor at 9600)
void setup() {
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  Serial.println("Ready! Press a button...");
}

void loop() {
  if (digitalRead(2) == LOW) Serial.println("SET pressed");
  if (digitalRead(3) == LOW) Serial.println("UP pressed");
  if (digitalRead(4) == LOW) Serial.println("DOWN pressed");
  delay(200);
}
