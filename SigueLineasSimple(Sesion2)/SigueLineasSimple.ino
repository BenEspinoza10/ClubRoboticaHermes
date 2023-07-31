//Este ejemplo de siguelineas está pensado para pista negra con línea blanca

#include <QTRSensors.h>

#define NUM_SENSORS             6  // NUMERO DE SENSORES USADOS
#define NUM_SAMPLES_PER_SENSOR  4  // MUESTRAS POR SENSOR (DEJAR EN 4)
#define EMITTER_PIN             11  // PIN QUE CONTROLA EL EMISOR

//definicion pines motor
#define AIN1 7

#define AIN2 8
#define PWMA 6

#define BIN1 9
#define BIN2 4
#define PWMB 5

QTRSensorsAnalog qtra((unsigned char[]) {5, 4, 3, 2, 1, 0}, 
  NUM_SENSORS, NUM_SAMPLES_PER_SENSOR, EMITTER_PIN);

unsigned int sensorValues[NUM_SENSORS];

int umbral = 500; //umbral de diferencia entre blanco y negro

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(BIN2  , OUTPUT);
  pinMode(BIN1  , OUTPUT);
  pinMode(PWMB  , OUTPUT);
  pinMode(AIN1  , OUTPUT);
  pinMode(AIN2  , OUTPUT);
  pinMode(PWMA  , OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  qtra.read(sensorValues); //LECTURA DE LOS VALORES DE LOS SENSORES

  //CÓDIGO SÓLO CON 3 CASOS PARA IDEA GENERAL, para funcionamiento óptimo se deben hacer el resto de casos
  // negro, negro, blanco, blanco, negro negro
  if(sensorValues[0] > umbral && sensorValues[1] > umbral && sensorValues[2] < umbral && sensorValues[3] < umbral && sensorValues[4] > umbral && sensorValues[5] > umbral){
    Motor(150,150);
  // negro, blanco, blanco, negro, negro negro  
  }else if(sensorValues[0] > umbral && sensorValues[1] < umbral && sensorValues[2] < umbral && sensorValues[3] > umbral && sensorValues[4] > umbral && sensorValues[5] > umbral){
    Motor(50,150);
  }
  // negro, negro, negro, blanco, blanco negro  
  else if(sensorValues[0] > umbral && sensorValues[1] > umbral && sensorValues[2] > umbral && sensorValues[3] < umbral && sensorValues[4] < umbral && sensorValues[5] > umbral){
    Motor(150,50);
  }
}


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
