// =====================================================================
//  common.h  —  Configuración y protocolo compartido entre ambos portales
//  ¡Este archivo debe ser IDÉNTICO en portal_inicio/, portal_meta/ y portal_meta_esp32/!
//  (los pines cambian solos según la placa elegida en el IDE)
// =====================================================================
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <esp_arduino_version.h>
#include <esp_now.h>

// ----------------------------- RED -----------------------------------
// Canal WiFi que usan AMBOS portales. El portal META crea un punto de
// acceso en este canal y ESP-NOW debe ir en el mismo canal.
#define WIFI_CANAL        1

// Identificador de "red" para ignorar otros equipos ESP-NOW cercanos.
#define RED_ID            0xC40A7E11UL

// Potencia de transmisión WiFi. Las placas pequeñas (SuperMini, S3-Zero) tienen
// una antena de chip débil y a potencia máxima suelen transmitir MAL o nada.
// 8.5 dBm alcanza de sobra para portales a pocos metros. Si necesitas más
// alcance prueba WIFI_POWER_11dBm o WIFI_POWER_13dBm.
// La ESP32 Dev Module tiene antena de PCB más grande y aguanta más potencia.
#if CONFIG_IDF_TARGET_ESP32
  #define WIFI_POTENCIA_TX  WIFI_POWER_11dBm
#else
  #define WIFI_POTENCIA_TX  WIFI_POWER_8_5dBm
#endif

// Contadores de radio para el diagnóstico (los usan ambos portales)
static volatile uint32_t radioRecibidos   = 0;  // mensajes válidos recibidos
static volatile uint32_t radioTamanoMalo  = 0;  // llegó algo con otro tamaño (¿firmware distinto?)
static volatile uint32_t radioOtraRed     = 0;  // llegó algo con otro RED_ID
static volatile uint32_t radioEnviados    = 0;  // envíos con ACK OK
static volatile uint32_t radioFallidos    = 0;  // envíos sin ACK
static volatile int      radioUltimoError = 0;  // último error de esp_now_send()

// ----------------------------- SENSOR --------------------------------
// Sensor VL53L0X usando la librería "Adafruit_VL53L0X" (Adafruit)
// Dirección I2C del sensor (0x29 es la de fábrica)
#define SENSOR_I2C_ADDR   0x29

// Pines I2C. Se eligen solos según la placa con la que compilas:
#if CONFIG_IDF_TARGET_ESP32
  // ESP32 Dev Module (ESP32 clásica): pines I2C estándar.
  // OJO: los GPIO 6 a 11 son de la memoria flash, no se pueden usar.
  #define PIN_SDA         21
  #define PIN_SCL         22
  // La Dev Module no trae LED RGB (solo un LED azul en GPIO2)
  #define PIN_LED_RGB     -1
#else
  // ESP32-S3 (SuperMini / Zero): cualquier GPIO libre sirve
  #define PIN_SDA         8
  #define PIN_SCL         9
  // LED RGB integrado (WS2812): GPIO48 en ESP32-S3 SuperMini, GPIO21 en ESP32-S3-Zero
  #define PIN_LED_RGB     48
#endif

// Lectura del sensor igual que en cronometro/esp32_timer_vl53l0x (que funcionaba bien):
// modo por defecto de la librería, una medición a la vez (rangingTest) y se
// descartan las lecturas fuera de rango (RangeStatus == 4).
// Pausa entre lecturas en ms (el original usaba 50; menor = más precisión temporal)
#define SENSOR_PAUSA_MS   5

// El robot se detecta cuando la distancia se aleja de la línea base
// (más cerca o más lejos) en al menos esta cantidad de mm:
#define UMBRAL_CAMBIO_MM  15

// Muestras para calibrar la línea base (con 50 ms entre cada una)
#define MUESTRAS_CALIBRACION 20

// Lecturas consecutivas "libres" necesarias para re-armar el portal
#define LECTURAS_REARME   5
// Tiempo mínimo entre dos detecciones del mismo portal (ms)
#define BLOQUEO_MS        1500

// ----------------------------- CARRERA -------------------------------
// Tiempo máximo de una vuelta. Si se supera, el intento es NULO
// (cuenta como intento usado, sin tiempo). Ejemplos:
//    5 minutos  ->  (5UL * 60UL * 1000UL)
//    90 segundos ->  (90UL * 1000UL)
#define TIEMPO_LIMITE_MS  (5UL * 60UL * 1000UL)

// ----------------------------- PROTOCOLO -----------------------------
enum TipoMsg : uint8_t {
  MSG_HOLA       = 1,  // inicio -> broadcast : "¿dónde está la meta?"
  MSG_HOLA_OK    = 2,  // meta   -> inicio    : "aquí estoy"
  MSG_LARGADA    = 3,  // inicio -> meta      : robot cruzó el inicio
  MSG_LATIDO     = 4,  // inicio -> meta      : estado periódico
  MSG_RECALIBRAR = 5,  // meta   -> inicio    : volver a medir el piso
  MSG_ESTADO     = 6,  // meta   -> inicio    : estado de la carrera (luces)
};

// Estados de la carrera (los decide el portal META)
enum EstadoCarrera : uint8_t { CARRERA_LISTO = 0, CARRERA_CORRIENDO = 1, CARRERA_TERMINADO = 2 };
// Resultado del último intento
enum Resultado : uint8_t { RES_VALIDO = 0, RES_NULO = 1, RES_TIEMPO_EXCEDIDO = 2 };

typedef struct __attribute__((packed)) {
  uint32_t red;        // RED_ID
  uint8_t  tipo;       // TipoMsg
  uint32_t seq;        // LARGADA: nº de evento | ESTADO: nº de resultado
  uint32_t edadUs;     // µs transcurridos desde la detección hasta el envío
  uint16_t distMm;     // última distancia medida
  uint16_t baseMm;     // línea base (distancia al piso)
  uint8_t  sensorOk;   // 1 = sensor funcionando
  // --- solo MSG_ESTADO (meta -> inicio) ---
  uint8_t  estado;     // EstadoCarrera
  uint8_t  listo;      // 1 = hay equipo en pista con intentos disponibles
  uint8_t  resultado;  // Resultado del último intento
  uint8_t  record;     // 0 = no, 1 = mejor personal, 2 = nuevo líder
  uint8_t  armado;     // 1 = se presionó "Listo" en la página: se acepta la largada
  uint32_t tMs;        // tiempo transcurrido de la carrera en curso
  uint32_t limiteMs;   // tiempo límite vigente
} Mensaje;

// =====================================================================
//  Sensor de portal: calibra la distancia al piso y detecta el paso
//  (misma forma de leer que cronometro/esp32_timer_vl53l0x)
// =====================================================================
#include <Adafruit_VL53L0X.h>

class SensorPortal {
public:
  volatile uint16_t distMm = 0;
  volatile uint16_t baseMm = 0;
  volatile bool     ok     = false;

  bool begin() {
    Wire.begin(PIN_SDA, PIN_SCL);
    if (!lox.begin(SENSOR_I2C_ADDR)) { ok = false; return false; }
    ok = true;
    return calibrar();
  }

  // Promedia varias lecturas con el portal despejado
  bool calibrar() {
    uint32_t suma = 0; int n = 0;
    for (int i = 0; i < MUESTRAS_CALIBRACION; i++) {
      uint16_t d = leer();
      if (d) { suma += d; n++; }
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (n == 0) { ok = false; return false; }
    baseMm = suma / n;
    libres = LECTURAS_REARME;
    ok = true;
    return true;
  }

  // Hace una medición. Devuelve mm, o 0 si está fuera de rango.
  uint16_t leer() {
    VL53L0X_RangingMeasurementData_t measure;
    if (lox.rangingTest(&measure, false) != VL53L0X_ERROR_NONE) { ok = false; return 0; }
    if (measure.RangeStatus == 4) return 0;   // 4 = fuera de rango
    distMm = measure.RangeMilliMeter;
    return distMm;
  }

  // Lee una vez; devuelve true en el instante en que el robot entra al haz.
  // tDeteccionUs recibe la marca de tiempo (esp_timer) de esa lectura.
  bool poll(int64_t &tDeteccionUs) {
    leer();                                    // fuera de rango: se mantiene la última
    int64_t ahora = esp_timer_get_time();
    vTaskDelay(pdMS_TO_TICKS(SENSOR_PAUSA_MS));
    if (!ok || baseMm == 0) return false;

    int diferencia = abs((int)distMm - (int)baseMm);
    bool tapado = (diferencia >= UMBRAL_CAMBIO_MM);
    if (!tapado) {
      if (libres < LECTURAS_REARME) libres++;
      return false;
    }
    bool armado = (libres >= LECTURAS_REARME) &&
                  (ahora - ultimaDeteccion > (int64_t)BLOQUEO_MS * 1000);
    libres = 0;
    if (!armado) return false;
    ultimaDeteccion = ahora;
    tDeteccionUs = ahora;
    return true;
  }

private:
  Adafruit_VL53L0X lox;
  uint8_t  libres = 0;
  int64_t  ultimaDeteccion = -10000000;
};

// LED RGB integrado de la placa: DESACTIVADO por defecto.
// La tira LED ya muestra todo el estado. Además, el LED de la placa y la tira
// usan el mismo periférico (RMT) y en el core ESP32 2.x pueden interferirse
// ("rmt_write_items ... RMT DRIVER ERR"). Pon 1 para volver a usarlo.
#define USAR_LED_PLACA    0

#if USAR_LED_PLACA
  #include <Adafruit_NeoPixel.h>
  static Adafruit_NeoPixel ledPlaca(1, PIN_LED_RGB, NEO_GRB + NEO_KHZ800);
#endif

inline void led(uint8_t r, uint8_t g, uint8_t b) {
#if USAR_LED_PLACA
  static bool     iniciado = false;
  static uint32_t colorAnt = 0xFFFFFFFF;
  if (!iniciado) { ledPlaca.begin(); iniciado = true; }
  uint32_t c = ledPlaca.Color(r, g, b);
  if (c == colorAnt) return;                // solo se reenvía si cambió
  colorAnt = c;
  ledPlaca.setPixelColor(0, c);
  ledPlaca.show();
#else
  (void)r; (void)g; (void)b;
#endif
}

// Firma del callback de recepción ESP-NOW (cambió en el core 3.x)
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #define RECV_CB_ARGS  const esp_now_recv_info_t *info, const uint8_t *data, int len
  #define RECV_MAC      (info->src_addr)
#else
  #define RECV_CB_ARGS  const uint8_t *mac, const uint8_t *data, int len
  #define RECV_MAC      (mac)
#endif
