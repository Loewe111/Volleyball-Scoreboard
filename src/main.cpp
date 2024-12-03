#include <Arduino.h>
#include <RCSwitch.h>
#include <PxMatrix.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/TomThumb.h>

#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 16
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

RCSwitch mySwitch = RCSwitch();
PxMATRIX display(64,32, P_LAT, P_OE, P_A, P_B, P_C, P_D);

void IRAM_ATTR display_updater(){
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(20);
  portEXIT_CRITICAL_ISR(&timerMux);
}

const uint16_t BLACK = display.color565(0, 0, 0);
const uint16_t SCORE = display.color565(255, 255, 255);
const uint16_t OLD_SCORE = display.color565(0, 64, 128);
const uint16_t SERVE = display.color565(0, 255, 0);
const uint16_t SET = display.color565(255, 0, 0);

void drawScore();

String formatNumber(int number){
  if (number < 10){
    return "0" + String(number);
  }
  return String(number);
}

bool winCondition(uint8_t points1, uint8_t points2){ // returns true if team 1 wins
  return points1 >= 25 && points1 - points2 >= 2;
}

bool anyWinCondition(uint8_t points1, uint8_t points2){ // returns true if any team wins
  return winCondition(points1, points2) || winCondition(points2, points1);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Starting...");
  display.begin(16);
  display.clearDisplay();

  // timer = timerBegin(0, 80, true);
  // timerAttachInterrupt(timer, &display_updater, true);
  // timerAlarmWrite(timer, 4000, true);
  // timerAlarmEnable(timer);
  
  mySwitch.enableReceive(17);

  drawScore();
}

uint8_t team_a_scores[5] = {0, 0, 0, 0, 0};
uint8_t team_a_set = 0;
uint8_t team_b_scores[5] = {0, 0, 0, 0, 0};
uint8_t team_b_set = 0;
uint8_t serve = 0; // 0 = team a, 1 = team b
uint8_t set = 0;

const uint8_t display_sets = 2;

unsigned long last_update = 0;

void nextSet(){
  if (winCondition(team_a_scores[set], team_b_scores[set])){
    team_a_set++;
    serve = 0;
  } else if (winCondition(team_b_scores[set], team_a_scores[set])){
    team_b_set++;
    serve = 1;
  }
  set++;
  team_a_scores[set] = 0;
  team_b_scores[set] = 0;
}

void drawScore(){
  display.clearDisplay();
  display.setFont(&FreeMonoBold12pt7b);
  display.setTextColor(SCORE);
  display.setCursor(1, 16);
  display.print(formatNumber(team_a_scores[set]));
  display.setCursor(35, 16);
  display.print(formatNumber(team_b_scores[set]));
  
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(SET);
  display.setCursor(1, 30);
  display.print(String(team_a_set));
  display.setCursor(51, 30);
  display.print(String(team_b_set));

  if (serve == 0){
    display.drawLine(0, 0, 0, 31, SERVE);
  } else {
    display.drawLine(63, 0, 63, 31, SERVE);
  }

  display.setFont(&TomThumb);
  display.setTextColor(OLD_SCORE);

  for (int i = 0; i < display_sets; i++){
    if (set - i > 0){
      display.setCursor(12, 24 + 6 * i);
      display.print(formatNumber(team_a_scores[set - i - 1]));
      display.setCursor(45, 24 + 6 * i);
      display.print(formatNumber(team_b_scores[set - i - 1]));
    }
  }
}

void loop() {
  if (mySwitch.available()){
    uint32_t value = mySwitch.getReceivedValue();
    uint16_t length = mySwitch.getReceivedBitlength();
    mySwitch.resetAvailable();
    Serial.println(value);
    if (length != 24 || millis() - last_update < 500){
      return;
    }
    value = value & 0b11;

    if ((value == 0 || value == 1) && anyWinCondition(team_a_scores[set], team_b_scores[set])){
      nextSet();
    } else {
      if (value == 0){
        team_a_scores[set]++;
        serve = 0;
      }
      if (value == 1){
        team_b_scores[set]++;
        serve = 1;
      }
    }

    if (value == 2){ // Reset
      team_a_scores[set] = 0;
      team_b_scores[set] = 0;
    }

    if (value == 3){ // Reset all
      for (int i = 0; i < 5; i++){
        team_a_scores[i] = 0;
        team_b_scores[i] = 0;
      }
      team_a_set = 0;
      team_b_set = 0;
      serve = 0;
      set = 0;
    }

    drawScore();

    last_update = millis();

  }

  display.display(20);
}
