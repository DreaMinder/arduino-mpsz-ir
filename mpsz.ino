#include "CyberLib.h"

unsigned long time;
unsigned long oldTime;
unsigned int timeDelay;
unsigned int rpm;
unsigned int period;

boolean advanceOn = false;
boolean impulse;
float angle;
float function;
unsigned int corrector = 1;
unsigned int angleK = 1;

int sensor = 3; 
int strobe = 8; 
int transistor = 12;
int led = 13; 

void setup() {
    Serial.begin(9600);
    pinMode(A0, INPUT);
    digitalWrite(A0, HIGH); 
    pinMode(strobe, OUTPUT);
    pinMode(led, OUTPUT);
    pinMode(transistor, OUTPUT);
    pinMode(sensor,INPUT);
    digitalWrite(sensor, HIGH); 
    attachInterrupt(digitalPinToInterrupt(sensor), fire, CHANGE);
}

void loop() {

    if (Serial.available() > 0) {
          int message = Serial.read();
          if(message == 105) advanceOn = false;
          else if(message == 73) advanceOn = true;
          else if(message == 75) angleK = Serial.read()-48;
          else corrector = message-48;
    }

    /*if(rpm > 200){
      Serial.print("*A");Serial.print(angle);Serial.println("*");
      Serial.print("*R");Serial.print(rpm);Serial.println("*");/
      Serial.print("*S");Serial.print(rpm*0.0232);Serial.println("*");
      Serial.print("*T");Serial.print(Thermistor(analogRead(0)));Serial.println("*");
      Serial.println("*LR0G0B0*");
    } else {
      Serial.println("*LR46G209B26*");
      Serial.println("*R0*");
    }*/
Serial.println(rpm);
    delay(100);

}

void fire(){
    impulse = D3_Read; //какой сейчас фронт сигнала?
    if(rpm > 50){
      if(advanceOn) delay_us(timeDelay); //если коррекция включена, ждем кол-во микросекунд
      digitalWrite(transistor,impulse); //отправляем текущий фронт сигнала на коммутатор
      digitalWrite(led,impulse);
    }

    getRPM(); //обновляем обороты

    if(impulse){ //если фронт положительный, мигаем стробоскопом
      digitalWrite(strobe,HIGH);
      delay_us(100);
      digitalWrite(strobe,LOW);
    }
      
    function = (rpm/6000.0)*angleK; //линейная фнукция упозняющая УОЗ с возращстанием оборотов в зависимости от коэффициента
    angle = corrector+function; //сумарный угол упознения, функция+октан-корректор
    if(angle > 12) angle = 12; //если насчитали сильно большой угол, ограничиваем
    //if(!impulse) angle = angle+angleK; //компенсация кривой шторки
    timeDelay = angle * period / 180; //считаем какую задержку ждать при следующем приходе сигнала из датчика 
  }

void getRPM()
{   
    time = micros();
    period = (period + time - oldTime)/2; //усреднение показаний - средние ариф. предыдущего и текущего периода
    rpm = 30000000/period;
    oldTime = time;
}

float Thermistor(int Raw) 
{
    long Resistance; 
    float Resistor = 9000; 
    float Temp; 
    Resistance= (Resistor*Raw /(1024-Raw)); 
    Temp = log(Resistance); 
    Temp = 1 / (0.00102119 + (0.000222468 * Temp) + (0.000000133342 * Temp * Temp * Temp));                    
    return  Temp - 273.15 - 55;            
}
