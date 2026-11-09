// TEST 1: Blink the small L light (no wires needed)
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // light on
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);    // light off
  delay(1000);
}
