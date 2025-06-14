//linbs

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <EncButton.h>

//logo file

#include "RI_logo_image_120x120.h"

// Screen Pins

#define TFT_CS   15
#define TFT_RST  4
#define TFT_DC   16
#define SDA      17   // MOSI
#define SCK      41   // SCK

// Enc Pins
#define SBM_SIGNAL 10
#define ENCODER_S1 11
#define ENCODER_S2 12
#define ENCODER_KEY 13

// Motor pins
#define MOTOR_PWM 5
#define MOTOR_DIR 6
#define PUMP_ONOFF 7

//Objects

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
EncButton eb(ENCODER_S1, ENCODER_S2, ENCODER_KEY, INPUT, INPUT_PULLUP);

enum State { INIT, LOGO_SHOWN, UI_STARTED };
State appState = INIT;

//Global vars

bool pumpEnabled = false;
int MAX_PUMP_SPEED = 255;
volatile int pumpSpeed = 0;
volatile int analog_counts = 0;
bool uiShown = false;
const int MIN_AMPLITUDE = 650;
volatile int EXPOS = 1;
const int MAX_EXPOS = 120;
const int MIN_EXPOS =1;
bool expos_edit_mode = false;

// Кэш значений

int lastPumpSpeed = -1;
int lastAnalog = -1;
int lastHold = 0;
int lastClick = 0;
int lastEXPOSval = -1;

// Последовательное соединение

bool serialConnected = false;
const char* RESP_KEY = "ggyes";
const char* AUTH_KEY = "g_group";

String serialRxBuffer = "";
unsigned long serialAuthStartTime = 0;
const unsigned long SERIAL_AUTH_TIMEOUT = 2000;
unsigned long lastSerialReceiveTime = 0;
const unsigned long SERIAL_DISCONNECT_TIMEOUT = 5000; // 15 секунд без данных

// Таймеры

unsigned long bootStartTime = 0;
unsigned long lastSerialSend = 0;
unsigned long lastClickTime = 0;
static bool waiting = false;
static unsigned long waitStart = 0;
unsigned long startTime = 0;
const unsigned long doubleClickInterval = 500;

//UI const
uint16_t PUMPON_color = 0x9F2A;
uint16_t PUMPOFF_color = 0xC902;
//Fuctions

//Serial port
void checkSerialAuth() {

 if (serialConnected && (millis() - lastSerialReceiveTime > SERIAL_DISCONNECT_TIMEOUT)) {
    // Serial.println("Serial disconnected due to timeout");
    serialConnected = false;
  }

  if (serialConnected) return;

  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    lastSerialReceiveTime = millis();  // обновляем таймер при любом приёме

    if (msg == AUTH_KEY) {
      Serial.println(RESP_KEY);
      serialConnected = true;
      Serial.println("AUTH OK");
    } else {
      Serial.println("AUTH FAIL");
    }
  }
}


void sendSerialData() {
  Serial.print("counts,");
  Serial.print(analog_counts);
  Serial.print(",time,");
  Serial.print(startTime);
}

//UI func

void drawLogo() {
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRGBBitmap(20, 5, image_data_120x120, IMG_WIDTH, IMG_HEIGHT);
}

void drawUI() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  tft.setCursor(0, 5);
  tft.println("PWM:");
  tft.setCursor(105, 5);
  tft.print(pumpSpeed);

  tft.setCursor(0, 35);
  tft.println("COUNTS:");
  tft.setCursor(105, 35);
  tft.print(analog_counts);

  tft.setCursor(0, 65);
  tft.println("PUMP: ");
  tft.fillRoundRect(65, 65, 60, 20, 2, PUMPOFF_color);

  tft.setCursor(0, 95);
  tft.println("EXPOS: ");
  tft.setCursor(105, 95);
  tft.print(EXPOS);
}

void updatePUMPValues() {
  tft.fillRect(105, 0, 50, 20, ST77XX_BLACK);
  tft.setCursor(105, 5);
  tft.print(pumpSpeed);
}

void updateCOUNTValues() {
  tft.fillRect(105, 35, 70, 20, ST77XX_BLACK);
  tft.setCursor(105, 35);
  tft.print(analog_counts);
}  

void updatePUMPstatus() {
  if (lastHold == 1){
    tft.fillRoundRect(65, 65, 60, 20, 2, PUMPON_color);
  }
  if (lastHold >= 2){
    tft.fillRoundRect(65, 65, 60, 20, 2, PUMPOFF_color);
  }
}

void updateEXPOSvalues() {
  tft.fillRect(105, 95, 70, 20, ST77XX_BLACK);
  tft.setCursor(105, 95);
  tft.print(EXPOS);
}

void setup() {

  Serial.begin(9600);
  eb.setEncType(EB_STEP4_LOW);
  eb.setHoldTimeout(500);

  SPI.begin(SCK, -1, SDA);  // SCK, MISO (не используется), MOSI
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  bootStartTime = millis();
  startTime = millis();

  // Motor control pins
  pinMode(MOTOR_PWM, OUTPUT);
  pinMode(MOTOR_DIR, OUTPUT);
  pinMode(PUMP_ONOFF, OUTPUT);
  
  // Input pins
  pinMode(SBM_SIGNAL, INPUT);

  
}

void loop() {

  checkSerialAuth();
  eb.tick();

  unsigned long now = millis();

  switch (appState) {
    case INIT:
      drawLogo();          // один раз
      appState = LOGO_SHOWN;
      break;

    case LOGO_SHOWN:
      if (now - bootStartTime > 10000) {
        drawUI();          // один раз
        appState = UI_STARTED;
      }
      break;
    
    case UI_STARTED:

    // if (serialConnected && millis() - lastSerialSend > 2000) {
    //   Serial.println("PING");
    //   lastSerialSend = millis();
    // }

    if (expos_edit_mode == false) {    

      if (eb.right() && pumpSpeed < MAX_PUMP_SPEED) {
        pumpSpeed += 5;
        analogWrite(MOTOR_PWM, pumpSpeed);
      }

      if (eb.left() && pumpSpeed > 0) {
        pumpSpeed -= 5;
        analogWrite(MOTOR_PWM, pumpSpeed);
      }

      if (pumpSpeed != lastPumpSpeed) {
        updatePUMPValues();
        lastPumpSpeed = pumpSpeed;
      }
    }
    

    if (eb.hold()) {
      Serial.println("Pump on");
      lastHold++;
      updatePUMPstatus();

      pumpEnabled = !pumpEnabled;
      // Serial.println(pumpEnabled);
      digitalWrite(PUMP_ONOFF, pumpEnabled ? HIGH : LOW);
      // digitalWrite(13, pumpEnabled ? HIGH : LOW);

      if (millis() - bootStartTime >= 500) {
      waiting = false;
      // Serial.println("Таймер сработал!");
      }

      pumpEnabled = !pumpEnabled;
      digitalWrite(PUMP_ONOFF, pumpEnabled ? HIGH : LOW);
      // digitalWrite(13, pumpEnabled ? HIGH : LOW);

      if (lastHold >=2){
        updatePUMPstatus();
        Serial.println("Pump off");
        lastHold = 0;
      }
    }

    if (lastHold == 1) {

      if (eb.click()) analog_counts++; // ЗАМЕНИТЬ НА АНАЛОГ РИД


      // if (analogRead(SBM_SIGNAL) > MIN_AMPLITUDE) {
      //   analog_counts++;
      // }

      if (millis() - startTime >= EXPOS*1000) {
        sendSerialData();
      // Таймер сработал — выводим результат
        if (analog_counts != lastAnalog) {
          updateCOUNTValues();
          
          lastAnalog = analog_counts;

        }
      // Сбрасываем счетчик и запускаем таймер заново
        analog_counts = 0;
        startTime = now;
      }
  }

    if (lastHold == 0) {

      if (eb.click()){

        if (now - lastClickTime < doubleClickInterval) {
          lastClick++;

          if (lastClick == 2) {
            expos_edit_mode = !expos_edit_mode;
            lastClick = 0;
          }

      } else {
        // Слишком долго между кликами — считаем его первым
          lastClick = 1;
        }

        lastClickTime = now;
      }

    }
      //Настройка экспозиции 
    if (expos_edit_mode) { 
        if (eb.right() && EXPOS < MAX_EXPOS) {
          EXPOS++;
          updateEXPOSvalues();
      }

      if (eb.left() && EXPOS > MIN_EXPOS) {
        EXPOS--;
        updateEXPOSvalues();

      }
    }
    


    
    }  
  }

  

