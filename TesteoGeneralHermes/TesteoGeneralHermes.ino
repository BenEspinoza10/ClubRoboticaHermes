#include <QTRSensors.h>

int modo = 0;

// pines motores
#define AIN1 7
#define AIN2 8
#define PWMA 6
#define BIN1 9
#define BIN2 4
#define PWMB 5

// pines componentes soldados
#define LED 13
#define BUZZER 10
#define BOTON 12

// pines sensores laterales
#define MIZ A7
#define MDE A6

// definicion siguelineas
#define NUM_SENSORS 6
#define EMITTER_PIN 11

QTRSensors qtr;
const uint8_t SensorCount = NUM_SENSORS;
uint16_t sensorValues[SensorCount];

// Función accionamiento motor izquierdo
void Motorde(int value) {
  if (value >= 0) {
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
  } else {
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    value *= -1;
  }
  analogWrite(PWMB, value);
}

// Función accionamiento motor derecho
void Motoriz(int value) {
  if (value >= 0) {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
  } else {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    value *= -1;
  }
  analogWrite(PWMA, value);
}

// Accionamiento de motores
void Motor(int left, int right) {
  Motoriz(left);
  Motorde(right);
}

void setup() {
  Serial.begin(9600);

  pinMode(LED, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(MIZ, INPUT);
  pinMode(MDE, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BOTON, INPUT);

  // Configuración QTR versión 4
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){A5, A4, A3, A2, A1, A0}, SensorCount);
  qtr.setEmitterPin(EMITTER_PIN);

  Serial.println("Este es un codigo de pruebas de la funcionalidad de Hermes");
  Serial.println("Presiona el boton para cambiar los modos");
}

void loop() {
  if (digitalRead(BOTON) == 1) {
    if (modo == 4) {
      modo = 1;
    } else {
      modo++;
    }
    while (digitalRead(BOTON) == 1);
  }

  if (modo == 1) {
    Motor(70, 70);
    delay(2000);
    Motor(-70, -70);
    delay(2000);
    Motor(70,0);
    delay(2000);
    Motor(0, 0);
    delay(500);
    modo = 2;

  } else if (modo == 2) {
    qtr.read(sensorValues);  // lectura de sensores QTR
    for (uint8_t i = 0; i < SensorCount; i++) {
      Serial.print(sensorValues[i]);
      Serial.print('\t');
    }
    Serial.println();
    delay(500);

  } else if (modo == 3) {
    int lectura_MIZ = analogRead(MIZ);
    int lectura_MDE = analogRead(MDE);
    Serial.print("sensor izquierdo: ");
    Serial.print(lectura_MIZ);
    Serial.print(" sensor derecho: ");
    Serial.println(lectura_MDE);
    delay(500);

  } else if (modo == 4) {
    Serial.println("Modo 4: Test de buzzer");
    tone(BUZZER, 1319 / 2, 100);
    delay(117);
    tone(BUZZER, 1319 / 2, 100);
    delay(333);
    tone(BUZZER, 1319 / 2, 100);
    delay(334);
    tone(BUZZER, 1047 / 2, 100);
    delay(117);
    tone(BUZZER, 1319 / 2, 100);
    delay(334);
    tone(BUZZER, 1564 / 2, 100);
    delay(600);
    tone(BUZZER, 784 / 2, 100);
  }
}