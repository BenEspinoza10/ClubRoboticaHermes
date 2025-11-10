/*
 * Timer con ESP32 S3 Zero + OLED 128x32 I2C + VL53L0X
 * 
 * Funcionamiento:
 * - Mide distancia constantemente
 * - Al detectar cambio: inicia timer
 * - Al volver a distancia base y detectar nuevo cambio: detiene timer
 * - Muestra información en pantalla OLED en tiempo real
 * 
 * Librerías necesarias:
 * - Adafruit_SSD1306
 * - Adafruit_GFX
 * - Adafruit_VL53L0X (o VL53L0X by Pololu)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_VL53L0X.h>

// Configuración OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Sensor VL53L0X
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// Variables del timer
enum Estado {
  CALIBRANDO,
  ESPERANDO_INICIO,
  CORRIENDO,
  ESPERANDO_FIN,
  DETENIDO
};

Estado estadoActual = CALIBRANDO;
unsigned long tiempoInicio = 0;
unsigned long tiempoFin = 0;
unsigned long tiempoTranscurrido = 0;
unsigned long tiempoMostrado = 0;

// Variables del sensor
uint16_t distanciaBase = 0;
uint16_t distanciaActual = 0;
const uint16_t UMBRAL_CAMBIO = 15;  // mm de diferencia para detectar cambio
const int NUM_MUESTRAS_CALIBRACION = 20;
const unsigned long TIEMPO_ESTABILIZACION = 1000; // ms para considerar distancia estable

unsigned long ultimoCambio = 0;
bool distanciaEstable = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Timer con VL53L0X...");

  // Inicializar I2C
  Wire.begin(2,1);

  // Inicializar OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Error: No se encuentra OLED"));
    while(1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Iniciando...");
  display.display();

  // Inicializar VL53L0X
  if (!lox.begin()) {
    Serial.println(F("Error: No se encuentra VL53L0X"));
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Error VL53L0X!");
    display.display();
    while(1);
  }
  
  Serial.println("VL53L0X inicializado");

  // Calibración inicial
  calibrarDistanciaBase();
}

void calibrarDistanciaBase() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibrando...");
  display.println("No mover objeto");
  display.display();

  uint32_t suma = 0;
  int muestrasValidas = 0;

  for (int i = 0; i < NUM_MUESTRAS_CALIBRACION; i++) {
    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false);

    if (measure.RangeStatus != 4) {  // 4 = fuera de rango
      suma += measure.RangeMilliMeter;
      muestrasValidas++;
    }
    delay(50);
  }

  if (muestrasValidas > 0) {
    distanciaBase = suma / muestrasValidas;
    estadoActual = ESPERANDO_INICIO;
    Serial.print("Distancia base calibrada: ");
    Serial.print(distanciaBase);
    Serial.println(" mm");
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Calibrado!");
    display.print("Base: ");
    display.print(distanciaBase);
    display.println(" mm");
    display.display();
    delay(1500);
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Error calibracion");
    display.display();
    delay(2000);
    calibrarDistanciaBase(); // Reintentar
  }
}

void loop() {
  // Leer distancia del sensor
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {  // Lectura válida
    distanciaActual = measure.RangeMilliMeter;
  } else {
    // Si no hay lectura válida, mantener la última
    Serial.println("Fuera de rango");
  }

  // Calcular diferencia de distancia
  int diferencia = abs((int)distanciaActual - (int)distanciaBase);
  
  // Detectar si la distancia está estable
  if (diferencia < UMBRAL_CAMBIO) {
    if (millis() - ultimoCambio > TIEMPO_ESTABILIZACION) {
      distanciaEstable = true;
    }
  } else {
    distanciaEstable = false;
    ultimoCambio = millis();
  }

  // Máquina de estados
  switch (estadoActual) {
    
    case ESPERANDO_INICIO:
      if (diferencia >= UMBRAL_CAMBIO) {
        // Detectado cambio - iniciar timer
        tiempoInicio = millis();
        estadoActual = CORRIENDO;
        Serial.println("Timer iniciado!");
      }
      break;

    case CORRIENDO:
      // Actualizar tiempo transcurrido
      tiempoTranscurrido = millis() - tiempoInicio;
      
      // Verificar si volvió a distancia base y está estable
      if (distanciaEstable) {
        estadoActual = ESPERANDO_FIN;
        Serial.println("Distancia estable, esperando siguiente cambio...");
      }
      break;

    case ESPERANDO_FIN:
      tiempoTranscurrido = millis() - tiempoInicio;
      
      if (diferencia >= UMBRAL_CAMBIO) {
        // Detectado segundo cambio - detener timer
        tiempoFin = millis();
        tiempoMostrado = tiempoFin - tiempoInicio;
        estadoActual = DETENIDO;
        Serial.print("Timer detenido! Tiempo: ");
        Serial.print(tiempoMostrado);
        Serial.println(" ms");
      }
      break;

    case DETENIDO:
      // Esperar reset manual o automático
      // Puedes agregar un botón o un timeout para volver a ESPERANDO_INICIO
      break;
  }

  // Actualizar pantalla
  actualizarPantalla();

  delay(50);  // Pequeño delay entre lecturas
}

void actualizarPantalla() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  // Línea 1: Estado actual
  display.print("Estado: ");
  switch (estadoActual) {
    case CALIBRANDO:
      display.println("Calibrando");
      break;
    case ESPERANDO_INICIO:
      display.println("Listo");
      break;
    case CORRIENDO:
      display.println("Corriendo");
      break;
    case ESPERANDO_FIN:
      display.println("En vuelta");
      break;
    case DETENIDO:
      display.println("Detenido");
      break;
  }

  // Línea 2: Distancia actual
  display.print("Dist: ");
  display.print(distanciaActual);
  display.print(" mm");
  if (distanciaEstable) {
    display.print(" *");
  }
  display.println();

  // Línea 3: Timer
  display.setTextSize(1);
  unsigned long tiempoAMostrar;
  
  if (estadoActual == CORRIENDO || estadoActual == ESPERANDO_FIN) {
    tiempoAMostrar = tiempoTranscurrido;
  } else if (estadoActual == DETENIDO) {
    tiempoAMostrar = tiempoMostrado;
  } else {
    tiempoAMostrar = 0;
  }

  // Formatear tiempo (mm:ss.ms)
  unsigned long milisegundos = tiempoAMostrar % 1000;
  unsigned long segundos = (tiempoAMostrar / 1000) % 60;
  unsigned long minutos = (tiempoAMostrar / 60000);

  display.print("Tiempo: ");
  if (minutos < 10) display.print("0");
  display.print(minutos);
  display.print(":");
  if (segundos < 10) display.print("0");
  display.print(segundos);
  display.print(".");
  if (milisegundos < 100) display.print("0");
  if (milisegundos < 10) display.print("0");
  display.print(milisegundos);

  display.display();
}

// Función opcional para resetear el timer (puedes agregar un botón)
void resetTimer() {
  estadoActual = ESPERANDO_INICIO;
  tiempoInicio = 0;
  tiempoFin = 0;
  tiempoTranscurrido = 0;
  tiempoMostrado = 0;
  Serial.println("Timer reseteado");
}
