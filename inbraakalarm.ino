#include "s4d_breadboard.h"

int magnetStartingValue;
int securityCode[3] = {1, 1, 1};

void setup() {
  initializeBreadboard();
  Serial.begin(9600);
  Serial.println("setup");
  magnetStartingValue = analogRead(MAGNETSENSOR);
}

long time = 0;
long debounce = 200;
int previousAlarmIsOn = false;
int alarmIsOn = false;

// CODES
int userCode[3] = {-1, -1, -1};
int currentCodeStep = 0;
int currentChangeCodeStep = 0;
bool enteringCode = false;
bool changingCode = false;

// TURN ALARM ON OR OFF
int toggleOnAndOff() {
  if (enteringCode || changingCode) return;
  int buttonReading = digitalRead(BUTTON1);
  
  if (buttonReading == HIGH && previousAlarmIsOn == false && millis() - time > debounce) {
    if (alarmIsOn) {
      alarmIsOn = false;
    } else {
      alarmIsOn = true;
    }

    time = millis();    
  }

  previousAlarmIsOn = buttonReading;
}

// ALARM
void alarm() {
  digitalWrite(LED_RED, 0);
  //digitalWrite(BUZZER, 1);
  digitalWrite(LED_BLUE, 255);
  delay(100);
  digitalWrite(LED_GREEN, 255);
  delay(100);
  digitalWrite(LED_YELLOW, 255);
  delay(100);
  digitalWrite(LED_RED, 255);
  delay(100);
  digitalWrite(LED_BLUE, 0);
  delay(100);
  digitalWrite(LED_GREEN, 0);
  delay(100);
  digitalWrite(LED_YELLOW, 0);
  delay(100);
}

// CHECK IF DOOR IS OPEN
int doorIsOpen() {
  int magnetValue = analogRead(MAGNETSENSOR);
  
  if (magnetValue < magnetStartingValue - 25 || magnetValue > magnetStartingValue + 25) {
    return true;
   } else {
    return false;
   }
}

// TURN ON LED
int turnOnLED(int LEDColor) {
  analogWrite(LEDColor, 255);
}

// TURN OFF LED
int turnOffLED(int previousLEDColor) {
  analogWrite(previousLEDColor, 0);
}

// TURN ON ONE LED AND TURN ALL OTHERS OFF
int turnOnLEDAndTurnOthersOff(int LEDColor) {
  turnOffLED(LED_BLUE);
  turnOffLED(LED_GREEN);
  turnOffLED(LED_RED);
  turnOffLED(LED_YELLOW);

  turnOnLED(LEDColor);
}

bool canClickAgain = true;

// COMPARE CODE USER FILLS IN TO SET SECURITY CODE
void checkCode() {
  enteringCode = true;
  int currentNumber = getNumberFromPotentiometer();

  if (digitalRead(BUTTON1) == LOW) {
    canClickAgain = true;
    return;
  }

  if (canClickAgain && millis() - time > debounce) {
    canClickAgain = false;
    Serial.println("Entering number: " + String(currentNumber));

    for (int i = 0; i < sizeof(securityCode); i++) {
      if (currentCodeStep == i && currentNumber == securityCode[i]) {
        Serial.println("Number is correct");
        userCode[i] = currentNumber;
        currentCodeStep++;

        for (int j = 0; j < 3; j++) {
          if (userCode[j] == securityCode[j] && currentCodeStep == 3) {
            Serial.println("code correct");
            alarmIsOn = false;
          } 
        }
        break; 
      }

      if (currentCodeStep == i && currentNumber != securityCode[i]) {
        Serial.println("Nummber is incorrect");
        userCode[i] = currentNumber;
        currentCodeStep++;

        for (int j = 0; j < 3; j++) {
          if (userCode[j] != securityCode[j] && currentCodeStep == 3) {
            for(;;) {
              OLED.print("Code incorrect");
              alarm();
            }
          }
        }
        break;
      }
    }
  }
}

bool codeChanged = false;
bool canStartInputNewCode = false;

// PRESS CANCEL BUTTON TO CHANGE SECURITY CODE
int changeCode() {
  changingCode = true;
  int newCodeNumber = getNumberFromPotentiometer();
  OLED.print("Change code: " + String(currentChangeCodeStep) + "=" + String(newCodeNumber));
  
  if (digitalRead(BUTTON2) == LOW) {
    canStartInputNewCode = true;
    canClickAgain = true;
    return;
  }

  if (!canStartInputNewCode) {
    return;
  }
  
  canClickAgain = false;

  if (!canClickAgain && millis() - time > debounce) {
    securityCode[currentChangeCodeStep] = newCodeNumber;
    Serial.println("New code for step " + String(currentChangeCodeStep) +" = " + String(newCodeNumber));
    currentChangeCodeStep++;
    time = millis();

    if (currentChangeCodeStep == 3) {
      codeChanged = true;
      changingCode = false;
      Serial.println("Code changed");
    }
  }
}

// TODO: cijfer dat je aan het invoeren bent laten knipperen, herbruikbare functies*
// TODO: cancel knop werkend maken, pas na invoeren laatste cijfer wordt code veranderd*
// TODO: switch, wanneer magneet dichtbij is is juist deur dicht

bool canSetDelay = true;
long startTime;

// SET DELAY AND MAKE ALARM GO OFF AFTER DELAY
void alarmGoesOffAfterCountdown(int timeToWait) {
  if (canSetDelay) {
      startTime = millis();
      canSetDelay = false;
  }

  while(millis() > startTime + timeToWait) {
    OLED.print("Time's up!");
    alarm();
  }
}

// GET MAPPED NUMBER FROM POTENTIOMETER
int getNumberFromPotentiometer() {
  int potValue = analogRead(POTENTIOMETER);
  int mappedPotValue = map(potValue, 0, 1023, 0, 10);
  return mappedPotValue;
}

// GET NUMBER FROM VOLUMESENSOR
int getNumberFromVolumesensor() {
  int volumeValue = analogRead(VOLUMESENSOR);
  return volumeValue;
}

int timeLeft = 5;
long countdownTime;
bool countdownStarted = false;

void loop() {
  toggleOnAndOff();
  int varDoorIsOpen = doorIsOpen();

  // DOOR CLOSED, ALARM OFF
  if (varDoorIsOpen == false && alarmIsOn == false) {
    turnOnLEDAndTurnOthersOff(LED_YELLOW);
    if (!changingCode) {
      OLED.print("Press OK to arm");
    }

    if ((digitalRead(BUTTON2) == HIGH && codeChanged == false) || (changingCode && codeChanged == false)) {
      changeCode();
    }
  }

  // DOOR OPEN, ALARM OFF
  if (varDoorIsOpen == true && alarmIsOn == false) {
    turnOnLEDAndTurnOthersOff(LED_BLUE);
    OLED.print("Alarm off");
  }

  // DOOR OPEN, ALARM ON
  if (varDoorIsOpen == true && alarmIsOn == true) {
    turnOnLEDAndTurnOthersOff(LED_RED);
    alarmGoesOffAfterCountdown(5000);

    if (!countdownStarted) {
      countdownTime = millis();
      countdownStarted = true;
    }
    
    checkCode();

    int timeLeftString = timeLeft - ((millis() - countdownTime) / 1000); 

    OLED.print("Input: " + String(getNumberFromPotentiometer()) + String("      | ") + String(timeLeftString));
  }

  // DOOR CLOSED, ALARM ON
  if (varDoorIsOpen == false && alarmIsOn == true) {
    turnOnLEDAndTurnOthersOff(LED_GREEN);
    OLED.print("Armed");

    if (getNumberFromVolumesensor() > 800) {
      for(;;) {
        OLED.print("You're too loud");
        alarm();
      }
    }
  }
}

// TODO: reformat
