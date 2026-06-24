#include"pines.h"

//Variables
int error = 0;      //Proporcional
int integral = 0;   //Integral
int lastError = 0;  //Ultimo error
int derivada = 0;   //Derivada

//referencia y velocidad base
int referencia = 0;

QTRSensors qtr;
const uint8_t SensorCount = 6;
uint16_t sensorValues[SensorCount];
void inicializarSensorSigueLineas() {
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){A5, A4, A3, A2, A1, A0}, SensorCount);
  qtr.setEmitterPin(11);
}

void calibrarSensor() {
  for (int i = 0; i < 200; i++) {
    qtr.calibrate();
    digitalWrite(LED, HIGH);
    delay(10);
    digitalWrite(LED, LOW);
    delay(10);
  }
}

int leerPosicion(){
  return qtr.readLineWhite(sensorValues);
}

void esperarBoton(){
  //Se pausa el programa hasta que se presione el boton
  while (true) {
    if (digitalRead(BOTON) == 1) {
      break;
    }
  }
}

void pid(int velocidadBase, int KP, int KI, int KD){
  int posicion = qtr.readLineWhite(sensorValues);
  posicion = map(posicion, 0, 5000, -255, 255);
  
  //Cálculo PID
  error = posicion - referencia;
  integral = integral + error;
  derivada = error - lastError;

  //Cálculo giro
  int giro = (error * KP) + (integral * KI) + (derivada * KD);

  //Seteo velocidad ruedas
  int velizq = velocidadBase + giro;
  int velder = velocidadBase - giro;

  //Movimiento
  moverMotores(velizq,velder);

  //Se actualiza el último error
  lastError = error; 
}