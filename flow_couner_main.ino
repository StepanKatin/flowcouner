#include "GyverEncoder.h"
#include <LiquidCrystal_I2C.h>


// ОБЪЯВЛЯЕМ ВЫХОДЫ
#define HV_pwm 3
#define analog_signal_sbm 14
#define KEY 4
#define S2 5
#define S1 6
// пины экрана
#define SDA 15
#define SCL 16


// ОБЪЯВЛЯЕМ ВНУТРЕННИЕ ПЕРЕМЕННЫЕ И КОНСТАНТЫ
Encoder enc(S1, S2, KEY);
LiquidCrystal_I2C lcd(0x27, 16, 2); // экран 16х2 
int exsposition = 60000; //время измерения в мс
bool flag = false;
unsigned long current_time = millis();
int counts_from_sbm[5];
uint32_t counter = 0;
int min_ampl = 650;
int HV_duty = 16;
int sum_of_counts = 0;
float result_of_imp = 0.0;
float pump_speed = 0.0;


void setup() {
  enc.setType(TYPE2);
  Serial.begin(9600);
  pinMode(HV_pwm, OUTPUT);
  lcd.init();           // инициализация
  lcd.backlight();      // включить подсветку
  lcd.setCursor(1, 0);
  lcd.print("Имп/сек");
  lcd.setCursor(10, 0);
  lcd.print(pump_speed);


  
}

void loop() {
  analogWrite(HV_pwm, HV_duty);
	enc.tick();

  if (enc.isTurn()) { // если был совершён поворот (индикатор поворота в любую сторону)
    if (enc.isRight() && pump_speed < 255) pump_speed += 5;
    if (enc.isLeft()&& pump_speed > 0) pump_speed -=5;
    lcd.setCursor(10, 0);
    lcd.print(pump_speed);  // выводим значение при повороте
  }

  if (enc.isClick() == true) {flag = true;}

  if (flag == true){
    for (int i = 0; i < 5; i++){
      while (millis() - current_time < exsposition){
        if (analogRead(analog_signal_sbm) > min_ampl){counter = counter + 1;}
      }
      counts_from_sbm[i] = counter;
      Serial.println(counter);
      counter = 0;
    }
    for (int i=0; i<5; i++) {sum_of_counts += counts_from_sbm[i];}
    result_of_imp = sum_of_counts/(exsposition*0.001);
    lcd.setCursor(1, 1);
    lcd.print("N, имп/с");
    lcd.setCursor(10, 1);
    lcd.print(result_of_imp);
    
  }
  flag = false;
  current_time = millis();

  
}  

float value_error(int counts_from_sbm, int sum_of_counts){
  float diff_square = 0.0;
  float sko = 0.0;
  double error = 0.0;
  for (int i=0; i < 5; i++){
    float diff = counts_from_sbm[i] - (sum_of_counts/5); 
    diff_square += diff*diff;
  }
  sko = sqrtf(diff_square/4);
  error = floor(sko*2,78/counts_from_sbm)*100)
  return error;
  
}
 

