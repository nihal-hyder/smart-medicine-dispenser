// TEST 3: Buzzer + buttons
void setup() {
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  tone(8, 1000, 300);                 // one beep at start
}

void loop() {
  if (digitalRead(2) == LOW) tone(8, 1000, 150);   // SET  = medium
  if (digitalRead(3) == LOW) tone(8, 1500, 150);   // UP   = high
  if (digitalRead(4) == LOW) tone(8, 600, 150);    // DOWN = low
  delay(200);
}
