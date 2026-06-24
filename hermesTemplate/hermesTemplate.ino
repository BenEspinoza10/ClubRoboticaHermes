#include <QTRSensors.h>
#include "pines.h"

void setup() {
  inicializarMotores();
  inicializarSensorSigueLineas();
  calibrarSensor();
  esperarBoton();
  Serial.begin(9600);
}

void loop() {
  int pos = leerPosicion();
  pid(100,0.39,0.1,1);
  Serial.println(pos);
}


