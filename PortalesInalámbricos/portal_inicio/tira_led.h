// =====================================================================
//  tira_led.h  —  Tira LED de los portales (WS2812B / NeoPixel)
//  ¡Este archivo debe ser IDÉNTICO en portal_inicio/ y portal_meta/!
//
//  Ambos portales muestran exactamente lo mismo:
//      AMARILLO ......... esperando: aún no se presiona "Listo" en la página
//      VERDE ............ listo para largar (se presionó "Listo" en la página)
//      BARRA AZUL ....... carrera en curso; se vacía según el tiempo restante
//      ANIMACIÓN ........ vuelta válida (dorada si es récord / nuevo líder)
//      ROJO ............. vuelta fallida (nula o tiempo excedido)
//      MORADO ........... equipo en pista sin intentos / ningún equipo
//      NARANJO parpadeo . sin conexión con el otro portal
//      ROJO parpadeo .... falla del sensor láser de este portal
//
//  Cada portal solo tiene que:
//    1) llamar a iniciarTira() en setup()
//    2) mantener actualizado "tira" (el estado de la carrera) usando tiraMux,
//       y los indicadores tiraSensorOk, tiraDatosOk y tiraEnlaceOk
//  Librería: Adafruit NeoPixel
// =====================================================================
#pragma once
#include <Adafruit_NeoPixel.h>
#include "common.h"

// ------------------------- Configuración -----------------------------
#define PIN_TIRA           4      // pin de datos (DIN) de la tira
#define NUM_LEDS           14     // cantidad de LEDs
#define BRILLO_TIRA        80     // 0-255 (30 LEDs a 255 en blanco ≈ 1,8 A)
#define TIPO_TIRA          (NEO_GRB + NEO_KHZ800)   // si rojo se ve verde: NEO_RGB
#define BARRA_INVERTIDA    true  // true = la barra se vacía hacia el otro extremo

#define DURACION_ROJO_MS   4000   // cuánto se muestra el rojo de vuelta fallida
#define DURACION_FIESTA_MS 4000   // cuánto dura la animación de vuelta válida
#define AVISO_FINAL_MS     30000  // la barra parpadea en los últimos 30 s (0 = sin aviso)

// ------------------------- Estado a mostrar --------------------------
struct EstadoTira {
  bool     recibido;     // ya hay al menos un estado válido
  uint32_t rxMs;         // millis() de la última actualización
  uint8_t  estado;       // EstadoCarrera
  uint8_t  listo;        // 1 = hay equipo en pista con intentos
  uint8_t  resultado;    // Resultado del último intento
  uint8_t  record;       // 0 no, 1 mejor personal, 2 nuevo líder
  uint8_t  armado;       // 1 = se presionó "Listo" en la página
  uint32_t tMs;          // tiempo transcurrido al momento de la actualización
  uint32_t limiteMs;     // tiempo límite vigente
  uint32_t resSeq;       // cambia con cada resultado nuevo
};
static EstadoTira    tira_estado = {};
static portMUX_TYPE  tiraMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool tiraSensorOk = true;   // sensor láser de ESTE portal
static volatile bool tiraDatosOk  = false;  // el estado de la carrera es confiable
static volatile bool tiraEnlaceOk = false;  // el otro portal está conectado

static Adafruit_NeoPixel tiraLeds(NUM_LEDS, PIN_TIRA, TIPO_TIRA);

// Qué está mostrando la tira (para el diagnóstico por Monitor Serie)
enum ModoTira : uint8_t { TIRA_APAGADA, TIRA_ERROR_SENSOR, TIRA_SIN_DATOS, TIRA_CARRERA,
                          TIRA_FALLIDA, TIRA_VALIDA, TIRA_SIN_ENLACE, TIRA_LISTO, TIRA_SIN_INTENTOS,
                          TIRA_ESPERANDO };
static volatile uint8_t tiraModo = TIRA_APAGADA;
static volatile uint32_t tiraCuadros = 0;     // cuadros dibujados (si no avanza, la tarea se colgó)

inline const char *nombreModoTira(uint8_t m) {
  switch (m) {
    case TIRA_ERROR_SENSOR: return "ROJO parpadeando (falla del sensor de este portal)";
    case TIRA_SIN_DATOS:    return "NARANJO parpadeando (sin estado del portal META)";
    case TIRA_CARRERA:      return "BARRA AZUL (carrera en curso)";
    case TIRA_FALLIDA:      return "ROJO (vuelta fallida)";
    case TIRA_VALIDA:       return "ANIMACION (vuelta valida)";
    case TIRA_SIN_ENLACE:   return "NARANJO parpadeando (el otro portal no esta conectado)";
    case TIRA_LISTO:        return "VERDE (listo para largar)";
    case TIRA_ESPERANDO:    return "AMARILLO (esperando que presionen LISTO en la pagina)";
    case TIRA_SIN_INTENTOS: return "MORADO (equipo sin intentos / sin equipo)";
    default:                return "APAGADA (la tarea de la tira no ha arrancado)";
  }
}

// Actualiza el estado a mostrar (llamar desde cualquier tarea o callback)
inline void tiraActualizar(const EstadoTira &e) {
  portENTER_CRITICAL(&tiraMux);
  tira_estado = e;
  tira_estado.recibido = true;
  tira_estado.rxMs = millis();
  if (!tira_estado.limiteMs) tira_estado.limiteMs = TIEMPO_LIMITE_MS;
  portEXIT_CRITICAL(&tiraMux);
}

// =====================================================================
//  Animaciones
// =====================================================================
// Escala un color 0-255 por un factor 0.0-1.0
static uint32_t escalar(uint8_t r, uint8_t g, uint8_t b, float f) {
  if (f < 0) f = 0;
  if (f > 1) f = 1;
  return tiraLeds.Color(r * f, g * f, b * f);
}

// Índice físico del LED según la dirección elegida para la barra
static int idxBarra(int i) { return BARRA_INVERTIDA ? (NUM_LEDS - 1 - i) : i; }

// Verde con una "respiración" suave
static void animListo(uint32_t t) {
  float f = 0.75f + 0.25f * sinf(t * 0.0025f);
  tiraLeds.fill(escalar(0, 255, 0, f));
}

// Barra azul: llena al largar, se vacía hasta el tiempo límite
static void animCarrera(uint32_t t, uint32_t transcurrido, uint32_t limite) {
  uint32_t restante = transcurrido >= limite ? 0 : limite - transcurrido;
  float lleno = (float)restante / limite * NUM_LEDS;   // LEDs encendidos (con decimal)
  bool apagarParpadeo = AVISO_FINAL_MS > 0 && restante < AVISO_FINAL_MS && (t / 250) % 2;

  tiraLeds.clear();
  for (int i = 0; i < NUM_LEDS; i++) {
    float f = lleno - i;                     // 1 = LED completo, 0..1 = el LED del borde
    if (f <= 0) break;
    tiraLeds.setPixelColor(idxBarra(i), escalar(0, 60, 255, apagarParpadeo ? f * 0.15f : f));
  }
}

// Vuelta fallida: rojo fijo (con un pulso al inicio para llamar la atención)
static void animFallida(uint32_t dt) {
  if (dt < 900) tiraLeds.fill((dt / 150) % 2 ? 0 : tiraLeds.Color(255, 0, 0));
  else          tiraLeds.fill(tiraLeds.Color(255, 0, 0));
}

// Rueda de colores 0-255 -> arcoíris
static uint32_t rueda(uint8_t p) {
  p = 255 - p;
  if (p < 85)  return tiraLeds.Color(255 - p * 3, 0, p * 3);
  if (p < 170) { p -= 85; return tiraLeds.Color(0, p * 3, 255 - p * 3); }
  p -= 170;    return tiraLeds.Color(p * 3, 255 - p * 3, 0);
}

// Vuelta válida: arcoíris en movimiento con destellos.
// Si es mejor personal o nuevo líder, termina en dorado con brillos blancos.
static void animValida(uint32_t dt, uint8_t record) {
  bool dorado = record && dt > DURACION_FIESTA_MS / 2;
  for (int i = 0; i < NUM_LEDS; i++) {
    uint32_t c;
    if (dorado) {
      float f = 0.55f + 0.45f * sinf(dt * 0.012f + i * 0.6f);
      c = escalar(255, record == 2 ? 170 : 120, 0, f);       // líder: más amarillo
    } else {
      c = rueda((i * 256 / NUM_LEDS + dt / 4) & 0xFF);
    }
    tiraLeds.setPixelColor(i, c);
  }
  // destellos blancos aleatorios
  for (int k = 0; k < NUM_LEDS / 8 + 1; k++)
    if (random(100) < 35) tiraLeds.setPixelColor(random(NUM_LEDS), tiraLeds.Color(255, 255, 255));
}

// Parpadeo de un color (sin conexión / error de sensor)
static void animParpadeo(uint32_t t, uint8_t r, uint8_t g, uint8_t b) {
  tiraLeds.fill((t / 400) % 2 ? tiraLeds.Color(r, g, b) : 0);
}

// =====================================================================
//  Tarea que dibuja la tira a ~50 cuadros por segundo (núcleo 0).
//  También maneja el LED RGB integrado de la placa con los mismos colores.
// =====================================================================
static void tareaLeds(void *) {
  tiraLeds.begin();
  tiraLeds.setBrightness(BRILLO_TIRA);
  tiraLeds.clear();
  tiraLeds.show();

  bool     sincronizado = false;   // para no repetir un resultado viejo al encender
  uint32_t resSeqVisto = 0, tResultado = 0;
  bool     hayResultado = false;

  for (;;) {
    uint32_t ahora = millis();
    portENTER_CRITICAL(&tiraMux);
    EstadoTira e = tira_estado;
    portEXIT_CRITICAL(&tiraMux);

    bool datos = tiraDatosOk && e.recibido && (ahora - e.rxMs) < 1500;

    // ¿llegó un resultado nuevo?
    if (e.recibido) {
      if (!sincronizado) { sincronizado = true; resSeqVisto = e.resSeq; }
      else if (e.resSeq != resSeqVisto) {
        resSeqVisto = e.resSeq; tResultado = ahora; hayResultado = true;
      }
    }
    uint32_t dtRes = ahora - tResultado;
    bool fallida = e.resultado != RES_VALIDO;
    uint32_t durRes = fallida ? DURACION_ROJO_MS : DURACION_FIESTA_MS;
    if (hayResultado && dtRes >= durRes) hayResultado = false;

    // --- elegir qué mostrar (en orden de prioridad) ---
    uint8_t r = 0, g = 0, b = 0;                     // LED integrado de la placa
    if (!tiraSensorOk) {
      animParpadeo(ahora, 255, 0, 0);                r = 40;          tiraModo = TIRA_ERROR_SENSOR;
    } else if (!datos) {
      animParpadeo(ahora, 255, 90, 0);               r = 40; g = 15;  tiraModo = TIRA_SIN_DATOS;
    } else if (e.estado == CARRERA_CORRIENDO) {
      uint32_t transcurrido = e.tMs + (ahora - e.rxMs);   // se extrapola entre actualizaciones
      animCarrera(ahora, transcurrido, e.limiteMs);  b = 50;          tiraModo = TIRA_CARRERA;
    } else if (e.estado == CARRERA_TERMINADO && hayResultado) {
      if (fallida) { animFallida(dtRes);             r = 40;          tiraModo = TIRA_FALLIDA; }
      else         { animValida(dtRes, e.record);    g = 30;          tiraModo = TIRA_VALIDA; }
    } else if (!tiraEnlaceOk) {
      animParpadeo(ahora, 255, 90, 0);               r = 40; g = 15;  tiraModo = TIRA_SIN_ENLACE;
    } else if (!e.listo) {
      tiraLeds.fill(tiraLeds.Color(120, 0, 160));    r = 25; b = 30;  tiraModo = TIRA_SIN_INTENTOS;
    } else if (e.armado) {
      animListo(ahora);                              g = 30;          tiraModo = TIRA_LISTO;
    } else {
      tiraLeds.fill(tiraLeds.Color(255, 160, 0));    r = 40; g = 28;  tiraModo = TIRA_ESPERANDO;
    }
    tiraLeds.show();
    tiraCuadros = tiraCuadros + 1;
    led(r, g, b);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// Llamar una vez en setup()
inline void iniciarTira() {
  xTaskCreatePinnedToCore(tareaLeds, "leds", 4096, nullptr, 1, nullptr, 0);
}
