#include <QTRSensors.h>

#define BUZZER 10
#define LED 13
#define BOTON 12

#define NUM_SENSORS             6
#define NUM_SAMPLES_PER_SENSOR  4
# define EMITTER_PIN 11
QTRSensorsAnalog qtra((unsigned char[]) {
  A5, A4, A3, A2, A1, A0
},
NUM_SENSORS, NUM_SAMPLES_PER_SENSOR, EMITTER_PIN);
unsigned int sensorValues[NUM_SENSORS];

//geo
#define HIZ A7
#define HDE A6

//contador de hitos
int geo = 0;
int geo1 = 0, geo2 = 0, geo3 = 0, geo4 = 0, geo5 = 0;
int umbral = 750;
int fin = 0;

void setup() {
  pinMode(LED   , OUTPUT);
  pinMode(BOTON, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(HIZ, INPUT);
  pinMode(HDE, INPUT);
  while (digitalRead(BOTON) == 0); // Se sale del bucle cuando se aprieta el botón
  tone(BUZZER, 2000, 500);
  for ( int i = 0; i < 400; i++) {
    digitalWrite(LED, HIGH);
    qtra.calibrate();
    digitalWrite(LED, LOW);
  }

  tone(BUZZER, 2000, 1500);

  while (true) {
    if (digitalRead(BOTON) == 1) {
      break;
    }
  }
  Serial.begin(9600);
}

void loop() {
  hitos();
}


void hitos() {
  int Hiz = analogRead(HIZ);
  int Hde = analogRead(HDE);

  if (Hiz < umbral) {
    Hiz = 1;
  } else {
    Hiz = 0;
  }

  if (Hde < umbral) {
    Hde = 1;
  } else {
    Hde = 0;
  }
  
  if (Hiz == 0 && Hde == 0) {
    geo = 0;
  }
  if (Hiz == 1 && Hde == 0) {
    geo = 1;
  }
  if (Hiz == 0 && Hde == 1) {
    geo = 2;
  }
  if (Hiz == 1 && Hde == 1) {
    geo = 3;
  }

  if (geo1 != geo) {
    if (geo == 0 && geo1 == 1 && geo2 == 0) {
      tone(BUZZER, 1000, 100);
       //funcionHitoIz();
    }
    if (geo == 0 && geo1 == 2 && geo2 == 0) {
      
      fin++;
      funcionHitoDe();
    }
    if (geo == 0 && ((geo1 == 3) || (geo2 == 3) || (geo3 == 3))) {

      funcionCruce();
    }
    geo5 = geo4;
    geo4 = geo3;
    geo3 = geo2;
    geo2 = geo1;
    geo1 = geo;
  }
}

void funcionCruce() {
  Serial.println("Intersection");
  tone(BUZZER, 2000, 500);
}

void funcionHitoDe() {

  tone(BUZZER, 2000, 5000);
  if (fin >= 2) {
    //aqui va el codigo para frenar
  }
}

void funcionHitoIz() {

  tone(BUZZER, 2000, 1000);
  //aqui va el codigo para los cambios de curvatura
}
