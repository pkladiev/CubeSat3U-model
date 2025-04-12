#define _LCD_TYPE 1
#define LED1 2
#define LED2 4
#include <LCD_1602_RUS_ALL.h>
#include <RBD_Timer.h>
#include <AmperkaServo.h>
#include <iarduino_Position_BMX055.h>
#include <SD.h>
#include <SPI.h>

LCD_1602_RUS lcd(0x27, 16, 2);
int on = 0;
int sec = 1;
int percentValue;
int lightValue;
bool busy = false;
bool sdOk = false;
bool showDataSD = false;
const float targetPitch = -1.9;
const float targetRoll = -4.2;   
const float Kp_servo = 2.0;  
const float tolerance = 15.0;

RBD::Timer stopwatch(1000);
RBD::Timer stopwatch3(3000);
RBD::Timer percentRead(10000);
RBD::Timer lightRead(400);
RBD::Timer displayTimer(1000);
RBD::Timer servoRunTimer(3000); 
RBD::Timer servoPauseTimer(1000);
bool isServoActive = false;

AmperkaServo servo1;
AmperkaServo servo2;
iarduino_Position_BMX055 imu(BMX);

int percent() {
  float rawValue = analogRead(A0);
  float percentValue = (rawValue - 11) / (1013 - 11) * 100;
  return percentValue;
}

void printBattery() {
  lcd.setCursor(4, 0);
  lcd.print("   ");
  lcd.setCursor(4, 0);
  lcd.print(percentValue, DEC);
  lcd.write(37);
}

void printLight() {
  lcd.setCursor(12, 0);
  lcd.print("    ");
  lcd.setCursor(12, 0);
  lcd.print(lightValue, DEC);
}

void correctPosition() {
  imu.read();
  float pitch = imu.axisX;  // Тангаж
  float roll = imu.axisY;   // Крен

  float pitchError = targetPitch - pitch;
  float rollError = targetRoll - roll;

  int pitchSpeed = -Kp_servo * pitchError;
  int rollSpeed = -Kp_servo * rollError;

  pitchSpeed = constrain(pitchSpeed, -255, 255);
  rollSpeed = constrain(rollSpeed, -255, 255);

  servo2.writeSpeed(pitchSpeed);
  servo1.writeSpeed(rollSpeed);

  if (abs(pitchError) < tolerance && abs(rollError) < tolerance) {
    Serial.println("Position OK");
  } else {
    Serial.println("Position WRONG");
  }

  delay(50);
}

String readImu() {
  String dataString;
  imu.read();
  dataString += "КУРС=";
  dataString += String(imu.axisZ);
  dataString += ", ";
  dataString += "ТАНГАЖ=";
  dataString += String(imu.axisX);
  dataString += ", ";
  dataString += "КРЕН=";
  dataString += String(imu.axisY);
  dataString += ", ";

  return dataString;
}

void saveData(String dataString) {
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println("Save OK");
  } else {
    Serial.println("Error opening datalog.txt");
  }
}

void setup() { 
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  Serial.begin(115200);
  imu.begin(&Wire);
  servo1.attach(11);
  servo2.attach(12);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("SD Init: ");
  if (!SD.begin(6)) {
    Serial.println("Card failed, or not present");
    lcd.print("FAIL");
  } else {
    Serial.println("Card initialized.");
    sdOk = true;
    lcd.print("PASS");
  }
  delay(2000);
  lcd.clear();
  lcd.print("Bat");
  lcd.setCursor(10, 0);
  lcd.print("L");
  digitalWrite(LED1, HIGH);
  servo1.writeSpeed(0);
  servo2.writeSpeed(0);
}

void loop() {
  percentValue = percent();
  lightValue = analogRead(A1);
  if(percentRead.onRestart()) {
    printBattery();
  }
  if(lightRead.onRestart()) {
    printLight();
  }
  if (Serial.available()) {
    String str = Serial.readString();
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(str);
  }
  
  String result = readImu();
  result += "LIGHT=";
  result += String(lightValue);
  result += "\n";
  
  if (stopwatch3.onRestart()) {
      Serial.println(result);
      if (sdOk) {
        lcd.setCursor(0, 1);
        lcd.print("DATA");
        lcd.write(126);
        lcd.print("SD");
        digitalWrite(LED2, HIGH);
        showDataSD = true;
        displayTimer.restart();
        saveData(result);
      }
  }

  if (showDataSD && displayTimer.onRestart()) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    digitalWrite(LED2, LOW);
    showDataSD = false;
  }
  correctPosition();
}
