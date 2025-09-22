// ================== LIBS ==================
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <EncButton.h>


#include "RI_logo_image_120x120.h" // должен задавать image_data_120x120, IMG_WIDTH, IMG_HEIGHT


// ================== ПИНЫ ЭКРАНА / SPI ==================
#define TFT_CS   5
#define TFT_RST  4       // если RST не подключен — ставь -1
#define TFT_DC   2
#define SDA      23      // MOSI
#define SCK      18      // SCK
#define MISO_PIN -1      // MISO не нужен дисплею

// Подсветка (BL/LED) дисплея — ОБЯЗАТЕЛЬНО подключить
#define BL_PIN         12    // выбери удобный GPIO (НЕ 12), или поставь -1 если BL на 3.3V напрямую
#define BL_ACTIVE_HIGH 1     // 1 — включается уровнем HIGH; 0 — уровнем LOW

// ================== ПРОЧИЕ ПИНЫ ==================
#define SBM_SIGNAL 36

#define SBM_SIGNAL 36

// Enc Pins
#define ENCODER_S1 34
#define ENCODER_S2 35
#define ENCODER_KEY 34

// Motor pins
#define MOTOR_PWM 27
#define MOTOR_DIR 26
#define PUMP_ONOFF 25

// ================== ОБЪЕКТЫ ==================
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
EncButton eb(ENCODER_S1, ENCODER_S2, ENCODER_KEY, INPUT, INPUT_PULLUP);

// ================== СОСТОЯНИЯ ==================
enum State { INIT, LOGO_SHOWN, UI_STARTED };
State appState = INIT;

// ================== ГЛОБАЛЫ ==================
bool pumpEnabled = false;
int  MAX_PUMP_SPEED = 255;
volatile int pumpSpeed = 0;
volatile int analog_counts = 0;
bool uiShown = false;
const int MIN_AMPLITUDE = 650;
volatile int EXPOS = 1;
const int MAX_EXPOS = 120;
const int MIN_EXPOS = 1;
bool expos_edit_mode = false;

// Кэш значений для избирательной перерисовки
int lastPumpSpeed = -1;
int lastAnalog = -1;
int lastHold = 0;
int lastClick = 0;
int lastEXPOSval = -1;

// Serial авторизация
bool serialConnected = false;
const char* RESP_KEY = "ggyes";
const char* AUTH_KEY = "g_group";

String serialRxBuffer = "";
unsigned long serialAuthStartTime = 0;
const unsigned long SERIAL_AUTH_TIMEOUT = 2000;
unsigned long lastSerialReceiveTime = 0;
const unsigned long SERIAL_DISCONNECT_TIMEOUT = 5000;

// Таймеры
unsigned long bootStartTime = 0;
unsigned long lastSerialSend = 0;
unsigned long lastClickTime = 0;
static bool waiting = false;
static unsigned long waitStart = 0;
unsigned long startTime = 0;
const unsigned long doubleClickInterval = 500;

// UI цвета
uint16_t PUMPON_color  = 0x9F2A;
uint16_t PUMPOFF_color = 0xC902;

// ================== СЕРИАЛ ==================
void checkSerialAuth() {
  if (serialConnected && (millis() - lastSerialReceiveTime > SERIAL_DISCONNECT_TIMEOUT)) {
    serialConnected = false;
  }
  if (serialConnected) return;

  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    lastSerialReceiveTime = millis();
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

// ================== UI ==================
void drawLogo() {
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRGBBitmap(20, 5, image_data_120x120, IMG_WIDTH, IMG_HEIGHT);
}

void drawUI() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  tft.setCursor(0, 5);
  tft.println(F("PWM:"));
  tft.setCursor(105, 5);
  tft.print(pumpSpeed);

  tft.setCursor(0, 35);
  tft.println(F("COUNTS:"));
  tft.setCursor(105, 35);
  tft.print(analog_counts);

  tft.setCursor(0, 65);
  tft.println(F("PUMP: "));
  tft.fillRoundRect(65, 65, 60, 20, 2, PUMPOFF_color);

  tft.setCursor(0, 95);
  tft.println(F("EXPOS: "));
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
  if (lastHold == 1) {
    tft.fillRoundRect(65, 65, 60, 20, 2, PUMPON_color);
  }
  if (lastHold >= 2) {
    tft.fillRoundRect(65, 65, 60, 20, 2, PUMPOFF_color);
  }
}

void updateEXPOSvalues() {
  tft.fillRect(105, 95, 70, 20, ST77XX_BLACK);
  tft.setCursor(105, 95);
  tft.print(EXPOS);
}

// ================== SETUP ==================
void setup() {
  Serial.begin(9600);
  eb.setEncType(EB_STEP4_LOW);
  eb.setHoldTimeout(500);

  // Подсветка — включаем ДО инициализации дисплея
#if (BL_PIN >= 0)
  pinMode(BL_PIN, OUTPUT);
  #if BL_ACTIVE_HIGH
    digitalWrite(BL_PIN, HIGH);
  #else
    digitalWrite(BL_PIN, LOW);
  #endif
#endif

  // Инициализация SPI с явными пинами (ESP32) + частота 8 МГц
  SPI.begin(SCK, MISO_PIN, SDA, TFT_CS);
  SPI.setFrequency(8000000);

  // Инициализация дисплея: 1.8" ST7735 128x160 — чаще всего BLACKTAB
  tft.initR(INITR_BLACKTAB);   // если «тихо» — попробуй INITR_GREENTAB или INITR_REDTAB
  tft.setRotation(1);

  bootStartTime = millis();
  startTime     = millis();

  // I/O
  pinMode(MOTOR_PWM, OUTPUT);
  pinMode(MOTOR_DIR, OUTPUT);
  pinMode(PUMP_ONOFF, OUTPUT);
  pinMode(SBM_SIGNAL, INPUT);
}

// ================== LOOP ==================
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
      if (now - bootStartTime > 10000) { // 10 сек заставки
        drawUI();          // один раз
        appState = UI_STARTED;
      }
      break;

    case UI_STARTED:
      // Регулировка PWM энкодером (если не редактируем EXPOS)
      if (!expos_edit_mode) {
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

      // Удержание — вкл/выкл помпы + индикация
      if (eb.hold()) {
        Serial.println("Pump on");
        lastHold++;
        updatePUMPstatus();

        pumpEnabled = !pumpEnabled;
        digitalWrite(PUMP_ONOFF, pumpEnabled ? HIGH : LOW);

        if (millis() - bootStartTime >= 500) {
          waiting = false;
        }

        pumpEnabled = !pumpEnabled;
        digitalWrite(PUMP_ONOFF, pumpEnabled ? HIGH : LOW);

        if (lastHold >= 2) {
          updatePUMPstatus();
          Serial.println("Pump off");
          lastHold = 0;
        }
      }

      // Режим подсчёта импульсов (примерно имитируем кликом)
      if (lastHold == 1) {
        if (analogRead(SBM_SIGNAL) < MIN_AMPLITUDE) {
          Serial.println(analogRead(SBM_SIGNAL));
          analog_counts++;
        }

        if (millis() - startTime >= (unsigned long)EXPOS * 1000UL) {
          sendSerialData();

          if (analog_counts != lastAnalog) {
            updateCOUNTValues();
            lastAnalog = analog_counts;
          }
          analog_counts = 0;
          startTime = now;
        }
      }

      // Режим двойного клика — переключаем редактирование EXPOS
      if (lastHold == 0) {
        if (eb.click()) {
          if (now - lastClickTime < doubleClickInterval) {
            lastClick++;
            if (lastClick == 2) {
              expos_edit_mode = !expos_edit_mode;
              lastClick = 0;
            }
          } else {
            lastClick = 1; // первый клик
          }
          lastClickTime = now;
        }
      }

      // Редактирование EXPOS энкодером
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
      break;
  }
}
