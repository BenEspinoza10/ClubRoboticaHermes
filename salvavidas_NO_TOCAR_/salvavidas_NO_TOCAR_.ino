#include <QTRSensors.h>

//Mapeo de pines
#define AIN1 7
#define AIN2 8
#define PWMA 6

#define BIN1 9  
#define BIN2 4
#define PWMB 5
#define BUZZER 10
#define LED 13
#define BOTON 12
//Esto ir modificandolo para ver el rendimiento
int proportional = 0; // proporcional
int derivative = 0; // derivada
int last_proportional;
int integrative = 0; //integral
int error2 = 0;
int error3 = 0;
int error4 = 0;
int error5 = 0;
int error6 = 0;
#define NUM_SENSORS             6
#define NUM_SAMPLES_PER_SENSOR  4
# define EMITTER_PIN 11
QTRSensorsAnalog qtra((unsigned char[]) {
  A5, A4, A3, A2, A1, A0
},
NUM_SENSORS, NUM_SAMPLES_PER_SENSOR, EMITTER_PIN);
unsigned int sensorValues[NUM_SENSORS];

//geo
#define HIZ A7
#define HDE A6

//contador de hitos
int geo = 0;
int geo1 = 0, geo2 = 0, geo3 = 0, geo4 = 0, geo5 = 0;
int CHiz;
int CHde;
int umbral = 750;
int fin = 0;
//control de ruedas
int ref = 0;

void Motoriz(int value) {
  if ( value >= 0 ) {
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
  } else {
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    value *= -1;
  }
  analogWrite(PWMB, value);
}
// FunciÃ³n accionamiento motor derecho
void Motorde(int value) {
  if ( value >= 0 ) {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
  } else {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    value *= -1;
  }
  analogWrite(PWMA, value);
}

//Accionamiento de motores
void Motor(int left, int righ) {
  Motoriz(left);
  Motorde(righ);
}
void setup() {
  pinMode(LED   , OUTPUT);
  pinMode(BIN2  , OUTPUT);
  pinMode(BIN1  , OUTPUT);
  pinMode(PWMB  , OUTPUT);
  pinMode(AIN1  , OUTPUT);
  pinMode(AIN2  , OUTPUT);
  pinMode(PWMA  , OUTPUT);
  pinMode(BOTON, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(HIZ, INPUT);
  pinMode(HDE, INPUT);
  while (digitalRead(BOTON) == 0); // Se sale del bucle cuando se aprieta el botón
  tone(BUZZER, 2000, 500);
  for ( int i = 0; i < 400; i++) {
    digitalWrite(LED, HIGH);
    qtra.calibrate();
    digitalWrite(LED, LOW);
  }

  tone(BUZZER, 2000, 1500);

  while (true) {
    if (digitalRead(BOTON) == 1) {
      break;
    }
  }
  Serial.begin(9600);
}

void loop() {
  siguelineas(0.4,0.15,75,100,150);
  hitos();
}

void siguelineas(float KP,float KI, float KD,int vel_base,int lim){
  int posicion = qtra.readLine(sensorValues, true, true);
  posicion = map(posicion, 0, 5000, -255, 255);
  Serial.println(posicion);

  //PID
  proportional = posicion - ref;
  derivative = proportional - last_proportional;
  integrative = last_proportional + error2 + error3 + error4 + error5 + error6;
  last_proportional = proportional;
  //Lo encuadrado eliminarlo si no se usa el integral
  //-------------------------------------//
  error6 = error5;                       //
  error5 = error4;                       //
  error4 = error3;                       //
  error3 = error2;                       //
  error2 = last_proportional;            //
  //-------------------------------------//
  int power_difference = (proportional * KP) + (derivative * KD) + (integrative * KI);//El (integrative*KI) es el elemento integral
  if (power_difference > lim)
    power_difference = lim;
  else if (power_difference < -lim)
    power_difference = -lim;
      
    Motor(vel_base - power_difference, vel_base +  power_difference);  
}
void hitos() {
  int Hiz = analogRead(HIZ);
  int Hde = analogRead(HDE);

  if (Hiz < umbral) {
    Hiz = 1;
  } else {
    Hiz = 0;
  }

  if (Hde < umbral) {
    Hde = 1;
  } else {
    Hde = 0;
  }
  
  if (Hiz == 0 && Hde == 0) {
    geo = 0;
  }
  if (Hiz == 1 && Hde == 0) {
    geo = 1;
  }
  if (Hiz == 0 && Hde == 1) {
    geo = 2;
  }
  if (Hiz == 1 && Hde == 1) {
    geo = 3;
  }

  if (geo1 != geo) {
    if (geo == 0 && geo1 == 1 && geo2 == 0) {
      tone(BUZZER, 1000, 100);
       //funcionHitoIz();
    }
    if (geo == 0 && geo1 == 2 && geo2 == 0) {
      
      fin++;
      funcionHitoDe();
    }
    if (geo == 0 && ((geo1 == 3) || (geo2 == 3) || (geo3 == 3))) {

      funcionCruce();
    }
    geo5 = geo4;
    geo4 = geo3;
    geo3 = geo2;
    geo2 = geo1;
    geo1 = geo;
  }
}

void funcionCruce() {
  Serial.println("Intersection");
  tone(BUZZER, 2000, 500);
}

void funcionHitoDe() {

  tone(BUZZER, 2000, 5000);
  if (fin >= 2) {
    Motor(0, 0);
    tone(BUZZER, 1319 / 2, 100);
    delay(117);
    tone(BUZZER, 1319 / 2 , 100);
    delay(333);
    tone(BUZZER, 1319 / 2, 100);
    delay(334);
    tone(BUZZER, 1047 / 2, 100);
    delay(117);
    tone(BUZZER, 1319 / 2 , 100);
    delay(334);
    tone(BUZZER, 1564 / 2 , 100);
    delay(600);
    tone(BUZZER, 784 / 2, 100);
    while (true);
  }
}
