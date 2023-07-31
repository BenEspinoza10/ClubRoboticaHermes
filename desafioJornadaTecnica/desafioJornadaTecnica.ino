#include <QTRSensors.h>
//Definición sensor Sigue Lineas
#define NUM_SENSORS             6
#define NUM_SAMPLES_PER_SENSOR  4
#define EMITTER_PIN 11
QTRSensorsAnalog qtra((unsigned char[]) {
  A5, A4, A3, A2, A1, A0
},
NUM_SENSORS, NUM_SAMPLES_PER_SENSOR, EMITTER_PIN);
unsigned int sensorValues[NUM_SENSORS];

//Mapeo de pines
//Motor A
#define AIN1 7
#define AIN2 8
#define PWMA 6
//Motor B
#define BIN1 9
#define BIN2 4
#define PWMB 5
//boton
#define BOTON 12

#define BUZZER 10
#define LED 13

void setup() {
  //Seteo pines motor  
  pinMode(BIN2  , OUTPUT);
  pinMode(BIN1  , OUTPUT);
  pinMode(PWMB  , OUTPUT);
  pinMode(AIN1  , OUTPUT);
  pinMode(AIN2  , OUTPUT);
  pinMode(PWMA  , OUTPUT);
  //seteo pin boton
  pinMode(BOTON, INPUT);
  //seteo buzzer
  pinMode(BUZZER, OUTPUT);


  while (digitalRead(BOTON) == 0); // Se sale del bucle cuando se aprieta el botón
  tone(BUZZER, 2000, 500);
  //calibracion sensor
  for ( int i = 0; i < 150; i++) {    
    qtra.calibrate();    
  }
  tone(BUZZER, 2000, 500);

  //Se pausa el programa hasta que se presione el boton
  while (true) {
    if (digitalRead(BOTON) == 1) {
      break;
    }
  }
  Serial.begin(9600);
}

void loop() {
  //aqui hacer el PID
  //La funcion para mover el robot es Motor(izq,der)
}


//Accionamiento de motores
void Motor(int left, int righ) {
  Motoriz(left);
  Motorde(righ);
}

//accionamiento motor izquierdo
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
//accionamiento motor derecho
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



