
#include <LiquidCrystal_I2C.h>  // подключаем библу
#include "GyverEncoder.h"



// пины экрана
#define SDA 18
#define SCL 19
// пины энкодера
#define DT 3
#define SW 2
#define CLK 4 
// пины мотора
#define DIR_PIN 8
#define PWM_PIN 9


Encoder enc(CLK, DT, SW);  // для работы c кнопкой
LiquidCrystal_I2C lcd(0x27, 16, 2); // экран 16х2 

int value = 0;
int max = 255;
boolean flag = 0 ;

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
    
    
   

  
  
  
    
  


}
