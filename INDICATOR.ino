/*
Indicator ECU Simulation
Hardware:
Left Button  -> D4
Right Button -> D5
Left LED     -> D8
Right LED    -> D10
*/

#define LEFT_BUTTON 4
#define RIGHT_BUTTON 5

#define LEFT_LED 8
#define RIGHT_LED 10

// Timing
#define SCHEDULER_INTERVAL 100
#define BLINK_INTERVAL 300
#define BUTTON_HOLD_TIME 1000
#define DEBOUNCE_TIME 50

unsigned long lastScheduler = 0;
unsigned long lastBlink = 0;

// Button tracking
bool leftPressed = false;
bool rightPressed = false;

unsigned long leftPressStart = 0;
unsigned long rightPressStart = 0;

unsigned long lastLeftDebounce = 0;
unsigned long lastRightDebounce = 0;

bool lastLeftState = HIGH;
bool lastRightState = HIGH;

// Indicator states
enum IndicatorState
{
  IDLE,
  LEFT_MODE,
  RIGHT_MODE,
  HAZARD_MODE
};

IndicatorState currentState = IDLE;

// LED states
bool leftLedState = LOW;
bool rightLedState = LOW;

void setup()
{
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);

  pinMode(LEFT_LED, OUTPUT);
  pinMode(RIGHT_LED, OUTPUT);

  Serial.begin(9600);

  Serial.println("Indicator ECU Started");
}

void loop()
{
  if (millis() - lastScheduler >= SCHEDULER_INTERVAL)
  {
    lastScheduler = millis();

    readButtons();
    updateIndicatorLogic();
  }

  blinkLEDs();
}

void readButtons()
{
  bool leftState = digitalRead(LEFT_BUTTON);
  bool rightState = digitalRead(RIGHT_BUTTON);

  // Debounce Left
  if (leftState != lastLeftState)
  {
    lastLeftDebounce = millis();
  }

  if ((millis() - lastLeftDebounce) > DEBOUNCE_TIME)
  {
    if (leftState == LOW && !leftPressed)
    {
      leftPressed = true;
      leftPressStart = millis();
      Serial.println("[EVENT] LEFT_BUTTON_PRESSED");
    }

    if (leftState == HIGH && leftPressed)
    {
      leftPressed = false;
    }
  }

  lastLeftState = leftState;

  // Debounce Right
  if (rightState != lastRightState)
  {
    lastRightDebounce = millis();
  }

  if ((millis() - lastRightDebounce) > DEBOUNCE_TIME)
  {
    if (rightState == LOW && !rightPressed)
    {
      rightPressed = true;
      rightPressStart = millis();
      Serial.println("[EVENT] RIGHT_BUTTON_PRESSED");
    }

    if (rightState == HIGH && rightPressed)
    {
      rightPressed = false;
    }
  }

  lastRightState = rightState;
}

bool rightActionDone = false;
bool leftActionDone = false;

void updateIndicatorLogic()
{
  bool leftHold = leftPressed && (millis() - leftPressStart >= 1000);
  bool rightHold = rightPressed && (millis() - rightPressStart >= 1000);

  // Reset trigger when button released
  if(!leftPressed)
    leftActionDone = false;

  if(!rightPressed)
    rightActionDone = false;

  // Hazard Mode
  if(leftPressed && rightPressed)
  {
    if(currentState != HAZARD_MODE)
    {
      currentState = HAZARD_MODE;
      Serial.println("[STATE] HAZARD_MODE_ON");
    }
    return;
  }

  switch(currentState)
  {
    case IDLE:

      if(rightHold && !rightActionDone)
      {
        currentState = RIGHT_MODE;
        rightActionDone = true;
        Serial.println("[STATE] RIGHT_INDICATOR_ON");
      }

      if(leftHold && !leftActionDone)
      {
        currentState = LEFT_MODE;
        leftActionDone = true;
        Serial.println("[STATE] LEFT_INDICATOR_ON");
      }

    break;

    case RIGHT_MODE:

      if(rightHold && !rightActionDone)
      {
        currentState = IDLE;
        rightActionDone = true;
        Serial.println("[STATE] RIGHT_INDICATOR_OFF");
      }

      if(leftHold && !leftActionDone)
      {
        currentState = LEFT_MODE;
        leftActionDone = true;
        Serial.println("[STATE] SWITCH_TO_LEFT");
      }

    break;

    case LEFT_MODE:

      if(leftHold && !leftActionDone)
      {
        currentState = IDLE;
        leftActionDone = true;
        Serial.println("[STATE] LEFT_INDICATOR_OFF");
      }

      if(rightHold && !rightActionDone)
      {
        currentState = RIGHT_MODE;
        rightActionDone = true;
        Serial.println("[STATE] SWITCH_TO_RIGHT");
      }

    break;

    case HAZARD_MODE:

      if((leftHold || rightHold) && !leftActionDone && !rightActionDone)
      {
        currentState = IDLE;
        Serial.println("[STATE] HAZARD_MODE_OFF");
        leftActionDone = true;
        rightActionDone = true;
      }

    break;
  }
}

void blinkLEDs()
{
  if (millis() - lastBlink < BLINK_INTERVAL)
    return;

  lastBlink = millis();

  switch (currentState)
  {
    case IDLE:

      digitalWrite(LEFT_LED, LOW);
      digitalWrite(RIGHT_LED, LOW);
      break;

    case LEFT_MODE:

      leftLedState = !leftLedState;
      digitalWrite(LEFT_LED, leftLedState);
      digitalWrite(RIGHT_LED, LOW);
      break;

    case RIGHT_MODE:

      rightLedState = !rightLedState;
      digitalWrite(RIGHT_LED, rightLedState);
      digitalWrite(LEFT_LED, LOW);
      break;

    case HAZARD_MODE:

      leftLedState = !leftLedState;
      rightLedState = leftLedState;

      digitalWrite(LEFT_LED, leftLedState);
      digitalWrite(RIGHT_LED, rightLedState);

      break;
  }
}