#include "pines.h"
//frecuencia y resolucion motores
const int frecuencia = 15000;
const int resolucion = 8;

void setup() {
  // put your setup code here, to run once:
  inicializarMotores();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  Motor(255,255);
  /*delay(1000);
  Motor(-255,-255);
  delay(1000);
  Motor(0,255);
  delay(1500);*/
}
