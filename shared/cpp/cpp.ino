// ---- Config ----
static const int TRIG_A = 19, ECHO_A = 18;   // Entry (A)
static const int TRIG_B = 23, ECHO_B = 22;   // Exit  (B)
static const float SPEED_CM_PER_US = 0.0343f;
static const unsigned long PULSE_TO_US = 25000;      // ~4.3m timeout
static const int  ENTER_THRESH_CM = 15, EXIT_THRESH_CM = 18;
static const unsigned long REFRACTORY_MS = 600, CLEAR_DWELL_MS = 250;
static const unsigned long MANUAL_PULSE_MS = 200, UI_REFRESH_MS = 500;

// ---- Types ----
struct Sensor {
  int trig, echo;

  bool tripped = false;       // effective (physical OR manual)
  bool prevTripped = false;   // snapshot before update (for rising-edge)
  bool clearStable = true;    // true after CLEAR has persisted CLEAR_DWELL_MS
  bool clearStablePre = true; // <-- NEW: snapshot of clearStable BEFORE update

  unsigned long clearSince = 0;
  unsigned long lastHitAt = 0;
  unsigned long manualTripEnd = 0;

  float lastCm = NAN;
};

struct QueueManager { int q = 0; };

// ---- Globals ----
Sensor A, B;
QueueManager QM;
unsigned long lastUiAt = 0;

// ---- Utils/UI ----
void fakeClearScreen(){ for(int i=0;i<20;i++) Serial.println(); }
void renderUI(const QueueManager& qm, const Sensor& a, const Sensor& b){
  fakeClearScreen();
  Serial.print("q="); Serial.print(qm.q);
  Serial.print("  A="); Serial.print(a.tripped ? "TRIPPED" : "CLEAR");
  Serial.print("  B="); Serial.print(b.tripped ? "TRIPPED" : "CLEAR");
}

// ---- Driver ----
float readDistanceCm(int trigPin, int echoPin){
  digitalWrite(trigPin, LOW);  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long dur = pulseIn(echoPin, HIGH, PULSE_TO_US);
  if (!dur) return NAN;
  return (dur * SPEED_CM_PER_US) / 2.0f;
}

// ---- Sensor update ----
void updateSensor(Sensor& S){
  float cm = readDistanceCm(S.trig, S.echo);
  S.lastCm = cm;

  bool physTripped;
  if (isnan(cm)) physTripped = false;
  else {
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

// ---- Keyboard ----
void handleKeyboard(Sensor& a, Sensor& b){
  while (Serial.available() > 0){
    char c = (char)Serial.read();
    if (c=='A'||c=='a') a.manualTripEnd = millis() + MANUAL_PULSE_MS;
    else if (c=='B'||c=='b') b.manualTripEnd = millis() + MANUAL_PULSE_MS;
  }
}

// ---- Edge processing (uses PRE-UPDATE snapshots) ----
void processEdges(QueueManager& qm, Sensor& a, Sensor& b){
  unsigned long now = millis();

  bool riseA = (!a.prevTripped && a.tripped);
  if (riseA){
    bool refOK = (now - a.lastHitAt >= REFRACTORY_MS);
    bool clearOK = a.clearStablePre;          // <-- use pre-update snapshot
    if (refOK && clearOK){ qm.q++; a.lastHitAt = now; }
  }

  bool riseB = (!b.prevTripped && b.tripped);
  if (riseB){
    bool refOK = (now - b.lastHitAt >= REFRACTORY_MS);
    bool clearOK = b.clearStablePre;          // <-- use pre-update snapshot
    if (refOK && clearOK){ if (qm.q>0) qm.q--; b.lastHitAt = now; }
  }
}

// ---- Setup helpers ----
void initSensor(Sensor& S, int trigPin, int echoPin){
  S.trig = trigPin; S.echo = echoPin;
  pinMode(S.trig, OUTPUT);
  pinMode(S.echo, INPUT);  // ECHO must be level-shifted to 3.3V
  digitalWrite(S.trig, LOW);
}

void snapshotPrevStates(Sensor& S){
  S.prevTripped   = S.tripped;     // pre-update effective state
  S.clearStablePre = S.clearStable; // <-- NEW: pre-update clearStable
}

// ---- Arduino lifecycle ----
void setup(){
  Serial.begin(9600);
  delay(800);
  initSensor(A, TRIG_A, ECHO_A);
  initSensor(B, TRIG_B, ECHO_B);
  fakeClearScreen();
  Serial.println("Queue Manager ready. A=Entry (19/18), B=Exit (23/22). Press 'A'/'B' to simulate.");
  lastUiAt = millis();
}

void loop(){
  handleKeyboard(A, B);

  // PRE-UPDATE snapshots (critical for correct edge/clear logic)
  snapshotPrevStates(A);
  snapshotPrevStates(B);

  // Update sensors (may flip tripped & clearStable)
  updateSensor(A);
  updateSensor(B);

  // Update queue on edges using pre-update snapshots
  processEdges(QM, A, B);

  // UI every 0.5s
  if (millis() - lastUiAt >= UI_REFRESH_MS){
    lastUiAt = millis();
    renderUI(QM, A, B);
  }

  delay(80);
}
