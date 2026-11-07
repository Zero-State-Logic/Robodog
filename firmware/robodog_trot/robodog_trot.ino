#include <Adafruit_PWMServoDriver.h>
#define LF_T 0
#define LF_F 1
#define RF_T 2
#define RF_F 3
#define LR_T 5
#define LR_F 4
#define RR_T 6
#define RR_F 7

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

int angleToPulse(int ang) {
  const int SERVOMIN = 125;
  const int SERVOMAX = 575;
  ang = constrain(ang, 0, 180);
  return map(ang, 0, 180, SERVOMIN, SERVOMAX);
}

// leg index order used everywhere below: 0=LF, 1=RF, 2=LR, 3=RR
int femurPin[4] = { LF_F, RF_F, LR_F, RR_F };
int tibiaPin[4] = { LF_T, RF_T, LR_T, RR_T };

// your calibrated "perfect stand" pose - the neutral reference
int standFemur[4] = { 40, 120, 140, 50 };
int standTibia[4] = { 65, 120, 60, 115 };

// Direction each leg needs to move (+1) to SWING FORWARD (femur) and
// (+1) to LIFT the foot (tibia).
int femurDir[4] = { +1, -1, +1, -1 };  // LF, RF, LR, RR
int tibiaDir[4] = { -1, +1, +1, -1 };  // LF, RF, LR, RR

// Per-leg gain multipliers - bump a leg's value if its visible swing
// is smaller than the others. Start with RF (index 1) higher and tune from there.
float femurGain[4] = { 1.3, 1.3, 1.0, 1.0 };  // LF, RF, LR, RR
float tibiaGain[4] = { 1.3, 1.0, 1.0, 1.0 };  // LF, RF, LR, RR

// Gait tuning - start conservative, increase once verified stable
int SWING = 15;        // femur swing amplitude in degrees
int LIFT  = 12;        // tibia lift amplitude in degrees
int STEP_DELAY = 200;  // ms per micro-phase - slower = more stable/quasi-static

int pairA[2] = { 0, 3 }; // LF + RR   (diagonal pair)
int pairB[2] = { 1, 2 }; // RF + LR   (diagonal pair)

void setLeg(int leg, int femurOffset, int tibiaOffset) {
  int fAngle = standFemur[leg] + femurDir[leg] * (int)(femurOffset * femurGain[leg]);
  int tAngle = standTibia[leg] + tibiaDir[leg] * (int)(tibiaOffset * tibiaGain[leg]);
  pwm.setPWM(femurPin[leg], 0, angleToPulse(fAngle));
  pwm.setPWM(tibiaPin[leg], 0, angleToPulse(tAngle));
}

void standAll() {
  for (int i = 0; i < 4; i++) setLeg(i, 0, 0);
}

// One diagonal pair lifts + swings forward while the other pair
// stays grounded and sweeps backward, pushing the body forward.
void trotHalfStep(int liftLegs[2], int stanceLegs[2]) {
  // 1) lift swing pair, plant stance pair at forward reach
  for (int i = 0; i < 2; i++) {
    setLeg(liftLegs[i], -SWING, LIFT);
    setLeg(stanceLegs[i], SWING, 0);
  }
  delay(STEP_DELAY);

  // 2) carry lifted pair forward through the air
  for (int i = 0; i < 2; i++) setLeg(liftLegs[i], SWING, LIFT);
  delay(STEP_DELAY);

  // 3) set lifted pair down; stance pair pushes back through neutral (propulsion)
  for (int i = 0; i < 2; i++) {
    setLeg(liftLegs[i], SWING, 0);
    setLeg(stanceLegs[i], -SWING, 0);
  }
  delay(STEP_DELAY);

  // 4) settle everyone back to neutral stand before next half-step
  for (int i = 0; i < 2; i++) {
    setLeg(liftLegs[i], 0, 0);
    setLeg(stanceLegs[i], 0, 0);
  }
  delay(STEP_DELAY);
}

void walkForward(int steps) {
  for (int s = 0; s < steps; s++) {
    trotHalfStep(pairA, pairB);
    trotHalfStep(pairB, pairA);
  }
}

// --- Use this to verify/tune a single leg ---
// Lifts ONE leg with the robot propped up so its feet are off the ground.
void testLeg(int leg) {
  standAll();
  delay(500);
  setLeg(leg, -SWING, LIFT);
  delay(500);
  setLeg(leg, SWING, LIFT);
  delay(500);
  setLeg(leg, 0, 0);
  delay(500);
}

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(60);
  delay(800);
  standAll();
  delay(5000);
}

void loop() {
  walkForward(1);

  // Serial.println("Testing RF leg only");
  // testLeg(1);   // RF = index 1
  // delay(1000);
}