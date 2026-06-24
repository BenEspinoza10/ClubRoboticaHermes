#include"pines.h"

void inicializarMotores(){
  //Seteo pines motor  
  pinMode(BIN2  , OUTPUT);
  pinMode(BIN1  , OUTPUT);
  pinMode(PWMB  , OUTPUT);
  pinMode(AIN1  , OUTPUT);
  pinMode(AIN2  , OUTPUT);
  pinMode(PWMA  , OUTPUT);
  //seteo pin boton
  pinMode(BOTON, INPUT);
}

//accionamiento motor izquierdo
void Motorde(int value) {
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
void Motoriz(int value) {
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
void moverMotores(int left, int righ) {
  Motoriz(left);
  Motorde(righ);
}