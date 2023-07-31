#include <QTRSensors.h>

#define NUM_SENSORS             6  // NUMERO DE SENSORES USADOS
#define NUM_SAMPLES_PER_SENSOR  4  // MUESTRAS POR SENSOR (DEJAR EN 4)
#define EMITTER_PIN             11  // PIN QUE CONTROLA EL EMISOR

QTRSensorsAnalog qtra((unsigned char[]) {5, 4, 3, 2, 1, 0}, 
  NUM_SENSORS, NUM_SAMPLES_PER_SENSOR, EMITTER_PIN);
unsigned int sensorValues[NUM_SENSORS];

#define BOTON 12

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Presionar botón para empezar a calibrar");
  while(true){
    if (digitalRead(BOTON)==1){ 
      break;
    }
  }
  for (int i = 0; i < 400; i++)  // aprox 10 seg
  {
    qtra.calibrate();       
  }

  //minimo leido en la calibraicion
  /*
  Serial.println("Minimo leido:"')
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    Serial.print(qtra.calibratedMinimumOn[i]);
    Serial.print(' ');
  }
  Serial.println();*/
  
  // maximo leido en la calibracion
  /*
  Serial.println("Maximo leido:"')
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    Serial.print(qtra.calibratedMaximumOn[i]);
    Serial.print(' ');
  }
  Serial.println();*/
  Serial.println("fin calibracion, presiona el boton para seguir");
  while(true){
    if (digitalRead(BOTON)==1){ 
      break;
    }
  }
}

void loop() {
  //Linea negra sobre fondo blanco
  //int posicion = qtra.readLine(sensorValues, true, false);
  //Linea blanca sobre fondo negro
  int posicion = qtra.readLine(sensorValues, true, true);


  //mapeo para que la lectura pase de estar entre 0 y 5000 a -255 y 255
  posicion = map(posicion, 0, 5000, -255, 255);
  Serial.println(posicion);
  
  delay(250);
}
