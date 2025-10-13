#include <WiFi.h>
#include "QueueManager.h"
#include <esp_core_dump.h>  // for esp_core_dump_image_erase()
#include "time.h"
static const char* TZ_IL = "IST-2IDT,M3.5.0/2,M10.5.0/2";
static const char* NTP1 = "pool.ntp.org";
static const char* NTP2 = "time.google.com";

// ============================ Types ================================
struct Sensor {
  int trig = -1, echo = -1;

  bool tripped = false;       // effective (physical OR manual)
  bool prevTripped = false;   // snapshot before update (for rising-edge)
  bool clearStable = true;    // true after CLEAR has persisted CLEAR_DWELL_MS
  bool clearStablePre = true; // snapshot of clearStable BEFORE update

  unsigned long clearSince = 0;
  unsigned long manualTripEnd = 0;

  float lastCm = NAN;
};

bool syncNTP(uint32_t maxWaitMs = 15000) {
  configTzTime(TZ_IL, NTP1, NTP2);
  struct tm tmnow;
  uint32_t start = millis();
  while (!getLocalTime(&tmnow, 250)) {
    if (millis() - start > maxWaitMs) return false;
  }
  time_t now = time(nullptr);
  Serial.printf("Time synced: %s", asctime(localtime(&now)));
  return true;
}

// ======================= WIFI  Setup =======================
const char* WIFI_SSID = "YuvalandElla 2.4";
const char* WIFI_PASS = "28112016";
bool connectWiFi(uint32_t timeoutMs = 20000) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK, IP=");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("WiFi FAILED (timeout). Running without cloud writes.");
    return false;
  }
}

// ======================= Queue Manager Setup =======================
static LineSelectionStrategy g_strategy = LineSelectionStrategy::SHORTEST_WAIT_TIME;
static const int NUM_LINES = 1;             // we manage 1 line (you can change later)
static const int EXIT_LINE_NUMBER = 1;      // hard-coded exit line

// Defer construction to setup() to avoid early crashes
QueueManager* g_qm = nullptr;

// ============================ Config ===============================
static const int TRIG_A = 19, ECHO_A = 18;   // Entry (A)  "hostess"
static const int TRIG_B = 23, ECHO_B = 22;   // Exit  (B)  "exit"
static const float SPEED_CM_PER_US = 0.0343f;
static const unsigned long PULSE_TO_US = 25000;      // ~4.3m timeout

// trip logic (with hysteresis)
static const int  ENTER_THRESH_CM = 15, EXIT_THRESH_CM = 18;

// UI + dwell
static const unsigned long CLEAR_DWELL_MS = 250;
static const unsigned long MANUAL_PULSE_MS = 200;
static const unsigned long UI_REFRESH_MS = 500;       // dashboard refresh ~2 Hz

// ============================ Globals ==============================
Sensor A, B;
unsigned long lastUiAt = 0;

// Counters + last event
volatile unsigned long g_entryEvents = 0;
volatile unsigned long g_exitEvents  = 0;
String g_lastEvent = "";

// ============================ Utils/UI =============================
static inline const char* onoff(bool x){ return x ? "ON " : "OFF"; }
static inline const char* yesno(bool x){ return x ? "YES" : "NO "; }

void fakeClearScreen(){ for(int i=0;i<20;i++) Serial.println(); }

int safeGetLineCount(int line){
  if (!g_qm) return -1;
  return g_qm->getLineCount(line);
}

void printLineState() {
  for (int ln = 1; ln <= NUM_LINES; ++ln) {
    int cnt = safeGetLineCount(ln);
    Serial.print("  Line ");
    Serial.print(ln);
    Serial.print(": ");
    if (cnt >= 0) { Serial.print(cnt); Serial.println(" waiting"); }
    else          { Serial.println("(n/a)"); }
  }
}

String formatNow() {
  struct tm tmnow;
  if (getLocalTime(&tmnow, 50)) {  // non-blocking; 50ms timeout
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &tmnow);
    return String(buf);
  }
  return String("1970-01-01 00:00:00 UTC (unsynced)");
}

void renderDashboard(const Sensor& a, const Sensor& b){
  fakeClearScreen();
  Serial.println(F("== Queue Dashboard =="));
  Serial.print(F("Time: "));      Serial.println(formatNow());            // timestamp
  Serial.print(F("Strategy: "));  Serial.println("_ESP32");
  Serial.print(F("Last event: ")); Serial.println(g_lastEvent);
  Serial.print(F("Entries: "));    Serial.print(g_entryEvents);
  Serial.print(F("   Exits: "));   Serial.println(g_exitEvents);

  Serial.println(F("\n-- Sensors --"));
  Serial.print(F("A(hostess) cm="));
  if (isnan(a.lastCm)) Serial.print("NaN ");
  else { Serial.print(a.lastCm, 1); Serial.print(" "); }
  Serial.print(F("   TRIPPED=")); Serial.print(onoff(a.tripped));
  Serial.print(F("   CLEAR_STABLE=")); Serial.println(yesno(a.clearStable));

  Serial.print(F("B(exit)    cm="));
  if (isnan(b.lastCm)) Serial.print("NaN ");
  else { Serial.print(b.lastCm, 1); Serial.print(" "); }
  Serial.print(F("   TRIPPED=")); Serial.print(onoff(b.tripped));
  Serial.print(F("   CLEAR_STABLE=")); Serial.println(yesno(b.clearStable));

  Serial.println(F("\n-- Thresholds --"));
  Serial.print(F("ENTER_THRESH_CM=")); Serial.println(ENTER_THRESH_CM);
  Serial.print(F("EXIT_THRESH_CM =")); Serial.println(EXIT_THRESH_CM);

  Serial.println(F("\n-- Queues --"));
  printLineState();

  Serial.println();
  Serial.println(F("Hints: 'A'/'B' simulate pulses • Serial @115200 baud"));
}

// ============================ Driver ===============================
float readDistanceCm(int trigPin, int echoPin){
  digitalWrite(trigPin, LOW);  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long dur = pulseIn(echoPin, HIGH, PULSE_TO_US);
  if (!dur) return NAN;
  return (dur * SPEED_CM_PER_US) / 2.0f;
}

// ========================= Sensor update ===========================
void updateSensor(Sensor& S){
  float cm = readDistanceCm(S.trig, S.echo);
  S.lastCm = cm;

  bool physTripped;
  if (isnan(cm)) {
    physTripped = false;
  } else {
    bool cur = S.tripped, nxt = cur;
    if (!cur && cm < ENTER_THRESH_CM)      nxt = true;
    else if (cur && cm > EXIT_THRESH_CM)   nxt = false;
    physTripped = nxt;
  }

  bool manualActive = (millis() < S.manualTripEnd);

  bool prevEff = S.tripped;
  S.tripped = physTripped || manualActive;

  // maintain CLEAR dwell on effective state
  if (!S.tripped) {
    if (prevEff) { S.clearSince = millis(); S.clearStable = false; }
    else if (!S.clearStable && millis() - S.clearSince >= CLEAR_DWELL_MS) S.clearStable = true;
  } else {
    S.clearStable = false;
  }
}

// ============================ Keyboard =============================
void handleKeyboard(Sensor& a, Sensor& b){
  while (Serial.available() > 0){
    char c = (char)Serial.read();
    if (c=='A'||c=='a') a.manualTripEnd = millis() + MANUAL_PULSE_MS;
    else if (c=='B'||c=='b') b.manualTripEnd = millis() + MANUAL_PULSE_MS;
    else if (c=='e'||c=='E') { // manual enqueue
      if (g_qm){
        int recommended = g_qm->getNextLineNumber(g_strategy);
        bool ok = g_qm->enqueue(g_strategy);
        if (ok){ g_entryEvents++; g_lastEvent = String("[ENTRY] Manual -> line ") + recommended; }
        else   { g_lastEvent = "[ENTRY] Manual FAILED (capacity?)"; }
      }
    } else if (c=='d'||c=='D') { // manual dequeue line 1
      if (g_qm){
        bool ok = g_qm->dequeue(EXIT_LINE_NUMBER);
        if (ok){ g_exitEvents++; g_lastEvent = "[EXIT] Manual -> line 1"; }
        else   { g_lastEvent = "[EXIT] Manual FAILED (empty)"; }
      }
    }
  }
}

// ============== Edge processing (rising edges only) ================
void processEdges(QueueManager& qmRef, const Sensor& hostessSensor, const Sensor& exitSensor) {
  bool hostessRising = (!hostessSensor.prevTripped && hostessSensor.tripped && hostessSensor.clearStablePre);
  bool exitRising    = (!exitSensor.prevTripped    && exitSensor.tripped    && exitSensor.clearStablePre);

  // --- Hostess sensor triggered -> enqueue (assign a line) ---
  if (hostessRising) {
    int recommended = qmRef.getNextLineNumber(g_strategy); // peek for logging
    bool ok = qmRef.enqueue(g_strategy);                   // writes to Firebase inside
    if (ok){ g_entryEvents++; g_lastEvent = String("[ENTRY] Hostess -> line ") + recommended; }
    else   { g_lastEvent = "[ENTRY] Hostess FAILED (capacity?)"; }
  }

  // --- Exit sensor triggered -> dequeue from line 1 ---
  if (exitRising) {
    bool ok = qmRef.dequeue(EXIT_LINE_NUMBER);             // writes to Firebase inside
    if (ok){ g_exitEvents++; g_lastEvent = "[EXIT] Exit -> line 1"; }
    else   { g_lastEvent = "[EXIT] Exit FAILED (empty)"; }
  }
}

// ========================== Setup helpers ==========================
void initSensor(Sensor& S, int trigPin, int echoPin){
  S.trig = trigPin; S.echo = echoPin;
  pinMode(S.trig, OUTPUT);
  pinMode(S.echo, INPUT);  // ECHO must be 3.3V (use divider if using HC-SR04!)
  digitalWrite(S.trig, LOW);
}

void snapshotPrevStates(Sensor& S){
  S.prevTripped    = S.tripped;      // pre-update effective state
  S.clearStablePre = S.clearStable;  // pre-update clearStable
}

// ======================== Arduino lifecycle ========================
void setup(){
 delay(1500);
  Serial.begin(115200);
  delay(50);

  esp_core_dump_image_erase();

  initSensor(A, TRIG_A, ECHO_A);
  initSensor(B, TRIG_B, ECHO_B);

  // Connect Wi-Fi BEFORE creating QueueManager (prevents HTTP from crashing)
  bool wifiOk = connectWiFi();

  if (wifiOk) {
    if (!syncNTP()) {
      Serial.println("WARN: NTP sync failed; using epoch=1970 fallback.");
    }
    g_qm = new QueueManager(0, NUM_LINES, "_ESP32", "iot-queue-management-ESP32");
  } else {
    // Fallback: create a local-only manager if your class supports it.
    // If it doesn't, you can still create it; writes will likely fail gracefully.
    g_qm = new QueueManager(0, NUM_LINES, "_ESP32", "iot-queue-management-ESP32");
  }

  fakeClearScreen();
  Serial.println(F("Queue Manager ready. A=Entry (19/18), B=Exit (23/22). 'A'/'B' simulate pulses."));
  lastUiAt = millis();
}

void loop() {
  handleKeyboard(A, B);

  snapshotPrevStates(A);
  snapshotPrevStates(B);

  updateSensor(A);
  updateSensor(B);

  if (g_qm) {                // <-- guard
    processEdges(*g_qm, A, B);
  }

  unsigned long now = millis();
  if (now - lastUiAt >= UI_REFRESH_MS) {
    renderDashboard(A, B);
    lastUiAt = now;
  }

  delay(30);
}
