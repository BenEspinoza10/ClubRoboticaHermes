// =====================================================================
//  PORTAL DE INICIO  —  ESP32-S3 (SuperMini / Zero) + VL53L0X (librería Adafruit_VL53L0X)
//  - Detecta el paso del robot y avisa al portal META por ESP-NOW.
//  - Muestra en su tira LED el estado que le envía el portal META
//    (colores y configuración de la tira en tira_led.h)
//  No necesitas configurar la MAC del otro portal: se encuentran solos.
//  Librerías: Adafruit_VL53L0X, Adafruit NeoPixel
// =====================================================================
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "common.h"
#include "tira_led.h"

static const uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

SensorPortal sensor;

// --- Estado de enlace ---
static uint8_t  macMeta[6];
static volatile bool emparejado      = false;
static volatile bool pedirRecalibrar = false;
static volatile int  fallosSeguidos  = 0;

// --- Evento de largada pendiente de entregar ---
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool     largadaPendiente = false;
static volatile int64_t  tLargadaUs = 0;
static volatile uint32_t seqLargada = 0;

// Resultado del último envío (lo escribe el callback de ESP-NOW)
static volatile int estadoEnvio = 0;   // 0 = esperando, 1 = OK, -1 = falló

// ---------------------------------------------------------------------
//  Callbacks ESP-NOW (la firma del callback de envío cambió en core 3.3)
// ---------------------------------------------------------------------
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 3, 0)
void alEnviar(const wifi_tx_info_t *info, esp_now_send_status_t st) {
#else
void alEnviar(const uint8_t *mac, esp_now_send_status_t st) {
#endif
  estadoEnvio = (st == ESP_NOW_SEND_SUCCESS) ? 1 : -1;
  if (st == ESP_NOW_SEND_SUCCESS) radioEnviados = radioEnviados + 1;
  else                            radioFallidos = radioFallidos + 1;
}

void alRecibir(RECV_CB_ARGS) {
  if (len != sizeof(Mensaje)) { radioTamanoMalo = radioTamanoMalo + 1; return; }
  Mensaje m; memcpy(&m, data, sizeof(m));
  if (m.red != RED_ID) { radioOtraRed = radioOtraRed + 1; return; }
  radioRecibidos = radioRecibidos + 1;

  if (m.tipo == MSG_HOLA_OK && !emparejado) {
    memcpy(macMeta, RECV_MAC, 6);
    esp_now_peer_info_t p = {};
    memcpy(p.peer_addr, macMeta, 6);
    p.channel = WIFI_CANAL;
    p.ifidx   = WIFI_IF_STA;
    p.encrypt = false;
    if (!esp_now_is_peer_exist(macMeta)) esp_now_add_peer(&p);
    fallosSeguidos = 0;
    emparejado = true;
  } else if (m.tipo == MSG_RECALIBRAR) {
    pedirRecalibrar = true;
  } else if (m.tipo == MSG_ESTADO) {       // estado de la carrera -> tira LED
    EstadoTira e = {};
    e.estado    = m.estado;
    e.listo     = m.listo;
    e.resultado = m.resultado;
    e.record    = m.record;
    e.armado    = m.armado;
    e.tMs       = m.tMs;
    e.limiteMs  = m.limiteMs;
    e.resSeq    = m.seq;
    tiraActualizar(e);
  }
}

// Envía y espera la confirmación (ACK de capa MAC). true = entregado.
bool enviar(const uint8_t *mac, Mensaje &m, bool esperarAck) {
  estadoEnvio = 0;
  esp_err_t err = esp_now_send(mac, (uint8_t *)&m, sizeof(m));
  if (err != ESP_OK) { radioUltimoError = err; return false; }
  if (!esperarAck) return true;
  uint32_t t0 = millis();
  while (estadoEnvio == 0 && millis() - t0 < 50) delay(1);
  return estadoEnvio == 1;
}

Mensaje nuevoMensaje(TipoMsg tipo) {
  Mensaje m = {};
  m.red      = RED_ID;
  m.tipo     = tipo;
  m.distMm   = sensor.distMm;
  m.baseMm   = sensor.baseMm;
  m.sensorOk = sensor.ok ? 1 : 0;
  return m;
}

// ---------------------------------------------------------------------
//  Tarea del sensor: lee continuamente y marca el instante de detección
// ---------------------------------------------------------------------
void tareaSensor(void *) {
  for (;;) {
    if (pedirRecalibrar) {
      pedirRecalibrar = false;
      sensor.calibrar();
    }
    int64_t t;
    if (sensor.poll(t)) {
      portENTER_CRITICAL(&mux);
      tLargadaUs = t;
      seqLargada = seqLargada + 1;
      largadaPendiente = true;
      portEXIT_CRITICAL(&mux);
    }
    if (!sensor.ok) { vTaskDelay(pdMS_TO_TICKS(500)); sensor.begin(); }
  }
}

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) delay(10);   // da tiempo a abrir el Monitor Serie
  Serial.println("\n=== PORTAL INICIO arrancando ===");
  led(40, 0, 0);  // rojo: arrancando

  // WiFi en modo estación, fijado al canal compartido
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CANAL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_ps(WIFI_PS_NONE);  // sin ahorro de energía = menos latencia
  WiFi.setTxPower(WIFI_POTENCIA_TX);  // antena débil: potencia baja = transmisión estable

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error iniciando ESP-NOW");
    ESP.restart();
  }
  esp_now_register_send_cb(alEnviar);
  esp_now_register_recv_cb(alRecibir);

  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, BROADCAST, 6);
  p.channel = WIFI_CANAL;
  p.ifidx   = WIFI_IF_STA;
  esp_now_add_peer(&p);

  Serial.printf("Portal INICIO  MAC: %s\n", WiFi.macAddress().c_str());
  Serial.println("Calibrando: deja el portal despejado...");
  if (sensor.begin()) Serial.printf("Linea base: %u mm\n", sensor.baseMm);
  else                Serial.println("¡Sensor no encontrado! Revisa SDA/SCL.");

  xTaskCreatePinnedToCore(tareaSensor, "sensor", 4096, nullptr, 5, nullptr, 1);
  iniciarTira();
  Serial.printf("Tira LED en GPIO%d (%d LEDs). Listo.\n", PIN_TIRA, NUM_LEDS);
}

// Diagnóstico por Monitor Serie cada 2 segundos
void diagnostico() {
  static uint32_t tDiag = 0, cuadrosAnt = 0;
  if (millis() - tDiag < 2000) return;
  tDiag = millis();
  portENTER_CRITICAL(&tiraMux);
  EstadoTira e = tira_estado;
  portEXIT_CRITICAL(&tiraMux);
  uint32_t cuadros = tiraCuadros;
  Serial.println("---------------- DIAGNOSTICO INICIO ----------------");
  Serial.printf(" Enlace META   : %s\n", emparejado ? "EMPAREJADO" : "buscando... (¿META encendida? ¿mismo firmware?)");
  if (!e.recibido) Serial.println(" Estado META   : nunca ha llegado (la META no está enviando su estado)");
  else Serial.printf(" Estado META   : hace %lu ms -> %s, %s\n", millis() - e.rxMs,
                     e.estado == CARRERA_CORRIENDO ? "CORRIENDO" : e.estado == CARRERA_TERMINADO ? "TERMINADO" : "LISTO",
                     e.listo ? (e.armado ? "LISTO (armado)" : "ESPERANDO (falta presionar LISTO)") : "SIN equipo/intentos");
  Serial.printf(" Latidos fallidos seguidos: %d\n", fallosSeguidos);
  uint8_t canal = 0; wifi_second_chan_t sec; esp_wifi_get_channel(&canal, &sec);
  Serial.printf(" Radio         : MAC %s, canal %u | enviados OK %lu, sin ACK %lu, error envio %d\n",
                WiFi.macAddress().c_str(), canal, radioEnviados, radioFallidos, radioUltimoError);
  Serial.printf("                 recibidos OK %lu, tamano distinto %lu, otra red %lu\n",
                radioRecibidos, radioTamanoMalo, radioOtraRed);
  Serial.printf(" Sensor        : %s, %u mm (piso %u mm)\n", sensor.ok ? "OK" : "FALLA", sensor.distMm, sensor.baseMm);
  Serial.printf(" Tira          : %s%s\n", nombreModoTira(tiraModo),
                cuadros == cuadrosAnt ? "  <-- ¡la tarea de la tira NO avanza!" : "");
  cuadrosAnt = cuadros;
}

void loop() {
  static uint32_t tHola = 0, tLatido = 0;
  uint32_t ahora = millis();

  // Indicadores para la tira LED (en este portal el estado viene de la META)
  tiraSensorOk = sensor.ok;
  tiraDatosOk  = emparejado;
  tiraEnlaceOk = emparejado;
  diagnostico();

  // 1) Buscar el portal META mientras no esté emparejado
  if (!emparejado) {
    if (ahora - tHola > 500) {
      tHola = ahora;
      Mensaje m = nuevoMensaje(MSG_HOLA);
      enviar(BROADCAST, m, false);
      Serial.println("Buscando portal META...");
    }
    delay(5);
    return;
  }

  // 2) Entregar la largada (reintenta hasta lograrlo)
  if (largadaPendiente) {
    portENTER_CRITICAL(&mux);
    int64_t  tDet = tLargadaUs;
    uint32_t seq  = seqLargada;
    portEXIT_CRITICAL(&mux);

    Mensaje m = nuevoMensaje(MSG_LARGADA);
    m.seq = seq;
    // La meta resta esta "edad" a su hora de recepción, así los
    // reintentos y retrasos no afectan el tiempo medido.
    m.edadUs = (uint32_t)(esp_timer_get_time() - tDet);
    bool ok = enviar(macMeta, m, true);

    if (ok) {
      portENTER_CRITICAL(&mux);
      if (seq == seqLargada) largadaPendiente = false;
      portEXIT_CRITICAL(&mux);
      fallosSeguidos = 0;
      Serial.printf("LARGADA #%lu enviada (edad %lu us)\n", seq, m.edadUs);
    } else if (m.edadUs > 3000000) {
      largadaPendiente = false;      // >3 s sin entregar: se descarta
      Serial.println("No se pudo entregar la largada");
    }
  }

  // 3) Latido de estado cada 500 ms
  if (ahora - tLatido > 500) {
    tLatido = ahora;
    Mensaje m = nuevoMensaje(MSG_LATIDO);
    if (enviar(macMeta, m, true)) fallosSeguidos = 0;
    else {
      fallosSeguidos = fallosSeguidos + 1;
    }
    if (fallosSeguidos > 10) {   // la meta desapareció: re-buscar
      fallosSeguidos = 0;
      esp_now_del_peer(macMeta);
      emparejado = false;
    }
  }
  delay(2);
}
