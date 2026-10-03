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
int turnTime = 550; // ms — increase for wider turns
int stopDist = 35;  // cm — how close before stopping (obstacle mode)
int diagStopDist = 30; // cm — same, for the diagonal looks
int backupTime = 250;  // ms — reverse this long before the scan so the tracks have room to pivot (0 = off)
int guardDist = 15; // cm — how close before force-stopping while driving forward in drive mode
const unsigned long GUARD_POLL_MS = 150; // how often to check the front sensor in drive mode

// While driving in obstacle mode the sensor looks ahead and to both
// diagonals in turn. A wall met at an angle bounces the ping away from a
// straight-ahead sensor, so it reads as clear; the diagonal looks face
// the wall more squarely and catch it before a track corner does.
// 120 = left diagonal, 60 = right diagonal.
const int lookAngles[] = {90, 120, 90, 60};
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

// Tank mode is button-toggled, not held: the receiver only decodes one
// button at a time, so each color latches its track on or off instead.
bool leftFwdOn = false, leftBwdOn = false, rightFwdOn = false, rightBwdOn = false;

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
  delay(100);

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

  if (lastLeft > lastRight) {
    Serial.println("TURN:LEFT");
    turnLeft();
  } else {
    Serial.println("TURN:RIGHT");
    turnRight();
  }

  stopMotors();
  delay(100);
}

// =====================
void checkIR() {
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
      bool isRepeat = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
      if (!isRepeat) lastIRCmd = IrReceiver.decodedIRData.command;
      lastIRMs = millis();

      Serial.print("IR:0x");
      Serial.println(lastIRCmd, HEX);

      if (!isRepeat) runIRCommand(lastIRCmd);
    }
    IrReceiver.resume();

  } else if (driveMode == MODE_WASD && irDriving && millis() - lastIRMs > IR_HOLD_MS) {
    // no repeat frame arrived in time — treat the arrow as released
    stopMotors();
    irDriving     = false;
    intentForward = false;
  }
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
  if (driveMode == MODE_OBSTACLE) delay(turnTime);
}

void turnRight() {
  analogWrite(ENA, pwmSpeed()); analogWrite(ENB, pwmSpeed());
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  if (driveMode == MODE_OBSTACLE) delay(turnTime);
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
