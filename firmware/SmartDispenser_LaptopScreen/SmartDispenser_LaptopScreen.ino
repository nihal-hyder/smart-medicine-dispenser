/*
  ==========================================================
   SMART MEDICINE DISPENSER
  ==========================================================

  The laptop is the screen. The Arduino sends what the LCD
  would show over the USB cable, and the "Dispenser Screen"
  page in Chrome shows it. The page also sends the laptop's
  real time to the Arduino when it connects.

  WIRING
  ------
   SET  button  -> pin 2   (other leg -> GND)
   UP   button  -> pin 3   (other leg -> GND)
   DOWN button  -> pin 4   (other leg -> GND)
   Buzzer  +    -> pin 8   (- leg -> GND)
   NO LCD needed. NO extra libraries needed.

  MENU
  ----
   UP / DOWN = choose, SET = open
     1 Dispense  : waits for dose time, beeps, shows
                   TAKE MEDICINE!, stock - 1, logs the dose
     2 Time Slots: change each medicine's dose hour
     3 Medicines : UP / DOWN changes stock
*/

// ---------------- SETTINGS YOU CAN CHANGE ----------------
const byte BTN_SET   = 2;
const byte BTN_UP    = 3;
const byte BTN_DOWN  = 4;
const byte BUZZER    = 8;

// 1 = real time. 60 = demo speed (1 second = 1 minute)
const unsigned long CLOCK_SPEED = 1;

const byte NUM_MEDS = 3;
const char* medName[NUM_MEDS] = {"Aspirin", "Panadol", "Vitamin C"};
byte doseHour[NUM_MEDS]       = {8, 14, 20};
byte stock[NUM_MEDS]          = {5, 5, 5};
const byte MAX_STOCK = 30;
// ---------------------------------------------------------


// =============== SCREEN (sent to the laptop) =============
char shown[40] = "";

void show(const char* top, const char* bottom) {
  char line[40];
  snprintf(line, sizeof(line), "%.16s|%.16s", top, bottom);
  if (strcmp(line, shown) != 0) {
    strcpy(shown, line);
    Serial.print(F("LCD|"));
    Serial.println(line);
  }
}

void sendMeds() {
  for (byte i = 0; i < NUM_MEDS; i++) {
    Serial.print(F("MED|"));
    Serial.print(i);  Serial.print('|');
    Serial.print(medName[i]); Serial.print('|');
    Serial.print(doseHour[i]); Serial.print('|');
    Serial.println(stock[i]);
  }
}
// =========================================================


// ======================== CLOCK ==========================
unsigned long clockBaseMillis = 0;
unsigned long clockBaseSec = 7UL * 3600UL + 59UL * 60UL;   // 07:59 until the laptop sends the time

unsigned long nowSec() {
  unsigned long passed = (millis() - clockBaseMillis) / 1000UL * CLOCK_SPEED;
  return (clockBaseSec + passed) % 86400UL;
}
void setClock(byte h, byte m, byte s) {
  clockBaseSec = (unsigned long)h * 3600UL + (unsigned long)m * 60UL + s;
  clockBaseMillis = millis();
}
byte nowHour()     { return nowSec() / 3600UL; }
byte nowMinute()   { return (nowSec() / 60UL) % 60; }
int  nowMinOfDay() { return nowSec() / 60UL; }
// =========================================================


// ======================= BUTTONS =========================
const byte btnPin[3] = {BTN_SET, BTN_UP, BTN_DOWN};
bool btnLast[3] = {HIGH, HIGH, HIGH};
unsigned long btnTime[3] = {0, 0, 0};
bool virtualPress[3] = {false, false, false};   // presses from the laptop page

bool pressed(byte i) {
  if (virtualPress[i]) { virtualPress[i] = false; tone(BUZZER, 2000, 30); return true; }
  bool reading = digitalRead(btnPin[i]);
  if (reading != btnLast[i] && millis() - btnTime[i] > 40) {
    btnTime[i] = millis();
    btnLast[i] = reading;
    if (reading == LOW) { tone(BUZZER, 2000, 30); return true; }
  }
  return false;
}
// =========================================================


// =============== MESSAGES FROM THE LAPTOP ================
// "S" "U" "D" = press SET / UP / DOWN
// "T,hh,mm,ss" = set the clock
// "?" = send everything again
char inBuf[24];
byte inLen = 0;
int lastCheckedMinute = -1;

void handleLine() {
  inBuf[inLen] = 0;
  if (inBuf[0] == 'S' && inLen == 1) virtualPress[0] = true;
  else if (inBuf[0] == 'U' && inLen == 1) virtualPress[1] = true;
  else if (inBuf[0] == 'D' && inLen == 1) virtualPress[2] = true;
  else if (inBuf[0] == 'T') {
    int h = 0, m = 0, s = 0;
    if (sscanf(inBuf, "T,%d,%d,%d", &h, &m, &s) == 3) {
      setClock(h, m, s);
      lastCheckedMinute = nowMinOfDay();
      Serial.print(F("LOG|Clock set from laptop: "));
      if (h < 10) Serial.print('0');
      Serial.print(h); Serial.print(':');
      if (m < 10) Serial.print('0');
      Serial.println(m);
    }
  }
  else if (inBuf[0] == '?') {
    shown[0] = 0;          // force the screen to be sent again
    sendMeds();
  }
  inLen = 0;
}

void readLaptop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { if (inLen) handleLine(); }
    else if (inLen < sizeof(inBuf) - 1) inBuf[inLen++] = c;
  }
}
// =========================================================


enum Mode { MENU, DISPENSE, TIME_SLOTS, MEDICINES };
Mode mode = MENU;
byte menuIndex = 0;
byte editIndex = 0;
char top[17];
char bottom[17];


int nextMedicine() {
  int best = -1;
  long bestWait = 999999L;
  long now = nowSec();
  for (byte i = 0; i < NUM_MEDS; i++) {
    if (stock[i] == 0) continue;
    long wait = (long)doseHour[i] * 3600L - now;
    if (wait <= 0) wait += 86400L;
    if (wait < bestWait) { bestWait = wait; best = i; }
  }
  return best;
}


void dispense(byte i) {
  if (stock[i] == 0) {
    Serial.print(F("EMPTY|")); Serial.println(medName[i]);
    snprintf(top, 17, "EMPTY: %s", medName[i]);
    show(top, "Refill: menu 3");
    tone(BUZZER, 300, 400); delay(600);
    tone(BUZZER, 300, 400); delay(2000);
    return;
  }

  Serial.print(F("ALARM|")); Serial.println(medName[i]);
  snprintf(bottom, 17, "Drop: %s", medName[i]);
  show("TAKE MEDICINE!", bottom);

  for (byte k = 0; k < 3; k++) {
    tone(BUZZER, 880, 200);  delay(260);
    tone(BUZZER, 1320, 200); delay(260);
  }

  delay(2000);

  stock[i]--;
  Serial.print(F("TAKEN|")); Serial.print(medName[i]); Serial.print('|'); Serial.println(stock[i]);
  sendMeds();
  tone(BUZZER, 1500, 150);
  delay(1500);
}


void checkSchedule() {
  int nowMin = nowMinOfDay();
  if (lastCheckedMinute < 0) { lastCheckedMinute = nowMin; return; }
  while (lastCheckedMinute != nowMin) {
    lastCheckedMinute = (lastCheckedMinute + 1) % 1440;
    if (mode != DISPENSE) continue;
    for (byte i = 0; i < NUM_MEDS; i++) {
      if (lastCheckedMinute == doseHour[i] * 60) dispense(i);
    }
    nowMin = nowMinOfDay();
  }
}


void drawScreen() {
  switch (mode) {
    case MENU: {
      snprintf(top, 17, "MENU       %02d:%02d", nowHour(), nowMinute());
      const char* opts[3] = {">1 Dispense", ">2 Time Slots", ">3 Medicines"};
      show(top, opts[menuIndex]);
      break;
    }
    case DISPENSE: {
      int n = nextMedicine();
      if (n < 0) show("All stock empty", "Refill: menu 3");
      else {
        snprintf(top, 17, "Next:%s", medName[n]);
        snprintf(bottom, 17, "Dose %02d:00 %02d:%02d", doseHour[n], nowHour(), nowMinute());
        show(top, bottom);
      }
      break;
    }
    case TIME_SLOTS: {
      snprintf(top, 17, "Set %s", medName[editIndex]);
      snprintf(bottom, 17, "Dose hour: %02d", doseHour[editIndex]);
      show(top, bottom);
      break;
    }
    case MEDICINES: {
      snprintf(top, 17, "%-10sStk:%02d", medName[editIndex], stock[editIndex]);
      show(top, stock[editIndex] ? "UP+ DN- SET>" : "EMPTY (cleared)");
      break;
    }
  }
}


void handleButtons() {
  bool bSet = pressed(0), bUp = pressed(1), bDown = pressed(2);
  if (!bSet && !bUp && !bDown) return;

  switch (mode) {
    case MENU:
      if (bUp)   menuIndex = (menuIndex + 2) % 3;
      if (bDown) menuIndex = (menuIndex + 1) % 3;
      if (bSet) {
        editIndex = 0;
        mode = (menuIndex == 0) ? DISPENSE : (menuIndex == 1) ? TIME_SLOTS : MEDICINES;
        lastCheckedMinute = nowMinOfDay();
      }
      break;

    case DISPENSE:
      if (bSet) mode = MENU;
      break;

    case TIME_SLOTS:
      if (bUp)   doseHour[editIndex] = (doseHour[editIndex] + 1) % 24;
      if (bDown) doseHour[editIndex] = (doseHour[editIndex] + 23) % 24;
      if (bUp || bDown) sendMeds();
      if (bSet) {
        editIndex++;
        if (editIndex >= NUM_MEDS) {
          show("Time slots", "Saved!");
          Serial.println(F("LOG|Time slots saved"));
          delay(900);
          mode = MENU;
          lastCheckedMinute = nowMinOfDay();
        }
      }
      break;

    case MEDICINES:
      if (bUp   && stock[editIndex] < MAX_STOCK) stock[editIndex]++;
      if (bDown && stock[editIndex] > 0)         stock[editIndex]--;
      if (bUp || bDown) sendMeds();
      if (bSet) {
        editIndex++;
        if (editIndex >= NUM_MEDS) {
          show("Stock", "Saved!");
          Serial.println(F("LOG|Stock saved"));
          delay(900);
          mode = MENU;
        }
      }
      break;
  }
}


void setup() {
  Serial.begin(9600);
  pinMode(BTN_SET, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  setClock(7, 59, 0);
  show("Med Dispenser", "Starting...");
  Serial.println(F("LOG|Smart Medicine Dispenser started"));
  sendMeds();
  tone(BUZZER, 1000, 150); delay(200);
  tone(BUZZER, 1500, 150);
  delay(1200);
  mode = MENU;
}


void loop() {
  readLaptop();
  handleButtons();
  checkSchedule();
  drawScreen();
}
