#define AI1 5
#define AI2 6
#define BI1 10
#define BI2 9

void Motoriz(int value) {
  if ( value >= 0 ) {
    analogWrite(BI1, value);
    analogWrite(BI2, 0);
  } else {
    value *= -1;
    analogWrite(BI1, 0);
    analogWrite(BI2, value);
  }
}
// Función accionamiento motor derecho
void Motorde(int value) {
  if ( value >= 0 ) {
    analogWrite(AI1, value);
    analogWrite(AI2, 0);
  } else {
    value *= -1;
    analogWrite(AI1, 0);
    analogWrite(AI2, value);
  }
}

//Accionamiento de motores
void Motor(int left, int righ) {
  Motoriz(left);
  Motorde(righ);
}
void setup() {
  // put your setup code here, to run once:
  pinMode(BI2  , OUTPUT);
  pinMode(BI1  , OUTPUT);
  pinMode(AI1  , OUTPUT);
  pinMode(AI2  , OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  Motor(255,255);
  delay(1000);
  Motor(-255,-255);
  delay(1000);
  Motor(0,255);
  delay(1500);
}
