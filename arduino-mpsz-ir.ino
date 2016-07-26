#include "CyberLib.h"
#include "AccelStepper.h"
#include "CmdMessenger.h"
AccelStepper stepper(4, 8, 9, 10, 11);
CmdMessenger cmdMessenger = CmdMessenger(Serial);
enum  //команды для управления телефоном
{
  advance,
  cor,
  k 
};

volatile unsigned long time;
volatile unsigned long oldTime;
volatile unsigned long checkTime;
volatile unsigned long debounceTime;
volatile unsigned int timeDelay;
volatile unsigned int rpm;
volatile float period;
volatile float oldPeriod;
volatile float pressure;
volatile boolean impulse;
volatile boolean flag;

boolean phaze = false;
boolean advanceOn = true;
boolean initialized = false;
float angle;
float function;
unsigned int corrector = 1;
unsigned int angleK = 1;

int i;
int sensor = 3;
int strobe = 6;
int transistor = 12;
int led = 13;
int mapSensor = A2;

void setup() {
  Serial.begin(9600);
  cmdMessenger.printLfCr();
  cmdMessenger.attach(OnUnknownCommand);
  cmdMessenger.attach(advance, OnSwitch);
  cmdMessenger.attach(cor, OnCorrecting);
  cmdMessenger.attach(k, OnK);
  stepper.setMaxSpeed(100);
  
  pinMode(A0, INPUT);
  digitalWrite(A0, HIGH); //внутренняя подтяжка для термистора
  pinMode(strobe, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(transistor, OUTPUT);
  pinMode(sensor, INPUT);
  pinMode(mapSensor, INPUT);
  attachInterrupt(digitalPinToInterrupt(sensor), getRPM, CHANGE);
}

void loop() {
  //Serial.println(rpm);

  digitalWrite(led,phaze);
  cmdMessenger.feedinSerialData();
  
  //pressure = mapFloat(A2_Read, 10, 1000, 0, 18); // считывание ДАД


    if(micros()-time < 50000){ //отправка данных на телефон
       Serial.print("*A");Serial.print((int) angle);Serial.println("*");
       Serial.print("*R");Serial.print(rpm);Serial.println("*");
       Serial.print("*P");Serial.print(pressure);Serial.println("*");
       Serial.print("*S");Serial.print(rpm*0.0232);Serial.println("*");
       Serial.print("*T");Serial.print(Thermistor(analogRead(A0)));Serial.println("*");
       Serial.println("*LR0G0B0*");
     } else {
       Serial.println("*LR46G209B26*");
       Serial.println("*R0*");
     }
     
    stepper.moveTo(angleK*10); // уравление шаговым дозатором газа
    stepper.setSpeed(50);
    stepper.runSpeedToPosition();
     
    if(corrector > 10) corrector = 10;
    /*if(rpm < 900) pressure = pressure + 4;
    if(pressure > 18) pressure = 18;
    if(pressure < 0) */pressure = 0;

    angle = corrector + pressure;
    
    delay(20);
}

void getRPM()
{ 
  time = micros();
  if(time - debounceTime > 18) { // исправления глюка когда возникают лишние прерывания
    impulse = !D3_Read; //читаем с датчика
    
    if(!initialized){ // если мимо датчика прошла шторка без прорези, нужно проверить  следующую шторку на ее наличие
        if(impulse){ 
          if(oldPeriod < 500000 && oldPeriod / 3.8 > time - checkTime){ //если период большой прорези минимум в 4 раза больше маленького выступа - значит это шторка с прорезью
            phaze = true;
            initialized = true;
          } else oldTime = time;
        }
        else {
          oldPeriod = time - oldTime;
          checkTime = time;
        }
      }
    else if(impulse){
             if (!advanceOn) digitalWrite(transistor, phaze);    //если в телефоне отключен корректор зажигания - отправляем сигнал сразу
          
             period = 0.5*(period + time - oldTime); // средние арифметическое данного и предыдущего периода между прерываниями для сглаживания
             rpm = 30000000 / period;
             oldTime = time;
             timeDelay = angle * period / 180 - 118; // отнимаем 118мкс для компенсации времени вычислений ардуины
             if(timeDelay < 0) timeDelay = 0;
             
             if(advanceOn) { 
                if(timeDelay > 0) delay_us(timeDelay); //ждем время на которое будет запаздывать зажигание
                if(phaze) D13_High;
                else D13_Low;
             }

              if(!phaze) initialized = false;
              phaze = false;        
   }
  }
   debounceTime = micros();
}




void OnUnknownCommand()
{
  Serial.println("error");
}
void OnSwitch()
{ Serial.println("adv");
  advanceOn = cmdMessenger.readFloatArg()?HIGH:LOW;
}
void OnCorrecting()
{
  corrector = cmdMessenger.readFloatArg();
}
void OnK()
{
  Serial.println(angleK);
  angleK = cmdMessenger.readFloatArg();
  Serial.println(angleK);
}

float mapFloat(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}

float Thermistor(int Raw) 
{
    long Resistance; 
    float Resistor = 2000; 
    float Temp; 
    Resistance= (Resistor*Raw /(1024-Raw)); 
    Temp = log(Resistance); 
    Temp = 1 / (0.00102119 + (0.000222468 * Temp) + (0.000000133342 * Temp * Temp * Temp));                    
    return  Temp- 273.15 - 25;            
}
