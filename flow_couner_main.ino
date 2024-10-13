#include "GyverPWM.h"
#include <LiquidCrystal_I2C.h>  // подключаем библу
#include "GyverEncoder.h"

// ОБЪЯВЛЯЕМ ВЫХОДЫ
// пины экрана
#define SDA 18 //пины А4
#define SCL 19 //пины А5
// пины энкодера
#define S1_ENC 8
#define S2_ENC 9
#define KEY_ENC 10 
// пины мотора
#define DIR_MOTOR 5
#define PWM_MOTOR 6

// ОБЪЯВЛЯЕМ ВНУТРЕННИЕ ПЕРЕМЕННЫЕ И КОНСТАНТЫ

#define SKVASHN = 500 // скважность, треубется подобрать что бы на СБМ поступало 400В

volatile unsigned long counts_from_SBM = 0;  // счетчик импульсов
bool isRecording = false;  // Флаг записи показаний
unsigned long startTime = 0; // Время начала записи
const unsigned long recordingDuration = 60000; // Длительность записи (60 секунд), устанавливается в зависимости от скорости мотора и диаметра тру

Encoder enc(KEY_ENC, S1_ENC, S2_ENC);  // для работы c кнопкой

LiquidCrystal_I2C lcd(0x27, 16, 2); // экран 16х2 


void setup() {
  Serial.begin(9600);
  enc.setType(TYPE2);   
  lcd.init();           // инициализация
  lcd.backlight();      // включить подсветку
  lcd.setCursor(1, 0);
  lcd.print("PWM");
  
  lcd.setCursor(10, 0);
  lcd.print(value);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);

  TCCR1A = 0b00000001;  // 8bit
  TCCR1B = 0b00001001;  // x1 fast pwm

}

void loop() {
	// обязательная функция отработки. Должна постоянно опрашиваться
  enc.tick();
  
  

  if (enc.isTurn()) { // если был совершён поворот (индикатор поворота в любую сторону)
      if (enc.isRight() && value < max) value += 5;
      if (enc.isLeft()&& value > 0) value -=5;
      lcd.setCursor(10, 0);
      lcd.print(value);  // выводим значение при повороте
  }
  
  analogWrite(PWM_PIN, value);
    
  digitalWrite(DIR_PIN, HIGH);  

