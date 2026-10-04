#include <Servo.h>
#include <IRremote.hpp>

// IDA: same tank chassis as MILA, cut down to a plain Arduino (Uno/Nano)
// with just the ultrasonic sensor on a servo, an IR receiver and an L298N.
// No WiFi/Bluetooth, everything is done from the IR remote:
//   1        OBSTACLE: drives itself and avoids obstacles (boot default)
//   2        WASD: hold the arrows to drive, let go to stop
//   3        TANK: each color button switches one track on/off
//              RED = left fwd, GREEN = left back,
//              YELLOW = right fwd, BLUE = right back
//              (RED + YELLOW = straight ahead, press again to stop)
//   OK       cycle speed 100 / 75 / 50 / 25 %
// To change tuning (distances, turn time...) edit the values below and
// re-upload over USB.

// === PINS ===
const int trigPin  = 8;
const int echoPin  = 9;
const int servoPin = 10;
const int irPin    = 11;
const int buzzerPin = A0;

// L298N
// On an Uno, Servo takes Timer1 (no PWM on 9/10) and IRremote takes
// Timer2 (no PWM on 3/11), so the only PWM pins left are 5 and 6 (Timer0).
// Remove the ENA/ENB jumpers on the L298N so the PWM pins take effect.
// Motor R
const int ENA = 5;
const int IN1 = 2;
const int IN2 = 3;

// Motor L
const int ENB = 6;
const int IN3 = 4;
const int IN4 = 7;

Servo myServo;

// === TUNING ===
int turnTime = 550; // ms — about a 180; obstacle turns are a random 1/4 to all of this
int stopDist = 35;  // cm — how close before stopping (obstacle mode)
int diagStopDist = 35; // cm — same, for the diagonal looks
int backupTime = 250;  // ms — reverse this long before the scan so the tracks have room to pivot (0 = off)
int guardDist = 15; // cm — how close before force-stopping while driving forward in drive mode
const unsigned long GUARD_POLL_MS = 150; // how often to check the front sensor in drive mode

// While driving in obstacle mode the sensor looks ahead and to both
// diagonals in turn. A wall met at an angle bounces the ping away from a
// straight-ahead sensor, so it reads as clear; the diagonal looks face
// the wall more squarely and catch it before a track corner does.
// 130 = left diagonal, 50 = right diagonal.
const int lookAngles[] = {90, 130, 90, 50};
const int NUM_LOOKS = 4;
const int LOOK_SETTLE_MS = 120; // let the servo stop before pinging
int lookIndex = 0;

// === SPEED ===
const int speedPresets[] = {100, 75, 50, 25};
const int NUM_SPEED_PRESETS = 4;
int speedIndex = 0;

int pwmSpeed() { return (255 * speedPresets[speedIndex]) / 100; }

void cycleSpeed() {
  speedIndex = (speedIndex + 1) % NUM_SPEED_PRESETS;
  Serial.print("SPEED:"); Serial.println(speedPresets[speedIndex]);
}

// === IR REMOTE ===
// Same remote as MILA. Every key is printed as "IR:0x.." on the Serial
// Monitor (115200), so a different remote's codes can be read off there.
#define IR_FORWARD  0x74
#define IR_BACKWARD 0x75
#define IR_LEFT     0x34
#define IR_RIGHT    0x33
#define IR_OK       0x65   // speed

#define IR_MODE_OBSTACLE 0x0   // "1"
#define IR_MODE_WASD     0x1   // "2"
#define IR_MODE_TANK     0x2   // "3"

#define IR_RED    0x25   // tank: left track forward
#define IR_GREEN  0x26   // tank: left track backward
#define IR_YELLOW 0x27   // tank: right track forward
#define IR_BLUE   0x24   // tank: right track backward

const unsigned long IR_HOLD_MS = 350; // no repeat frame within this long = button released
unsigned long lastIRMs  = 0;
uint8_t       lastIRCmd = 0;
bool          irDriving = false;   // true while a held arrow key is driving the motors
unsigned long holdMs    = IR_HOLD_MS;   // release timeout for the current drive (remote or link)

// === IDA LINK (commands relayed by NORA) ===
// NORA's web page, Python controller and Bluetooth link can drive IDA through
// NORA's IR transmitter. Those frames use their own protocol so nothing else
// in the fleet reacts to them: Samsung-format IR (38 kHz) with address
// IDA_LINK_ADDRESS. The command codes 0x48-0x4F are also unused by every
// fleet remote, so even a robot that ignored the address couldn't misread one.
// Drive commands are re-sent every 150 ms while held; IDA stops once they
// have been quiet for LINK_HOLD_MS (a little longer than the remote's 350 ms,
// since frames can arrive while she's busy reading the distance sensor).
#define IDA_LINK_ADDRESS 0x0DA1
#define LINK_FORWARD   0x48
#define LINK_BACKWARD  0x49
#define LINK_LEFT      0x4A
#define LINK_RIGHT     0x4B
#define LINK_STOP      0x4C
#define LINK_OBSTACLE  0x4D
#define LINK_MANUAL    0x4E   // WASD mode
#define LINK_SPEED     0x4F   // cycle 100 / 75 / 50 / 25 %
const unsigned long LINK_HOLD_MS = 500;

// === TALKING WITH NORA ===
// NORA "talks" to IDA over the same link: 0x40 + phrase (hello, how are you,
// happy, curious, sleepy, let's play, bye) after chirping it on her own
// buzzer, and IDA answers on hers. 0x47 is NORA's silent "I am here" beacon:
// when it comes back after NORA_AWAY_MS of silence, IDA greets her. While IDA
// is driving she only gives a short chirp, so her answer never holds up the
// obstacle checks or the drive guard.
#define TALK_FIRST  0x40
#define TALK_LAST   0x46
#define TALK_BEACON 0x47
const unsigned long NORA_AWAY_MS = 60000;
unsigned long lastNoraMs = 0;
bool          noraSeen   = false;

// Tank mode is button-toggled, not held: the receiver only decodes one
// button at a time, so each color latches its track on or off instead.
bool leftFwdOn = false, leftBwdOn = false, rightFwdOn = false, rightBwdOn = false;

// === BUZZER ===
// On an Uno, tone() and IRremote share Timer2, so the receiver is paused
// for the length of the beep and restarted afterwards (as in IRremote's
// ReceiveDemo). Blocks for durMs.
void beep(unsigned int freq, unsigned int durMs) {
  IrReceiver.stopTimer();
  tone(buzzerPin, freq, durMs);
  delay(durMs);
  IrReceiver.restartTimer();
}

// IDA's answers: up to four {freq, ms} notes, freq 0 = a rest, ms 0 = the
// end. Higher and quicker than NORA's voice, so you can tell who's talking.
const uint16_t TALK_REPLY[7][8] PROGMEM = {
  {1200,  80, 1600, 120,    0,   0,    0, 0 },   // hello -> "hi!"
  {1500,  80, 1300,  80, 1800, 140,    0, 0 },   // how are you? -> "great!"
  {1600,  50, 2000,  50, 1600,  50, 2200, 120},   // happy -> trill
  { 900,  80,    0,  40, 1400, 160,    0, 0 },   // curious -> "hm? oh!"
  { 900, 160,  700, 220,    0,   0,    0, 0 },   // sleepy -> yawn
  {1800,  60, 1800,  60, 2400, 140,    0, 0 },   // let's play -> "yes yes!"
  {1600, 100, 1100, 180,    0,   0,    0, 0 },   // bye -> "bye"
};
const char* const TALK_NAMES[7] = { "hello", "how are you", "happy", "curious", "sleepy", "play", "bye" };

// === MODE & STATE ===
enum DriveMode { MODE_OBSTACLE, MODE_WASD, MODE_TANK };
DriveMode driveMode = MODE_OBSTACLE;   // boots straight into obstacle avoidance

// Declared by hand: the IDE's auto-generated prototypes go above this
// enum, where DriveMode doesn't exist yet.
void setMode(DriveMode m);

bool intentForward = false; // driving forward -> watch the front sensor

long lastDist  = 0;
long lastLeft  = 0;
long lastRight = 0;

// =====================
void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
  stopMotors();

  myServo.attach(servoPin);

  IrReceiver.begin(irPin, ENABLE_LED_FEEDBACK);

  // Servo sweep test
  myServo.write(30);  delay(500);
  myServo.write(150); delay(500);
  myServo.write(90);  delay(300);

  pinMode(buzzerPin, OUTPUT);
  beep(2000, 200);

  Serial.println("IDA ready");
}

// =====================
void loop() {
  checkIR();

  if (driveMode != MODE_OBSTACLE) {
    checkDriveGuard();
    return;
  }

  // === OBSTACLE AVOIDANCE ===
  int angle = lookAngles[lookIndex];
  lookIndex = (lookIndex + 1) % NUM_LOOKS;
  myServo.write(angle);
  delay(LOOK_SETTLE_MS);
  long d = getDistance();

  if (angle == 90) {
    lastDist = d;
    Serial.print("DIST:"); Serial.println(d);
  } else {
    Serial.print(angle > 90 ? "DIAG_L:" : "DIAG_R:"); Serial.println(d);
  }

  if (d > (angle == 90 ? stopDist : diagStopDist)) {
    forward();
    return;
  }

  stopMotors();
  lookIndex = 0;
  beep(400, 150);

  if (backupTime > 0) {
    backward();
    delay(backupTime);
    stopMotors();
  }
  delay(200);

  // Scan left
  myServo.write(150);
  delay(500);
  lastLeft = getDistance();
  Serial.print("LEFT:"); Serial.println(lastLeft);

  // Scan right
  myServo.write(30);
  delay(500);
  lastRight = getDistance();
  Serial.print("RIGHT:"); Serial.println(lastRight);

  // Return to center
  myServo.write(90);
  delay(300);

  // A mode key pressed during the scan is held by the receiver; act on it
  // before committing to a turn.
  checkIR();
  if (driveMode != MODE_OBSTACLE) return;

  unsigned long t = pickTurnTime(max(lastLeft, lastRight));
  if (lastLeft > lastRight) {
    Serial.println("TURN:LEFT");
    turnLeft();
  } else {
    Serial.println("TURN:RIGHT");
    turnRight();
  }
  delay(t);

  stopMotors();
  delay(100);
}

// Picks how long to pivot after the scan. turnTime is roughly a 180, so a
// fixed turnTime always spun her round. Instead the turn is sized by how
// open the chosen side is, with some randomness so she doesn't repeat the
// same move in a corner.
unsigned long pickTurnTime(long best) {
  static bool seeded = false;
  if (!seeded) { randomSeed(micros()); seeded = true; }   // first turn comes at a different moment every run
  if (best > 2 * stopDist) return random(turnTime / 4, turnTime / 2);        // wide open: ~45-90 deg
  if (best > stopDist)     return random(turnTime / 2, turnTime * 3 / 4);    // some room: ~90-135 deg
  return random(turnTime * 3 / 4, turnTime + 1);                             // boxed in: ~135-180 deg
}

// =====================
void checkIR() {
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.protocol == SAMSUNG && IrReceiver.decodedIRData.address == IDA_LINK_ADDRESS) {
      bool isRepeat = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
      uint8_t c = IrReceiver.decodedIRData.command;
      lastIRMs = millis();
      Serial.print("LINK:0x");
      Serial.println(c, HEX);
      runLinkCommand(c, isRepeat);

    } else if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
      bool isRepeat = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
      if (!isRepeat) lastIRCmd = IrReceiver.decodedIRData.command;
      lastIRMs = millis();
      if (!isRepeat) holdMs = IR_HOLD_MS;

      Serial.print("IR:0x");
      Serial.println(lastIRCmd, HEX);

      if (!isRepeat) runIRCommand(lastIRCmd);
    }
    IrReceiver.resume();

  } else if (driveMode == MODE_WASD && irDriving && millis() - lastIRMs > holdMs) {
    // no repeat frame arrived in time — treat the arrow as released
    stopMotors();
    irDriving     = false;
    intentForward = false;
  }
}

// A command relayed by NORA over the IDA link. Driving switches IDA into WASD
// mode if she isn't already in it, so one press from NORA is enough; each
// repeated frame keeps the drive alive until the link goes quiet.
void runLinkCommand(uint8_t c, bool isRepeat) {
  if (c >= TALK_FIRST && c <= TALK_BEACON) { hearNora(c); return; }
  switch (c) {
    case LINK_FORWARD: case LINK_BACKWARD: case LINK_LEFT: case LINK_RIGHT:
      if (driveMode != MODE_WASD) setMode(MODE_WASD);
      holdMs = LINK_HOLD_MS;
      if (!irDriving || c != lastIRCmd) {
        lastIRCmd     = c;
        intentForward = (c == LINK_FORWARD);
        if      (c == LINK_FORWARD)  forward();
        else if (c == LINK_BACKWARD) backward();
        else if (c == LINK_LEFT)     turnLeft();
        else                         turnRight();
        irDriving = true;
      }
      break;
    case LINK_STOP:
      setMode(MODE_WASD);   // stops the motors, and takes her out of obstacle mode
      break;
    case LINK_OBSTACLE: if (!isRepeat) setMode(MODE_OBSTACLE); break;
    case LINK_MANUAL:   if (!isRepeat) setMode(MODE_WASD);     break;
    case LINK_SPEED:    if (!isRepeat) cycleSpeed();           break;
  }
}

// True while the motors may be running: IDA answers with one short chirp then.
bool isDriving() {
  return driveMode == MODE_OBSTACLE || irDriving || leftFwdOn || leftBwdOn || rightFwdOn || rightBwdOn;
}

// Play one of the answer phrases (blocking, at most ~0.4 s).
void sing(uint8_t phrase) {
  for (uint8_t i = 0; i < 4; i++) {
    uint16_t f = pgm_read_word(&TALK_REPLY[phrase][i * 2]);
    uint16_t d = pgm_read_word(&TALK_REPLY[phrase][i * 2 + 1]);
    if (d == 0) break;
    if (f) beep(f, d); else delay(d);
    delay(20);
  }
}

// NORA said something (or her beacon arrived): answer, or greet her if
// she's been away.
void hearNora(uint8_t c) {
  bool wasAway = !noraSeen || millis() - lastNoraMs > NORA_AWAY_MS;
  lastNoraMs = millis();
  noraSeen   = true;

  if (c == TALK_BEACON) {
    if (!wasAway) return;                // she's still around: nothing to say
    Serial.println("TALK:NORA is back");
    c = TALK_FIRST;                      // greet her with a hello
  } else {
    Serial.print("TALK:");
    Serial.println(TALK_NAMES[c - TALK_FIRST]);
  }
  if (isDriving()) beep(1800, 40);       // busy: just a quick "mm-hm"
  else             sing(c - TALK_FIRST);
}

// =====================
void setMode(DriveMode m) {
  driveMode     = m;
  irDriving     = false;
  intentForward = false;
  leftFwdOn = leftBwdOn = rightFwdOn = rightBwdOn = false;
  stopMotors();
  myServo.write(90);
  Serial.println(m == MODE_OBSTACLE ? "MODE:OBSTACLE" : m == MODE_WASD ? "MODE:WASD" : "MODE:TANK");
}

// Polls the front sensor in WASD/TANK and force-stops if something gets
// within guardDist while driving forward.
void checkDriveGuard() {
  static unsigned long lastGuardPoll = 0;
  if (millis() - lastGuardPoll < GUARD_POLL_MS) return;
  lastGuardPoll = millis();

  lastDist = getDistance();

  if (intentForward && lastDist > 0 && lastDist < guardDist) {
    stopMotors();
    intentForward = false;
    leftFwdOn = leftBwdOn = rightFwdOn = rightBwdOn = false;
    Serial.println("GUARD");
    beep(400, 150);
  }
}

// Applies the four tank-track toggle latches to the motor pins.
void applyTankMotors() {
  if      (leftFwdOn)  leftMotorFwd();
  else if (leftBwdOn)  leftMotorBwd();
  else                 leftMotorOff();

  if      (rightFwdOn) rightMotorFwd();
  else if (rightBwdOn) rightMotorBwd();
  else                 rightMotorOff();

  intentForward = (leftFwdOn || rightFwdOn);
}

// =====================
void runIRCommand(uint8_t cmd) {
  switch (cmd) {
    case IR_MODE_OBSTACLE: setMode(MODE_OBSTACLE); break;
    case IR_MODE_WASD:     setMode(MODE_WASD);     break;
    case IR_MODE_TANK:     setMode(MODE_TANK);     break;
    case IR_OK:            cycleSpeed();           break;

    case IR_FORWARD:
      if (driveMode == MODE_WASD) { intentForward = true;  forward();   irDriving = true; }
      break;

    case IR_BACKWARD:
      if (driveMode == MODE_WASD) { intentForward = false; backward();  irDriving = true; }
      break;

    case IR_LEFT:
      if (driveMode == MODE_WASD) { intentForward = false; turnLeft();  irDriving = true; }
      break;

    case IR_RIGHT:
      if (driveMode == MODE_WASD) { intentForward = false; turnRight(); irDriving = true; }
      break;

    case IR_RED:
      if (driveMode == MODE_TANK) { leftFwdOn = !leftFwdOn; leftBwdOn = false; applyTankMotors(); }
      break;

    case IR_GREEN:
      if (driveMode == MODE_TANK) { leftBwdOn = !leftBwdOn; leftFwdOn = false; applyTankMotors(); }
      break;

    case IR_YELLOW:
      if (driveMode == MODE_TANK) { rightFwdOn = !rightFwdOn; rightBwdOn = false; applyTankMotors(); }
      break;

    case IR_BLUE:
      if (driveMode == MODE_TANK) { rightBwdOn = !rightBwdOn; rightFwdOn = false; applyTankMotors(); }
      break;
  }
}

// =====================
long getDistance() {
  long total = 0;
  for (int i = 0; i < 3; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, 38000);
    total += (duration == 0) ? 999 : duration * 0.0343 / 2;
    delay(10);
  }
  return total / 3;
}

// =====================
// If a track spins the wrong way, swap that motor's two wires on the
// L298N outputs (OUT1/OUT2 or OUT3/OUT4) rather than editing these.
void stopMotors() {
  digitalWrite(ENA, LOW); digitalWrite(ENB, LOW);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void forward() {
  analogWrite(ENA, pwmSpeed()); analogWrite(ENB, pwmSpeed());
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void backward() {
  analogWrite(ENA, pwmSpeed()); analogWrite(ENB, pwmSpeed());
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  analogWrite(ENA, pwmSpeed()); analogWrite(ENB, pwmSpeed());
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void turnRight() {
  analogWrite(ENA, pwmSpeed()); analogWrite(ENB, pwmSpeed());
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void leftMotorFwd() {
  analogWrite(ENB, pwmSpeed());
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void leftMotorBwd() {
  analogWrite(ENB, pwmSpeed());
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void leftMotorOff() {
  analogWrite(ENB, 0);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void rightMotorFwd() {
  analogWrite(ENA, pwmSpeed());
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
}

void rightMotorBwd() {
  analogWrite(ENA, pwmSpeed());
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
}

void rightMotorOff() {
  analogWrite(ENA, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
}
