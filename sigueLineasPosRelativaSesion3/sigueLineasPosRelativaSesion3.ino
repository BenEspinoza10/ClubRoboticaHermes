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

#define BOTON 12

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(BIN2  , OUTPUT);
  pinMode(BIN1  , OUTPUT);
  pinMode(PWMB  , OUTPUT);
  pinMode(AIN1  , OUTPUT);
  pinMode(AIN2  , OUTPUT);
  pinMode(PWMA  , OUTPUT);

  while(true){
    if (digitalRead(BOTON)==1){ 
      break;
    }
  
  for (int i = 0; i < 400; i++)  // aprox 10 seg
  {
    qtra.calibrate();       
  }
  Serial.println("fin calibracion, presiona el boton para seguir");
  while(true){
    if (digitalRead(BOTON)==1){ 
      break;
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  //Linea negra sobre fondo blanco
  //int posicion = qtra.readLine(sensorValues, true, false);
  //Linea blanca sobre fondo negro
  int posicion = qtra.readLine(sensorValues, true, true);


  //mapeo para que la lectura pase de estar entre 0 y 5000 a -255 y 255
  posicion = map(posicion, 0, 5000, -255, 255);

  //siguelíneas sólo con 3 rangos, para suavizarlo se pueden añadir más intervalos
  if (posicion < 100 and posicion >-100){
    Motor(150,150);
  }else if(posicion > 100){
    Motor(100,0);
  }else if(posicion < -100){
    Motor(0,100);
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
