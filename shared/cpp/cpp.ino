#include <WiFi.h>
#include <WiFiClient.h>
#include "QueueManager.h"
#include <esp_core_dump.h> // for esp_core_dump_image_erase()
#include "time.h"

// ======================= Timezone & NTP =======================
// Israel TZ (DST: last Sunday of Mar/Oct at 02:00)
static const char *TZ_IL = "IST-2IDT,M3.5.0/2,M10.5.0/2";
// NTP servers
static const char *NTP1 = "pool.ntp.org";
static const char *NTP2 = "time.google.com";

// ============================ Types ================================
struct Sensor
{
  int trig = -1, echo = -1;

  bool tripped = false;       // effective (physical OR manual)
  bool prevTripped = false;   // snapshot before update (for rising-edge)
  bool clearStable = true;    // true after CLEAR has persisted CLEAR_DWELL_MS
  bool clearStablePre = true; // snapshot of clearStable BEFORE update

  unsigned long clearSince = 0;
  unsigned long manualTripEnd = 0;

  float lastCm = NAN;
};

// ============================ Config ===============================
// Entry (A) "hostess"
static const int TRIG_A = 19, ECHO_A = 18;
// Exit (B) "exit line 1"
static const int TRIG_B = 23, ECHO_B = 22;
// Exit (C) "exit line 2"
static const int TRIG_C = 2, ECHO_C = 4;

static const float SPEED_CM_PER_US = 0.0343f;
static const unsigned long PULSE_TO_US = 25000; // ~4.3m timeout

// trip logic (with hysteresis)
static const int ENTER_THRESH_CM = 15, EXIT_THRESH_CM = 18;

// UI + dwell
static const unsigned long CLEAR_DWELL_MS = 250;
static const unsigned long MANUAL_PULSE_MS = 200;
static const unsigned long UI_REFRESH_MS = 500; // dashboard refresh ~2 Hz

// ======================= WIFI  Setup =======================
const char *WIFI_SSID = "iPhone";
const char *WIFI_PASS = "11111112";

bool connectWiFi(uint32_t timeoutMs = 20000)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs)
  {
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("WiFi OK, IP=");
    Serial.println(WiFi.localIP());
    return true;
  }
  else
  {
    Serial.println("WiFi FAILED (timeout). Running without cloud writes.");
    return false;
  }
}

// ======================= Queue Manager Setup =======================
static LineSelectionStrategy g_strategy = LineSelectionStrategy::SHORTEST_WAIT_TIME;
static const int NUM_LINES = 2;         // now managing 2 lines
static const int EXIT1_LINE_NUMBER = 1; // Sensor B drains line 1
static const int EXIT2_LINE_NUMBER = 2; // Sensor C drains line 2

// Defer construction to setup() to avoid early crashes
QueueManager *g_qm = nullptr;

// ============================ Globals ==============================
Sensor A, B, C;
unsigned long lastUiAt = 0;

// Counters + last event
volatile unsigned long g_entryEvents = 0;
volatile unsigned long g_exitEvents = 0;
String g_lastEvent = "";

// ============================ Time/NTP =============================
// Simple sanity check (avoid 1970)
static inline bool timeIsSynced()
{
  const time_t MIN_VALID = 1609459200; // 2021-01-01
  return time(nullptr) >= MIN_VALID;
}

// ---------- HTTP fallback: worldtimeapi.org ----------
bool httpTimeFallback(uint32_t timeoutMs = 8000)
{
  WiFiClient client;
  const char *host = "worldtimeapi.org";
  const char *path = "/api/ip";
  uint32_t start = millis();

  if (!client.connect(host, 80))
  {
    Serial.println("[Time] HTTP fallback: connect failed");
    return false;
  }
  client.print(String("GET ") + path + " HTTP/1.1\r\n"
                                       "Host: " +
               host + "\r\n"
                      "User-Agent: ESP32\r\n"
                      "Connection: close\r\n\r\n");

  while (!client.available() && millis() - start < timeoutMs)
    delay(10);
  if (!client.available())
  {
    Serial.println("[Time] HTTP fallback: no response");
    return false;
  }

  String body;
  bool inBody = false;
  while (client.connected() || client.available())
  {
    String line = client.readStringUntil('\n');
    if (!inBody)
    {
      if (line == "\r")
        inBody = true; // end of headers
    }
    else
    {
      body += line;
    }
  }
  int k = body.indexOf("\"unixtime\"");
  if (k < 0)
  {
    Serial.println("[Time] HTTP fallback: 'unixtime' not found");
    return false;
  }
  int colon = body.indexOf(':', k);
  if (colon < 0)
    return false;
  while (colon + 1 < (int)body.length() && body[colon + 1] == ' ')
    colon++;
  int j = colon + 1;
  long unixSec = 0;
  while (j < (int)body.length() && isDigit(body[j]))
  {
    unixSec = unixSec * 10 + (body[j] - '0');
    j++;
  }
  if (unixSec <= 0)
  {
    Serial.println("[Time] HTTP fallback: bad unixtime");
    return false;
  }

  struct timeval tv;
  tv.tv_sec = unixSec;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);

  setenv("TZ", TZ_IL, 1);
  tzset();

  time_t now = time(nullptr);
  Serial.printf("[Time] HTTP fallback set: %s", asctime(localtime(&now)));
  return timeIsSynced();
}

// ---------- Primary NTP sync with fallback ----------
bool syncNTP(uint32_t maxWaitMs = 15000)
{
  configTzTime(TZ_IL, NTP1, NTP2);
  struct tm tmnow;
  uint32_t start = millis();
  while (!getLocalTime(&tmnow, 250))
  {
    if (millis() - start > maxWaitMs)
    {
      Serial.println("[Time] NTP sync timeout — trying HTTP fallback...");
      if (httpTimeFallback())
        return true;
      return false;
    }
  }
  time_t now = time(nullptr);
  Serial.printf("[Time] NTP synced (local): %s", asctime(localtime(&now)));
  return true;
}

String formatNowLocalOrUnsynced()
{
  if (!timeIsSynced())
    return String("1970-01-01 00:00:00 UTC (unsynced)");
  struct tm tmnow;
  if (!getLocalTime(&tmnow, 50))
    return String("1970-01-01 00:00:00 UTC (unsynced)");
  char buf[48];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &tmnow);
  return String(buf);
}

String formatNowUTC()
{
  if (!timeIsSynced())
    return String("1970-01-01 00:00:00 UTC (unsynced)");
  time_t now = time(nullptr);
  struct tm tm_utc;
  gmtime_r(&now, &tm_utc);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", &tm_utc);
  return String(buf);
}

// ============================ Utils/UI =============================
static inline const char *onoff(bool x) { return x ? "ON " : "OFF"; }
static inline const char *yesno(bool x) { return x ? "YES" : "NO "; }

void fakeClearScreen()
{
  for (int i = 0; i < 20; i++)
    Serial.println();
}

int safeGetLineCount(int line)
{
  if (!g_qm)
    return -1;
  return g_qm->getLineCount(line);
}

void printLineState()
{
  for (int ln = 1; ln <= NUM_LINES; ++ln)
  {
    int cnt = safeGetLineCount(ln);
    Serial.print("  Line ");
    Serial.print(ln);
    Serial.print(": ");
    if (cnt >= 0)
    {
      Serial.print(cnt);
      Serial.println(" waiting");
    }
    else
    {
      Serial.println("(n/a)");
    }
  }
}

void renderDashboard(const Sensor &a, const Sensor &b, const Sensor &c)
{
  fakeClearScreen();
  Serial.println(F("== Queue Dashboard =="));
  if (timeIsSynced())
  {
    Serial.print(F("Time (Local): "));
    Serial.println(formatNowLocalOrUnsynced());
    Serial.print(F("Time (UTC)  : "));
    Serial.println(formatNowUTC());
  }
  else
  {
    Serial.print(F("Time: "));
    Serial.println("1970-01-01 00:00:00 UTC (unsynced)");
  }
  Serial.print(F("Strategy: "));
  Serial.println("_ESP32");
  Serial.print(F("Last event: "));
  Serial.println(g_lastEvent);
  Serial.print(F("Entries: "));
  Serial.print(g_entryEvents);
  Serial.print(F("   Exits: "));
  Serial.println(g_exitEvents);

  Serial.println(F("\n-- Sensors --"));
  Serial.print(F("A(hostess) cm="));
  if (isnan(a.lastCm))
    Serial.print("NaN ");
  else
  {
    Serial.print(a.lastCm, 1);
    Serial.print(" ");
  }
  Serial.print(F("   TRIPPED="));
  Serial.print(onoff(a.tripped));
  Serial.print(F("   CLEAR_STABLE="));
  Serial.println(yesno(a.clearStable));

  Serial.print(F("B(exit L1) cm="));
  if (isnan(b.lastCm))
    Serial.print("NaN ");
  else
  {
    Serial.print(b.lastCm, 1);
    Serial.print(" ");
  }
  Serial.print(F("   TRIPPED="));
  Serial.print(onoff(b.tripped));
  Serial.print(F("   CLEAR_STABLE="));
  Serial.println(yesno(b.clearStable));

  Serial.print(F("C(exit L2) cm="));
  if (isnan(c.lastCm))
    Serial.print("NaN ");
  else
  {
    Serial.print(c.lastCm, 1);
    Serial.print(" ");
  }
  Serial.print(F("   TRIPPED="));
  Serial.print(onoff(c.tripped));
  Serial.print(F("   CLEAR_STABLE="));
  Serial.println(yesno(c.clearStable));

  Serial.println(F("\n-- Thresholds --"));
  Serial.print(F("ENTER_THRESH_CM="));
  Serial.println(ENTER_THRESH_CM);
  Serial.print(F("EXIT_THRESH_CM ="));
  Serial.println(EXIT_THRESH_CM);

  Serial.println(F("\n-- Mapping --"));
  Serial.println(F("B → line 1   |   C → line 2"));

  Serial.println(F("\n-- Queues --"));
  printLineState();

  Serial.println();
  Serial.println(F("Hints: 'A'/'B'/'C' simulate pulses • 'D'=dequeue L1 • 'F'=dequeue L2 • 'E'=enqueue by strategy • Serial @115200 baud"));
}

// ============================ Driver ===============================
float readDistanceCm(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long dur = pulseIn(echoPin, HIGH, PULSE_TO_US);
  if (!dur)
    return NAN;
  return (dur * SPEED_CM_PER_US) / 2.0f;
}

// ========================= Sensor update ===========================
void updateSensor(Sensor &S)
{
  float cm = readDistanceCm(S.trig, S.echo);
  S.lastCm = cm;

  bool physTripped;
  if (isnan(cm))
  {
    physTripped = false;
  }
  else
  {
    bool cur = S.tripped, nxt = cur;
    if (!cur && cm < ENTER_THRESH_CM)
      nxt = true;
    else if (cur && cm > EXIT_THRESH_CM)
      nxt = false;
    physTripped = nxt;
  }

  bool manualActive = (millis() < S.manualTripEnd);

  bool prevEff = S.tripped;
  S.tripped = physTripped || manualActive;

  // maintain CLEAR dwell on effective state
  if (!S.tripped)
  {
    if (prevEff)
    {
      S.clearSince = millis();
      S.clearStable = false;
    }
    else if (!S.clearStable && millis() - S.clearSince >= CLEAR_DWELL_MS)
      S.clearStable = true;
  }
  else
  {
    S.clearStable = false;
  }
}

// ============================ Keyboard =============================
void handleKeyboard(Sensor &a, Sensor &b, Sensor &c)
{
  while (Serial.available() > 0)
  {
    char ch = (char)Serial.read();
    if (ch == 'A' || ch == 'a')
      a.manualTripEnd = millis() + MANUAL_PULSE_MS;
    else if (ch == 'B' || ch == 'b')
      b.manualTripEnd = millis() + MANUAL_PULSE_MS;
    else if (ch == 'C' || ch == 'c')
      c.manualTripEnd = millis() + MANUAL_PULSE_MS;
    else if (ch == 'e' || ch == 'E')
    { // manual enqueue by auto strategy
      if (g_qm)
      {
        bool ok = g_qm->enqueueAuto();
        if (ok)
        {
          bool using_shortest_wait = (g_qm->getCumulativePeopleSummary().completedPeople >= 30);
          LineSelectionStrategy actualStrategy = using_shortest_wait ? LineSelectionStrategy::SHORTEST_WAIT_TIME : LineSelectionStrategy::FEWEST_PEOPLE;
          int recommended = g_qm->getNextLineNumber(actualStrategy);

          g_entryEvents++;
          g_lastEvent = String("[ENTRY] Manual -> line ") + recommended +
                        (using_shortest_wait ? " (SHORTEST_WAIT)" : " (FEWEST_PEOPLE)");
        }
        else
        {
          g_lastEvent = "[ENTRY] Manual FAILED (capacity?)";
        }
      }
    }
    else if (ch == 'd' || ch == 'D')
    { // manual dequeue line 1
      if (g_qm)
      {
        bool ok = g_qm->dequeueAuto(EXIT1_LINE_NUMBER);
        if (ok)
        {
          g_exitEvents++;
          g_lastEvent = "[EXIT] Manual -> line 1";
        }
        else
        {
          g_lastEvent = "[EXIT] Manual FAILED (empty line 1)";
        }
      }
    }
    else if (ch == 'f' || ch == 'F')
    { // manual dequeue line 2
      if (g_qm)
      {
        bool ok = g_qm->dequeueAuto(EXIT2_LINE_NUMBER);
        if (ok)
        {
          g_exitEvents++;
          g_lastEvent = "[EXIT] Manual -> line 2";
        }
        else
        {
          g_lastEvent = "[EXIT] Manual FAILED (empty line 2)";
        }
      }
    }
  }
}

// ============== Edge processing (rising edges only) ================
static inline bool risingEdgeReady(const Sensor &s)
{
  return (!s.prevTripped && s.tripped && s.clearStablePre);
}

void processEdges3(QueueManager &qmRef,
                   const Sensor &hostessSensor,
                   const Sensor &exit1, // B
                   const Sensor &exit2)
{ // C
  bool hostessRising = risingEdgeReady(hostessSensor);
  bool exit1Rising = risingEdgeReady(exit1);
  bool exit2Rising = risingEdgeReady(exit2);

  // --- Hostess sensor triggered -> enqueue (assign a line by strategy) ---
  if (hostessRising)
  {
    // Use automatic strategy (FEWEST_PEOPLE until 30 completions, then SHORTEST_WAIT_TIME)
    bool ok = qmRef.enqueueAuto(); // writes to Firebase inside
    if (ok)
    {
      // Determine which strategy was actually used for logging
      bool using_shortest_wait = (qmRef.getCumulativePeopleSummary().completedPeople >= 30);
      LineSelectionStrategy actualStrategy = using_shortest_wait ? LineSelectionStrategy::SHORTEST_WAIT_TIME : LineSelectionStrategy::FEWEST_PEOPLE;
      int recommended = qmRef.getNextLineNumber(actualStrategy);

      g_entryEvents++;
      g_lastEvent = String("[ENTRY] Hostess -> line ") + recommended +
                    (using_shortest_wait ? " (SHORTEST_WAIT)" : " (FEWEST_PEOPLE)");
    }
    else
    {
      g_lastEvent = "[ENTRY] Hostess FAILED (capacity?)";
    }
  }

  // --- Exit B drains line 1 ---
  if (exit1Rising)
  {
    bool ok = qmRef.dequeueAuto(EXIT1_LINE_NUMBER);
    if (ok)
    {
      g_exitEvents++;
      g_lastEvent = "[EXIT] Exit(B) -> line 1";
    }
    else
    {
      g_lastEvent = "[EXIT] Exit(B) FAILED (empty line 1)";
    }
  }

  // --- Exit C drains line 2 ---
  if (exit2Rising)
  {
    bool ok = qmRef.dequeueAuto(EXIT2_LINE_NUMBER);
    if (ok)
    {
      g_exitEvents++;
      g_lastEvent = "[EXIT] Exit(C) -> line 2";
    }
    else
    {
      g_lastEvent = "[EXIT] Exit(C) FAILED (empty line 2)";
    }
  }
}

// ========================== Setup helpers ==========================
void initSensor(Sensor &S, int trigPin, int echoPin)
{
  S.trig = trigPin;
  S.echo = echoPin;
  pinMode(S.trig, OUTPUT);
  pinMode(S.echo, INPUT); // ECHO must be 3.3V (use divider if using HC-SR04!)
  digitalWrite(S.trig, LOW);
}

void snapshotPrevStates(Sensor &S)
{
  S.prevTripped = S.tripped;        // pre-update effective state
  S.clearStablePre = S.clearStable; // pre-update clearStable
}

// ======================== Arduino lifecycle ========================
void setup()
{
  delay(1500);
  Serial.begin(115200);
  delay(50);

  esp_core_dump_image_erase();

  initSensor(A, TRIG_A, ECHO_A);
  initSensor(B, TRIG_B, ECHO_B);
  initSensor(C, TRIG_C, ECHO_C);

  // Connect Wi-Fi BEFORE creating QueueManager (prevents HTTP from crashing)
  bool wifiOk = connectWiFi();

  if (wifiOk)
  {
    if (!syncNTP())
    {
      Serial.println("WARN: Time sync failed; using epoch=1970 fallback.");
    }
    g_qm = new QueueManager(0, NUM_LINES, "_ESP32", "iot-queue-management-ESP32");
  }
  else
  {
    g_qm = new QueueManager(0, NUM_LINES, "_ESP32", "iot-queue-management-ESP32");
  }

  fakeClearScreen();
  Serial.println(F("Queue Manager ready. A=Entry (19/18), B=Exit L1 (23/22), C=Exit L2 (5/17). 'A'/'B'/'C' simulate pulses; 'D'=L1 dequeue; 'F'=L2 dequeue; 'E'=enqueue."));
  lastUiAt = millis();
}

void loop()
{
  handleKeyboard(A, B, C);

  snapshotPrevStates(A);
  snapshotPrevStates(B);
  snapshotPrevStates(C);

  updateSensor(A);
  updateSensor(B);
  updateSensor(C);

  if (g_qm)
  {
    processEdges3(*g_qm, A, B, C);
  }

  unsigned long now = millis();
  if (now - lastUiAt >= UI_REFRESH_MS)
  {
    renderDashboard(A, B, C);
    lastUiAt = now;
  }

  delay(30);
}
