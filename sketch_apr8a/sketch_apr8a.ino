#include <TFT_eSPI.h>
#include <ESP32Servo.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

// ===== SENSOR =====
#define VIB_PIN 27

bool angryMode = false;
unsigned long angryStart = 0;

// ===== SERVOS =====
Servo leftHand, rightHand, bodyServo;

float leftPos = 90, rightPos = 90, bodyPos = 90;
float targetLeft = 90, targetRight = 90, targetBody = 90;

// ===== SCREEN =====
#define SCREEN_W 320
#define SCREEN_H 240
#define SPR_W 240
#define SPR_H 140

// ===== EYE BASE =====
int baseWidth  = 50;
int baseHeight = 80;

int centerX = SPR_W / 2;
int centerY = SPR_H / 2 - 25;

int eyeSpacing = 140;

int leftEyeX  = centerX - eyeSpacing / 2;
int rightEyeX = centerX + eyeSpacing / 2;
int eyeY      = centerY;

// ===== CONSTANTS =====
#define MIN_SCALE 3.0
#define BIG_SCALE 18.0

// ===== STATES =====
enum State {
  NORMAL,
  BLINK,
  BIG_LEFT,
  BIG_RIGHT
};

State currentState = NORMAL;

float scaleL = 10.0;
float scaleR = 10.0;

unsigned long lastAction = 0;
unsigned long holdTime = 0;

// ===== DRAW =====
void drawEyes() {

  spr.fillSprite(TFT_BLACK);

  // ===== 😠 ANGRY =====
  if (angryMode) {

    int w = 60;
    int h = 40;

    // Left eye
    spr.fillRoundRect(leftEyeX - w/2, eyeY - h/2, w, h, 10, TFT_WHITE);
    spr.fillTriangle(leftEyeX - w/2, eyeY - h/2,
                     leftEyeX + w/2, eyeY - h/2,
                     leftEyeX + w/2, eyeY,
                     TFT_BLACK);

    // Right eye
    spr.fillRoundRect(rightEyeX - w/2, eyeY - h/2, w, h, 10, TFT_WHITE);
    spr.fillTriangle(rightEyeX - w/2, eyeY,
                     rightEyeX + w/2, eyeY - h/2,
                     rightEyeX - w/2, eyeY - h/2,
                     TFT_BLACK);
  }

  // ===== 😐 NEUTRAL =====
  else {

    int wL = baseWidth  * (scaleL / 10.0);
    int hL = baseHeight * (scaleL / 10.0);

    int wR = baseWidth  * (scaleR / 10.0);
    int hR = baseHeight * (scaleR / 10.0);

    int maxHeight = 110;
    if (hL > maxHeight) hL = maxHeight;
    if (hR > maxHeight) hR = maxHeight;

    int shiftL = (hL - baseHeight) / 2;
    int shiftR = (hR - baseHeight) / 2;

    spr.fillRoundRect(leftEyeX - wL/2,
                      eyeY - hL/2 + shiftL,
                      wL, hL,
                      wL/2,
                      TFT_WHITE);

    spr.fillRoundRect(rightEyeX - wR/2,
                      eyeY - hR/2 + shiftR,
                      wR, hR,
                      wR/2,
                      TFT_WHITE);
  }

  spr.pushSprite((SCREEN_W - SPR_W)/2,
                 (SCREEN_H - SPR_H)/2 - 10);
}

// ===== BLINK =====
void doBlink() {

  static int phase = 0;

  if (phase == 0) {
    scaleL -= (scaleL - MIN_SCALE) * 0.35;
    scaleR -= (scaleR - MIN_SCALE) * 0.35;

    if (scaleL <= MIN_SCALE + 0.2) {
      scaleL = scaleR = MIN_SCALE;
      holdTime = millis();
      phase = 1;
    }
  }
  else if (phase == 1) {
    if (millis() - holdTime > 80) phase = 2;
  }
  else if (phase == 2) {
    scaleL += (10 - scaleL) * 0.2;
    scaleR += (10 - scaleR) * 0.2;

    if (scaleL >= 9.8) {
      scaleL = scaleR = 10;
      phase = 0;
      currentState = NORMAL;
    }
  }
}

// ===== BIG LEFT =====
void bigLeft() {

  static bool growing = true;

  if (growing) {
    scaleL += (BIG_SCALE - scaleL) * 0.35;
    if (scaleL > BIG_SCALE - 0.5) growing = false;
  } else {
    scaleL += (10 - scaleL) * 0.08;
    if (scaleL < 10.2) {
      scaleL = 10;
      growing = true;
      currentState = NORMAL;
    }
  }
}

// ===== BIG RIGHT =====
void bigRight() {

  static bool growing = true;

  if (growing) {
    scaleR += (BIG_SCALE - scaleR) * 0.35;
    if (scaleR > BIG_SCALE - 0.5) growing = false;
  } else {
    scaleR += (10 - scaleR) * 0.08;
    if (scaleR < 10.2) {
      scaleR = 10;
      growing = true;
      currentState = NORMAL;
    }
  }
}

// ===== STATE UPDATE =====
void updateState() {

  if (angryMode) return;

  if (millis() - lastAction > random(2000, 5000)) {

    int r = random(0, 100);

    if (r < 40) currentState = BLINK;
    else if (r < 70) currentState = BIG_LEFT;
    else currentState = BIG_RIGHT;

    lastAction = millis();
  }
}

// ===== ANIMATION =====
void updateAnimation() {

  if (angryMode) return;

  switch (currentState) {

    case NORMAL:
      scaleL += (10 - scaleL) * 0.1;
      scaleR += (10 - scaleR) * 0.1;
      break;

    case BLINK:
      doBlink();
      break;

    case BIG_LEFT:
      bigLeft();
      break;

    case BIG_RIGHT:
      bigRight();
      break;
  }
}

// ===== SERVO =====
void updateServos() {

  static unsigned long lastChange = 0;

  float speed = angryMode ? 0.30 : 0.20;

  leftPos  += (targetLeft  - leftPos)  * speed;
  rightPos += (targetRight - rightPos) * speed;
  bodyPos  += (targetBody  - bodyPos)  * speed;

  leftHand.write(180 - leftPos);
  rightHand.write(180 - rightPos);
  bodyServo.write(bodyPos);

  if (millis() - lastChange > 1500) {

    if (angryMode) {
      targetLeft  = random(20, 160);
      targetRight = random(20, 160);
      targetBody  = random(60, 120);
    } else {
      targetLeft  = random(40, 140);
      targetRight = random(50, 130);
      targetBody  = random(70, 110);
    }

    lastChange = millis();
  }
}

// ===== SENSOR =====
void checkVibration() {

  if (digitalRead(VIB_PIN) == HIGH) {
    angryMode = true;
    angryStart = millis();
  }

  if (angryMode && millis() - angryStart > 3000) {
    angryMode = false;
  }
}

// ===== SETUP =====
void setup() {

  pinMode(VIB_PIN, INPUT);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  spr.createSprite(SPR_W, SPR_H);

  leftHand.setPeriodHertz(50);
  rightHand.setPeriodHertz(50);
  bodyServo.setPeriodHertz(50);

  leftHand.attach(13, 500, 2400);
  rightHand.attach(12, 500, 2400);
  bodyServo.attach(14, 500, 2400);

  leftHand.write(90);
  rightHand.write(90);
  bodyServo.write(90);
}

// ===== LOOP =====
void loop() {

  checkVibration();
  updateState();
  updateAnimation();
  updateServos();
  drawEyes();

  delay(16);
}