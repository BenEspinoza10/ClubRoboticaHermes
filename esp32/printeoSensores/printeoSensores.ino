#include <QTRSensors.h>
#include "pines.h"

//Se crea el objeto de los sensores
QTRSensors sigueLineas;

//Se crea la variable que representa el número de sensores
const uint8_t numeroSensores = 6;

//Se crea la lista para almacenar las lecturas de los sensores
uint16_t valoresSensor[numeroSensores];

void setup() {
  //Se inicia la comunicacion Serial
  Serial.begin(115200);

  //Se inician los sensores
  inicializarSensores();
  pinMode(LED, OUTPUT);  //led interna

  //calibracion
  //Se espera a que se apriete el botón para inicializar los sensores
  while (true) {
    if (digitalRead(BOTON) == 1) {
      calibrarSensores();
      Serial.println("listo");
      break;
    }
  }

  //Se imprimen los minimos y maximos detectados por la calibracion
  imprimirMinimosMaximos();
  //Se espera a que se apriete el boton para iniciar el programa
  while (digitalRead(BOTON) == 0) {
  }
}

void loop() {
  //Se imprimen los valores leidos por cada sensor
  imprimirValoresSensor();
  delay(200);
}
