/*

Remote controller for ATA project

BLUETOOTH ADDR:98d3:33:809944


*/


#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h> 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define setPressureLOW 14
#define setPressureHIGH 15
#define switchLockLOW 8

#define setPressure A0   // Potentiometer to set pressure
#define switchLock 7     // Switch for airing
#define buzzerPin 9
#define btStatePin 6       // HC-05 state pin

#define sUp 1
#define sDown 2
#define sFinished 3
#define sMeasure 4
#define sReady 5
#define sAvg 200
#define writeWindow 500


//Define Variables we'll be connecting to
double newSetpoint;
double tnewSetpoint;
unsigned long setTime, setTime2 = 0, sMeasureTime = 0, tTemp, tFinalTemp, debugTime = 0;

unsigned int tError;
int tSetpoint, tInput=0, tSetpointTemp, tInputTemp;

int currentSetPressure;
bool switchLockState, switchLockStateCheck=0 , btState;
int sensVal;
int sStatus = 0;

void setup() {
   Serial.begin (9600);
   Serial1.begin(9600);
   while(!Serial1);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
   Serial.println("SSD1306 allocation failed");
   for(;;); // Don't proceed, loop forever
  }

  pinMode (setPressure, INPUT);
  pinMode (switchLock, INPUT_PULLUP);
  pinMode (btStatePin, INPUT);
  pinMode (buzzerPin, OUTPUT);
  pinMode (setPressureHIGH, OUTPUT);
  pinMode (setPressureLOW, OUTPUT);
  pinMode (switchLockLOW, OUTPUT);
  digitalWrite(setPressureHIGH, HIGH);
  digitalWrite(setPressureLOW, LOW);
  digitalWrite(switchLockLOW, LOW);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Auto Tire Airing"); 
  display.println("Initialting Bluetooth"); 
  display.println(" "); 
  display.println("Connecting..."); 
  display.display();
  while (!digitalRead(btStatePin)){};
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println("CONNECTED!"); 
  display.display();
  delay (2000);



  switchLockState = digitalRead(switchLock);
  while (!switchLockState) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,10);
  display.println("Release"); 
  display.println(" the "); 
  display.println("switch!"); 
  display.display();
  switchLockState = digitalRead(switchLock);
  }
  
}


//////////////////////////////////////////       Functions          //////////////////////////////////////////

void beep (int x){
  if ((millis()/1000)%5){
    for (int i=1; i<=x; i++) {
      int y = i%2 ;
      digitalWrite(buzzerPin,y);
      delay(100);
    }
    digitalWrite(buzzerPin,0);
  }
}

void showPSI(){
   display.setCursor(0,9);
   display.setTextSize(1);
   display.println("SET       TIRE");
   display.setTextSize(2);
   display.setCursor(0,15);
   display.print((unsigned int)tSetpoint);
   display.setCursor(50,15);
   if (tInput > 65) display.print("ER");
   if (tInput < 65) display.print(tInput);
   display.display();
 }

void displayStatus(){
  display.clearDisplay();
  switch (sStatus){
    case sReady:
      display.setTextSize(1);
      display.setCursor(45,0);
      display.println("Ready!");
      showPSI();
      break;

    case sUp:
      display.setTextSize(1);
      display.setCursor(20,0);
      display.println("Working UP!");
      showPSI();
      break;

    case sDown:
       display.setTextSize(1);
       display.setCursor(20,0);
       display.println("Working DOWN!");
       showPSI();
      break;

    case sFinished:
        display.setTextSize(4);
        display.setCursor(0,0);
        display.println("DONE!");
        display.display();
        beep(6);
      break;

    case sMeasure:
        display.setTextSize(1);
        display.setCursor(30,0);
        display.println("Measuring...");
        showPSI();
      break;
    case 0:
        display.setTextSize(1);
        display.setCursor(30,0);
        display.println("Error");
        showPSI();
      break;
  }
}
  
  void checkBtState(){
    btState = digitalRead(btStatePin);
    while (!btState){
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println("LOST"); 
    display.println("CONNECTION"); 
    display.display();
    btState = digitalRead(btStatePin);
  }
}

int avgMeasure(){
  tTemp = 0;
  for (int i=0; i<sAvg; i++){
    sensVal = analogRead(setPressure);
    tTemp += sensVal;
  }
  tFinalTemp = tTemp/sAvg;
  return tFinalTemp;
}

void debug(){
  if (millis() - debugTime > 2000){
  Serial.print("tSetpointTemp: ");
  Serial.println(tSetpointTemp);
  Serial.print("tInput: ");
  Serial.println(tInput);
  Serial.print("tInputTemp: ");
  Serial.println(tInputTemp);
  Serial.print("sStatus: ");
  Serial.println(sStatus);
  Serial.println(" ");
  debugTime = millis();
  }
  return;
}

void readData(){
   if (Serial1.available()>1){
    tInputTemp = Serial1.read();                  ///////////////////////// Serial1 read
    delay(2);
    int tInputTemp2 = Serial1.read();                  ///////////////////////// Serial1 read
    delay(2);

    if (tInputTemp<100) tInput = tInputTemp;
      else if (tInputTemp2<100) tInput = tInputTemp2;
    if (tInputTemp>100) sStatus = tInputTemp-100;
      else if (tInputTemp2>100) sStatus = tInputTemp2-100;
  }
}

void writeData(){
    if (switchLockState != switchLockStateCheck){
    Serial1.write(tSetpointTemp);            ///////////////////// Serial1 write
    Serial1.flush();
    switchLockStateCheck = switchLockState;
    Serial.println("SENT!");
  }   
}

void setPot(){
  switchLockState = digitalRead(switchLock);
  if (switchLockState) currentSetPressure = avgMeasure();

  tSetpoint = map(currentSetPressure, 0, 1023, 0, 50);
   
  if (!switchLockState) tSetpointTemp = tSetpoint+100;
    Serial1.write(tSetpointTemp);            ///////////////////// Serial1 write
    Serial1.flush();
    switchLockStateCheck = switchLockState;
    Serial.println("SENT!");
  }

  if (Serial1.available()>1){
    tInputTemp = Serial1.read();                  ///////////////////////// Serial1 read
    delay(2);
    int tInputTemp2 = Serial1.read();                  ///////////////////////// Serial1 read
    delay(2);

    if (tInputTemp<100) tInput = tInputTemp;
    else if (tInputTemp2<100) tInput = tInputTemp2;
    if (tInputTemp>100) sStatus = tInputTemp-100;
    else if (tInputTemp2>100) sStatus = tInputTemp2-100;
  }
 
  displayStatus();   

  debug(); 
}
