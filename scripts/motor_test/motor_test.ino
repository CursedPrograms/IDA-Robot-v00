// IDA motor test: no remote, no sensor, no servo.
// Repeats forward, backward, left, right for 1.5 s each with a pause
// between, and prints each step on the Serial Monitor (115200).
// Same L298N pins and same motor code as IDA.ino.

const int ENA = 5;  // Motor R
const int IN1 = 2;
const int IN2 = 3;

const int ENB = 6;  // Motor L
const int IN3 = 4;
const int IN4 = 7;

const int SPEED = 255;

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
  stopMotors();
  delay(2000);
}

void loop() {
  Serial.println("FORWARD");  drive(LOW,  HIGH, HIGH, LOW);
  Serial.println("BACKWARD"); drive(HIGH, LOW,  LOW,  HIGH);
  Serial.println("LEFT");     drive(LOW,  HIGH, LOW,  HIGH);
  Serial.println("RIGHT");    drive(HIGH, LOW,  HIGH, LOW);
}

void drive(int in1, int in2, int in3, int in4) {
  analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
  digitalWrite(IN1, in1); digitalWrite(IN2, in2);
  digitalWrite(IN3, in3); digitalWrite(IN4, in4);
  delay(1500);
  stopMotors();
  delay(1000);
}

void stopMotors() {
  digitalWrite(ENA, LOW); digitalWrite(ENB, LOW);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}
