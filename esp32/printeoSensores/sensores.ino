#include "pines.h"

//Funcion para inicializar el sensor Infrarrojo, el objeto debe estar creado de forma global
//Para crear el objeto se debe usar: QTRSensors sigueLineas;
void inicializarSensores() {
  sigueLineas.setTypeAnalog();
  sigueLineas.setSensorPins((const uint8_t[]) {
    A5, A4, A3, A2, A1, A0
  },
  numeroSensores);
  sigueLineas.setEmitterPin(38);
}


//Funcion que calibra los sensores
void calibrarSensores() {
  //El límite del for define la duración de la calibración
  for (int i = 0; i < 150; i++) {
    digitalWrite(LED, HIGH);
    delay(10);
    sigueLineas.calibrate();
    digitalWrite(LED, LOW);
    delay(10);
  }
}

//Función que imprimer los valores reales leido por cada uno de los sensores
void imprimirValoresSensor() {
  sigueLineas.read(valoresSensor);
  for (int i=0;i<numeroSensores;i++){
    Serial.print(i);
    Serial.print(": ");
    Serial.print(valoresSensor[i]);
    Serial.print(" ");
  }
  Serial.println();
}

//Función que imprime los minimos y maximos leidos en la calibración por cada uno de los sensores
void imprimirMinimosMaximos() {
  Serial.println("Minimos: ");
  for (int i=0;i<numeroSensores;i++){
    Serial.print(i);
    Serial.print(": ");
    Serial.print(sigueLineas.calibrationOn.minimum[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println("Maximos: ");
  for (int i=0;i<numeroSensores;i++){
    Serial.print(i);
    Serial.print(": ");
    Serial.print(sigueLineas.calibrationOn.maximum[i]);
    Serial.print(" ");
  }
  Serial.println();
}

