// Single-sensor queue counter + keyboard leave
// Enter: real HC-SR04 trip (A) -> q_len++
// Leave: press '1' in Serial Monitor -> q_len--
// TRIG = GPIO19, ECHO = GPIO18 (ECHO through 10k/15k divider to 3.3V)

const int trigPin = 19;
const int echoPin = 18;

const float SPEED_CM_PER_US = 0.0343f;          // cm/us
const unsigned long PULSE_TIMEOUT_US = 25000;   // ~4.3 m

// Hysteresis for "presence" on Sensor A
const int ENTER_THRESH = 15;   // cm: under this => TRIPPED
const int EXIT_THRESH  = 18;   // cm: over this  => CLEAR (must be > ENTER)

int q_len = 0;          // current queue length
int lastPrintedQL = -1; // ensures first print happens

bool A_tripped = false; // current logical state (with hysteresis)
bool A_prev    = false; // for edge detection (false->true is an entry)

void setup() {
  Serial.begin(9600);
  delay(200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT); // ECHO must be level-shifted to 3.3V
  digitalWrite(trigPin, LOW);

  clearScreenFake();
  Serial.println("Queue counter ready. Type '1' to decrement (leave).");
  Serial.print("Queue length: ");
  Serial.println(q_len);
  lastPrintedQL = q_len;
}

float readDistanceCm() {
  // trigger pulse
  digitalWrite(trigPin, LOW);  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long dur = pulseIn(echoPin, HIGH, PULSE_TIMEOUT_US);
  if (dur == 0) return NAN;
  return (dur * SPEED_CM_PER_US) / 2.0f;
}

void updateSensorA() {
  float cm = readDistanceCm();

  if (isnan(cm)) {
    // treat no-echo as CLEAR (you can change to "hold last" if preferred)
    if (A_tripped) A_tripped = false;
    return;
  }

  if (!A_tripped && cm < ENTER_THRESH) {
    A_tripped = true;
  } else if (A_tripped && cm > EXIT_THRESH) {
    A_tripped = false;
  }
}

void handleKeyboard() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '1') {
      if (q_len > 0) q_len--;
      printIfChanged();
    }
    // ignore other keys
  }
}

void clearScreenFake() {
  for (int i = 0; i < 20; i++) Serial.println();
}

void printIfChanged() {
  if (q_len != lastPrintedQL) {
    clearScreenFake();
    Serial.print("Queue length: ");
    Serial.println(q_len);
    lastPrintedQL = q_len;
  }
}

void loop() {
  // 1) Update presence from ultrasonic
  updateSensorA();

  // 2) Detect rising edge (CLEAR -> TRIPPED) as an entry
  bool fellA = (!A_prev && A_tripped);
  if (fellA) {
    q_len++;
    printIfChanged();
  }
  A_prev = A_tripped;

  // 3) Handle keyboard-based leave
  handleKeyboard();

  // 4) Short delay to keep sensor happy
  delay(80);
}
