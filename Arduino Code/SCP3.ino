#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>
#include <Keypad.h>
#include <Wire.h>
#include <Servo.h>

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#define SLEEP_POWER_DOWN 2
#define ALL_OFF 3
#define ALL_ON 4

//=========WDT SETUP===================
void WDT_interrupt_enable(uint8_t timeout_v) {
  unsigned char bakSREG;
  uint8_t prescaler;

  prescaler = timeout_v & 0x07;
  prescaler |= (1 << WDIE);
  if (timeout_v > 7)
    prescaler |= (1 << WDP3);

  bakSREG = SREG;
  cli();
  wdt_reset();
  WDTCSR |= ((1 << WDCE) | (1 << WDE));
  WDTCSR = prescaler;
  SREG = bakSREG;
}


void WDT__enable(uint8_t timeout_v) {
  unsigned char bakSREG;
  uint8_t prescaler;

  prescaler = timeout_v & 0x07;
  prescaler |= (1 << WDE);
  if (timeout_v > 7)
    prescaler |= (1 << WDP3);

  bakSREG = SREG;
  cli();
  wdt_reset();
  WDTCSR |= ((1 << WDCE) | (1 << WDE));
  WDTCSR = prescaler;

  SREG = bakSREG;
}

void WDT__disable(void) {
  uint8_t bakSREG;
  bakSREG = SREG;
  cli();
  wdt_reset();
  MCUSR &= ~(1 << WDRF);  //?????????????? WDRF ????????????
  WDTCSR |= ((1 << WDCE) | (1 << WDE));
  WDTCSR = (0 << WDE) | (0 << WDIE);
  SREG = bakSREG;
}

void SLEEP_DISABLE(void) {
  SMCR &= 0xFE;
}

void SLEEP_INITIALIZE(uint8_t m) {
  SMCR = (m << 1) | 0x01;
}

//=====================================
String receivedData = "";
static unsigned long lastButtonPressTime = 0;
static unsigned long lastTimeSendState = 0;
int irReceiverPin = 2;
int IRval = 0;

//--------------------------------
//ultrasonic for Car-Check
const int triPin_1 = 3;
const int echoPin_1 = 4;
//-------------------------------
volatile bool admin_state;
volatile bool lock_state;
volatile bool car_state;
volatile bool d_time = true;
volatile int dst_car;
//--------------------------------
#define I2CADDR 0x20
//--------------------------------
Servo servo;  // Create a servo object to control a servo motor
//--------------------------------
const int led = 9;
const int SVpin = 8;
//--------------------------------
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 0, 1, 2, 3 };
byte colPins[COLS] = { 4, 5, 6, 7 };

LiquidCrystal_I2C lcd(0x27, 16, 2);
Keypad_I2C keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR);

char password[7] = "123456";
char enteredPassword[7] = "";
int incorrectAttempts = 0;
//-----------------------------------
//-----------------------------------
void I2C_bus_scan(void);
void clearEnteredPassword();
//-----------------------------------
int sensorValue() {
  long duration, distance_cm;
  digitalWrite(triPin_1, LOW);
  delayMicroseconds(2);
  digitalWrite(triPin_1, HIGH);
  delayMicroseconds(10);
  digitalWrite(triPin_1, LOW);

  duration = pulseIn(echoPin_1, HIGH);

  distance_cm = duration / 58.0;
  return distance_cm;
}
bool distance_ck(int dst) {
  bool state = false;
  if (dst < 10) {
    state = true;
  }
  return state;
}
void Func_delay(int tm) {
  unsigned long previous_time_1 = millis();
  while (1) {
    while (millis() - previous_time_1 < tm) {
      ;
    }
    break;
  }
  return 0;
}
//==================================
void LCD_Wake() {
  //Serial.println("Interrupted");
  lastButtonPressTime = millis();
}
//------------------------------------
void setup() {
  pinMode(irReceiverPin, INPUT);
  //----------------------
  //ultrasonic_pin_set
  pinMode(triPin_1, OUTPUT);
  pinMode(echoPin_1, INPUT);
  //--------------------
  servo.attach(SVpin);
  servo.write(0);
  //--------------------
  Wire.begin();
  keypad.begin();
  Serial.begin(9600);
  //I2C_bus_scan();
  //Serial.println(password);
  //--------------------
  pinMode(led, OUTPUT);
  //--------------------
  lcd.init();
  lcd.backlight();
  lcd.home();
  lcd.print("Enter Password");
  lcd.setCursor(0, 1);

  attachInterrupt(digitalPinToInterrupt(irReceiverPin), LCD_Wake, CHANGE);
  strcpy(password, generateRandomPassword());

  WDT_interrupt_enable(6);
}

//=======================WDT==================

ISR(WDT_vect) {
  dst_car = sensorValue();
  car_state = distance_ck(dst_car);
  Serial.println(car_state);  //Send car_state to Odroid 
}


//============================================

void loop() {
  strcpy(password, generateRandomPassword());

  IRval = digitalRead(irReceiverPin);
  char key = keypad.getKey();
  if (key != NO_KEY) {
    lastButtonPressTime = millis();
    switch (key) {
      case 'C':
        if (lock_state) {
          unlock_();
          //resetPassword();
        }
        if (lock_state == true) {
          lock_state = false;
          d_time = true;
          //resetPassword();
        } else {
          ;
        }
        //resetPassword();
        break;

      case 'B':
        clearLCD();
        resetUserState();
        if(servo.read() > 0){
          toggleServoPosition();
        }
        break;

      case 'A':
        if (admin_state) {
          toggleServoPosition();
        }
        break;

      case 'D':
        if (admin_state) {
          resetPassword();
          //Send state 3 to Odroid 
          Serial.println(3);
          Serial.println(3);
          Serial.println(3);
        }
        break;
        

      case '*':
        deleteLastCharacterFromPassword();
        break;

      case '#':
        if (checkPassword(enteredPassword)) {
          handleCorrectPassword();
        } else if (checkAdminPassword(enteredPassword)) {
          enterAdminMode();
        } else {
          handleIncorrectPassword();
        }
        break;

      default:
        if (strlen(enteredPassword) < 6) {
          enteredPassword[strlen(enteredPassword)] = key;
        }
        break;
    }

    updateLCD();
  }

  if (IRval == 1 && (millis() - lastButtonPressTime >= 30000)) {
    lcd.noBacklight();
    SLEEP_INITIALIZE(SLEEP_POWER_DOWN);
    sleep_cpu();
    SLEEP_DISABLE();
    lcd.backlight();
  }
}

void resetPassword() {
  for (int i = 0; i > 20; i++) {
    Serial.println(3);
  }
  Func_delay(100);
  strcpy(password, generateRandomPassword());
  Func_delay(100);
  lock_state = false;
}

void clearLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password");
  admin_state = false;
}

void resetUserState() {
  clearEnteredPassword();
  incorrectAttempts = 0;
}

void toggleServoPosition() {
  if (servo.read() == 0) {
    for (float i = 0; i < 90; i += 0.5) {
      Func_delay(20);
      servo.write(i);
    }
  } else {
    for (float i = 90.0; i > 0; i -= 0.5) {
      Func_delay(20);
      servo.write(i);
    }
  }
}

void deleteLastCharacterFromPassword() {
  if (strlen(enteredPassword) > 0) {
    enteredPassword[strlen(enteredPassword) - 1] = '\0';
  }
}

bool checkPassword(const char* entered) {
  return strcmp(entered, password) == 0;
}

bool checkAdminPassword(const char* entered) {
  return strcmp(entered, "420604") == 0;
}

void handleCorrectPassword() {
  //Serial.println("Correct Password:");
  //Serial.println(password);
  clearEnteredPassword();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Successed");
  for (float i = 0; i < 90.0; i += 0.5) {
    Func_delay(20);
    servo.write(i);
  }
  //-------------------------------------------------------
  while (1) {
    // Serial.println(dst_car);
    // Serial.println(car_state);
    //----------------------------------------------------
    while (d_time) {
      dst_car = sensorValue();
      car_state = distance_ck(dst_car);
      //Serial.println(car_state);
      if (car_state == true) {
        //Send state 1 to Odroid 
        Serial.write(1);
        Serial.write(1);
        Serial.write(1);
        digitalWrite(led, HIGH);
        Func_delay(5000);
        digitalWrite(led, LOW);
        int check_again = sensorValue();
        bool check_state = distance_ck(check_again);
        //Serial.println(check_state);
        if (check_state == true) {
          d_time = false;
          //Send state 1 to Odroid 
          Serial.write(1);
          Serial.write(1);
          Serial.write(1);
          break;
        } else {
          Func_delay(20);
          car_state = false;
          //Send state 0 to Odroid 
          Serial.write(0);
          Serial.write(0);
          Serial.write(0);
        }
      } else {
        Func_delay(20);
      }
    }
    if (d_time == false) {
      digitalWrite(led, HIGH);
      Func_delay(2000);
      for (float i = 90.0; i > 0; i -= 0.5) {
        Func_delay(20);
        servo.write(i);
      }
      break;
    } else {
      Func_delay(20);
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password");
  lcd.setCursor(0, 1);
  lcd.print(password);
  incorrectAttempts = 0;
  lock_state = true;
}

void enterAdminMode() {
  admin_state = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Admin State");
  clearEnteredPassword();
  // Return or perform other admin-related actions
}

void handleIncorrectPassword() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Incorrect");
  //Serial.println("Incorrect Password:");
  //Serial.println(enteredPassword);
  clearEnteredPassword();
  Func_delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password");
  lcd.setCursor(0, 1);
  incorrectAttempts++;
  lock_state = false;

  if (incorrectAttempts >= 5) {
    resetPassword();
    //Send state 3 to Odroid
    Serial.println(3);
    Serial.println(3);
    Serial.println(3);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Password Changed");
    //Serial.println("New Password:");
    //Serial.println(password);
    Func_delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter Password");
    lcd.setCursor(0, 1);
    incorrectAttempts = 0;
  }
}

void updateLCD() {
  lcd.setCursor(0, 1);
  lcd.print("            ");  // Clear the second line
  lcd.setCursor(0, 1);
  lcd.print(enteredPassword);
}

String readDataFromOdroid() { //Read passowrd from Odroid
  // Check if data is available to read
  while (Serial.available() > 0) {
    // Read the incoming byte
    char receivedChar = Serial.read();

    // Append the received character to the string
    receivedData += receivedChar;
  }

  // Check if we have received 6 characters
  if (receivedData.length() == 6) {
    return receivedData;
  } else {
    // Return an empty string if we haven't received 6 characters yet
    return password;
  }
}

void convertStringToCharArray(String str, char charArray[]) {
  str.toCharArray(charArray, 7);  // Assumes 6 digits plus null-terminator
}

char* generateRandomPassword() {
  static char newPassword[7];
  String receivedNumber = readDataFromOdroid();
  convertStringToCharArray(receivedNumber, newPassword);
  return newPassword;
}

void clearEnteredPassword() {
  for (int i = 0; i < 7; i++) {
    enteredPassword[strlen(enteredPassword) - 1] = '\0';
  }
}


void I2C_bus_scan(void) {
  Serial.println();
  Serial.println("www.9arduino.com ...");
  Serial.println("I2C scanner. Scanning ...");
  byte count = 0;
  Wire.begin();
  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found address: ");
      Serial.print(i, DEC);
      Serial.print(" (0x");
      Serial.print(i, HEX);
      Serial.println(")");
      count++;
      Func_delay(1);
    }
  }
  Serial.println("Done.");
  Serial.print("Found ");
  Serial.print(count, DEC);
  Serial.println(" device(s).");
}
void unlock_() {
  for (float i = 0; i < 90; i += 0.5) {
    Func_delay(20);
    servo.write(i);
  }
  while (1) {
    int n_dst = sensorValue();
    bool nw_state;
    if (n_dst < 15) {
      nw_state = false;
    } else {
      nw_state = true;
    }
    if (nw_state) {
      digitalWrite(led, HIGH);
      Func_delay(20);
      //Serial.print("logic = ");
      //Serial.println(n_dst);
    } else {
      digitalWrite(led, LOW);
      Func_delay(5000);
      for (float i = 90.0; i >- 0; i -= 0.5) {
        Func_delay(20);
        servo.write(i);
      }
      //Send state 3 to Odroid 
      Serial.println(3);
      Serial.println(3);
      Serial.println(3);
      break;
    }
  }
}