#include "GyverEncoder.h"

// ОБЪЯВЛЯЕМ ВЫХОДЫ
#define HV_pwm 3
#define analog_signal_sbm 14
#define KEY 4
#define S2 5
#define S1 6 

// ОБЪЯВЛЯЕМ ВНУТРЕННИЕ ПЕРЕМЕННЫЕ И КОНСТАНТЫ
Encoder enc(S1, S2, KEY);
int exsposition = 60000; //время измерения в мс
bool flag = false;
unsigned long current_time = millis();
int counts_from_sbm[5];
uint32_t counter = 0;
int min_ampl = 650;
int HV_duty = 16;
int sum_of_counts = 0;
float result_of_imp = 0.0;


void setup() {
  enc.setType(TYPE2);
  Serial.begin(9600);
  pinMode(HV_pwm, OUTPUT);


  
}

void loop() {
  analogWrite(HV_pwm, HV_duty);
	enc.tick();
    
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
    Serial.print("N, имп/с");
    Serial.print(result_of_imp);  
    
  }
  flag = false;
  current_time = millis();




  

   

  
}  

  
 

