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
void setOffAlarm() {
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

  // < en > wisselen
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
            Serial.println("Code is correct");
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
              OLED.print("Code is incorrect");
              setOffAlarm();
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
  if (digitalRead(BUTTON1) == HIGH) {
    Serial.println("cancel"); // BUT HOW DO I STOP IT? MOET TERUGKEREN NAAR "PRESS OK TO ARM"
    changingCode = false;
    codeChanged = false;
    return;
  } 
  
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

// TODO: cijfer dat je aan het invoeren bent laten knipperen (bij invoer & veranderen), herbruikbare functies*
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
    setOffAlarm();
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

int timeLeft = 100;
long countdownTime;
bool countdownStarted = false;

void loop() {
  toggleOnAndOff();
  int varDoorIsOpen = doorIsOpen();

  if (enteringCode == true) {
    varDoorIsOpen = true;
  }

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
    alarmGoesOffAfterCountdown(100000);

    if (!countdownStarted) {
      countdownTime = millis();
      countdownStarted = true;
    }
    
    checkCode();

    String timeLeftString = String(timeLeft - ((millis() - countdownTime) / 1000)); 

    // OLED.print("Input: " + String(getNumberFromPotentiometer()) + String("      | ") + String(timeLeftString));

    // 3 (lege) plekken om getal in te voeren 
    // bij elke OK getal opslaan en doorgaan naar de volgende lege plek
    // bij CANCEL opnieuw beginnen
    // delay aftellen naast de invoer

    String text = getCodeInterfaceText(getNumberFromPotentiometer(), timeLeftString);
    OLED.print(text);

//    for (i = 0; i < 3; i++)
//      if (userCode[0] == nog niet ingevuld && userCode[1] == nog niet ingevuld && userCode[2] == nog niet ingevuld) {
//        // Print dan 3 lege plekken, de eerste is getNumberFromPotentiometer
//      }
//
//      if (userCode[0] == ingevuld && userCode[1] == niet ingevuld && userCode[2] == niet ingevuld {
//        // Print dan firstNumber op de eerste plek, getNumberFromPotentiometer op tweede plek, leeg op derde
//      }
//
//      if (userCode[0] == ingevuld && userCode[1] == ingevuld && userCode[2] == niet ingevuld {
//        // Print dan firstNumber op de eerste plek, secondNumber op tweede, getNumberFromPotentiometer op derde 
//      }
//
//      if (userCode[0] == ingevuld && userCode[1] == ingevuld && userCode[2] == ingevuld {
//        // Dan gaat ie als het goed is naar "Code correct" in checkCode(), dus dan kan deze denk ik weg.
//      }
  }

  // DOOR CLOSED, ALARM ON
  if (varDoorIsOpen == false && alarmIsOn == true) {
    turnOnLEDAndTurnOthersOff(LED_GREEN);
    OLED.print("Armed");

    if (getNumberFromVolumesensor() > 800) {
      for(;;) {
        OLED.print("You're too loud");
        setOffAlarm();
      }
    }
  }
}

String getCodeInterfaceText(int potentioMeterNumber, String timeLeftString) {
  String text = "";
  for (int i = 0; i < 3; i++) {
    text += "[";
    if (currentCodeStep == i) {
      text += String(potentioMeterNumber);
    } else if (userCode[i] == -1) {
      text += "_";
    } else {
      text += String(userCode[i]);
    }
    text += "]";
  }

  if (timeLeftString == NULL) {
     return text;
  }
  return text + "  Time: " + timeLeftString;
//  return "[" + String(potentioMeterNumber) + "]" + "[" + String(potentioMeterNumber) + 
//    "]" + "[" + String(potentioMeterNumber) + "]" + "  Time: " + timeLeftString;
}

// TODO: reformat
