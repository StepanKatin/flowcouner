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
// пины СБМ-20
#define SBM_21 16
#define PWM_SBM 12

// ОБЪЯВЛЯЕМ ВНУТРЕННИЕ ПЕРЕМЕННЫЕ И КОНСТАНТЫ

#define SKVASHN 500 // скважность, треубется подобрать что бы на СБМ поступало 400В

volatile unsigned long counts_from_SBM = 0;  // счетчик импульсов
bool isRecording = false;  // Флаг записи показаний
unsigned long long initialtime = millis();
unsigned long long startTime = 0; // Время начала записи
unsigned long long pumptime_start = 0; // Время начала прокачки мотора
const int pumping_time = 60000; // Время работы мотора
const int recordingDuration = 60000; // Длительность записи (60 секунд), устанавливается в зависимости от скорости мотора и диаметра тру

int value_pwm = 0;
const int max_pwm = 255;
boolean event_perticle = LOW;
Encoder enc(KEY_ENC, S1_ENC, S2_ENC);  // для работы c кнопкой

LiquidCrystal_I2C lcd(0x27, 16, 2); // экран 16х2 


void setup() {
  Serial.begin(9600);
  enc.setType(TYPE2);   
  lcd.init();           // инициализация
  lcd.backlight();      // включить подсветку
  lcd.setCursor(1, 0);
  lcd.print("PWM_MOTOR");
  
  lcd.setCursor(10, 0);
  lcd.print(value_pwm);
  pinMode(DIR_MOTOR, OUTPUT);
  pinMode(PWM_MOTOR, OUTPUT);
  pinMode(PWM_SBM, OUTPUT);
  PWM_16KHZ_D9(SKVASHN);  // ШИМ 16 кГц на пине D9, заполнение SKVASHN из 1023
  pinMode(SBM_21, INPUT_PULLUP); // подключили катод датчика на A2
  


  TCCR1A = 0b00000001;  // 8bit
  TCCR1B = 0b00001001;  // x1 fast pwm

  attachInterrupt(2, btnIsr, FALLING);

}

void btnIsr() {
  counts_from_SBM ++;
   
}

void loop() {
	// обязательная функция отработки. Должна постоянно опрашиваться
  enc.tick();
   

  if (enc.isTurn()) { // если был совершён поворот (индикатор поворота в любую сторону)
      if (enc.isRight() && value_pwm < max_pwm) value_pwm += 5;
      if (enc.isLeft()&& value_pwm > 0) value_pwm -=5;
      lcd.setCursor(10, 0);
      lcd.print(value_pwm);  // выводим значение при повороте
  }
  if (enc.isClick()) {
    pumptime_start = millis();
      if ((pumptime_start - initialtime) < pumping_time) {
      analogWrite(PWM_MOTOR, value_pwm);
  
      digitalWrite(DIR_MOTOR, HIGH);
      }
      if ((pumptime_start - initialtime) > pumping_time) {
      analogWrite(PWM_MOTOR, 0);
      isRecording = true; 

      }
    initialtime = pumptime_start;    
  } if (isRecording = true) {
    isRecording = false;
    counts_from_SBM = 0;
    startTime = millis();
    if ((startTime - initialtime) < recordingDuration){
      counts_from_SBM ++;
      analogWrite(PWM_MOTOR, value_pwm);
      digitalWrite(DIR_MOTOR, HIGH);
    }
    if ((startTime - initialtime) > recordingDuration){
      lcd.setCursor(10, 1);
      lcd.print(counts_from_SBM);
      counts_from_SBM = 0;
      analogWrite(PWM_MOTOR, value_pwm);

    }

  }
}  

  
 

