#include <QTRSensors.h>

#define NUM_SENSORS             6  // NUMERO DE SENSORES USADOS
#define NUM_SAMPLES_PER_SENSOR  4  // MUESTRAS POR SENSOR (DEJAR EN 4)
#define EMITTER_PIN             11  // PIN QUE CONTROLA EL EMISOR

QTRSensorsAnalog qtra((unsigned char[]) {5, 4, 3, 2, 1, 0}, 
  NUM_SENSORS, NUM_SAMPLES_PER_SENSOR, EMITTER_PIN);
unsigned int sensorValues[NUM_SENSORS];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  qtra.read(sensorValues); //LECTURA DE LOS VALORES DE LOS SENSORES

   /*printeo de los sensores individuales, 0 es maxima reflectancia (blanco) 
   y 1023 es minima reflectancia (negro) */
  for (unsigned char i = 0; i < NUM_SENSORS; i++){
    Serial.print(sensorValues[i]);
    Serial.print('\t'); 
  }
  Serial.println();
  
  delay(250);
}
