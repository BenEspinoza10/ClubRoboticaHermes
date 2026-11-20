// =====================================================================
//  PORTAL DE META  —  ESP32-S3 (SuperMini / Zero) + VL53L0X (librería Adafruit_VL53L0X)
//  - Recibe la largada del portal de INICIO por ESP-NOW
//  - Detecta la llegada con su propio sensor y calcula el tiempo
//  - Cada equipo tiene N intentos (5 por defecto); solo se guarda su mejor
//  - Un intento puede repetirse (falla técnica) sin que cuente
//  - Una vuelta que supera TIEMPO_LIMITE_MS (common.h) es NULA
//  - Ambos portales muestran el mismo estado en su tira LED (tira_led.h)
//  - Crea la red WiFi "Cronometro-Robot" y sirve la página web:
//      Control:   http://192.168.4.1         (o http://cronometro.local)
//      Pantalla:  http://192.168.4.1/pantalla (solo lectura, para proyectar)
// =====================================================================
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "common.h"
#include "tira_led.h"
#include "pagina.h"

// ------------------------- Configuración -----------------------------
const char *AP_SSID = "Cronometro-Robot";
const char *AP_PASS = "robot1234";                     // mínimo 8 caracteres
// El tiempo límite de una vuelta (TIEMPO_LIMITE_MS) está en common.h

// Equipos iniciales (solo se usan la primera vez o tras "Restablecer equipos")
const char *EQUIPOS_INICIALES[] = { "Equipo 1", "Equipo 2", "Equipo 3" };

#define INTENTOS_DEFECTO 5   // intentos por equipo (se puede cambiar desde la web)
#define MAX_INTENTOS     20  // tope del ajuste en la web
#define MAX_EQUIPOS      32  // equipos máximos
#define LARGO_NOMBRE     24  // bytes (incluye el \0)

WebServer    server(80);
SensorPortal sensor;
Preferences  prefs;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// =====================================================================
//  Datos de equipos (solo se tocan desde loop(), nunca desde callbacks)
// =====================================================================
struct Equipo {
  uint16_t id;
  char     nombre[LARGO_NOMBRE];
  uint8_t  usados;         // intentos que ya contaron
  uint8_t  deshacible;     // el último intento puede anularse
  uint32_t mejor;          // mejor tiempo (ms), 0 = sin tiempo válido
  int32_t  ultimo;         // último intento: -1 ninguno, 0 nulo, >0 ms
  // copia del estado anterior para poder anular el último intento
  uint32_t mejorPrevio;
  int32_t  ultimoPrevio;
};
struct Datos {
  uint32_t magia;
  uint16_t sigId;
  int16_t  activo;         // id del equipo en pista (-1 = ninguno)
  uint8_t  autoAvanzar;    // pasar al siguiente equipo al agotar intentos
  uint8_t  intentosMax;
  uint8_t  nEq;
  Equipo   eq[MAX_EQUIPOS];
};
#define MAGIA_DATOS 0xC40A0003UL   // cambia si cambia el formato guardado

static Datos    D;
static uint32_t rev = 1;            // cambia cada vez que cambian los datos
static bool     guardarPendiente = false;
static uint32_t tCambio = 0;

int  buscar(int id) { for (int i = 0; i < D.nEq; i++) if (D.eq[i].id == id) return i; return -1; }
bool tieneIntentos(int i) { return i >= 0 && D.eq[i].usados < D.intentosMax; }
void cambio() { rev++; guardarPendiente = true; tCambio = millis(); }

// Quita caracteres problemáticos y corta sin romper caracteres UTF-8
String limpiarNombre(String s) {
  String r;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if ((uint8_t)c < 0x20 || c == '"' || c == '\\' || c == '<' || c == '>') continue;
    r += c;
  }
  r.trim();
  if (r.length() > LARGO_NOMBRE - 1) {
    int len = LARGO_NOMBRE - 1;
    while (len > 0 && ((uint8_t)r[len] & 0xC0) == 0x80) len--;
    r = r.substring(0, len);
    r.trim();
  }
  return r;
}

void limpiarEquipo(Equipo &e) {
  e.usados = 0; e.deshacible = 0; e.mejor = 0; e.ultimo = -1;
  e.mejorPrevio = 0; e.ultimoPrevio = -1;
}

int agregarEquipo(const String &nombre) {
  if (D.nEq >= MAX_EQUIPOS) return -1;
  Equipo &e = D.eq[D.nEq++];
  memset(&e, 0, sizeof(e));
  e.id = D.sigId++;
  strlcpy(e.nombre, nombre.c_str(), LARGO_NOMBRE);
  limpiarEquipo(e);
  if (D.activo < 0) D.activo = e.id;
  cambio();
  return e.id;
}

void datosDeFabrica() {
  memset(&D, 0, sizeof(D));
  D.magia = MAGIA_DATOS; D.sigId = 1; D.activo = -1;
  D.autoAvanzar = 1; D.intentosMax = INTENTOS_DEFECTO;
  for (auto n : EQUIPOS_INICIALES) agregarEquipo(n);
}

void cargarDatos() {
  prefs.begin("crono", false);
  if (prefs.getBytesLength("datos") == sizeof(D) &&
      prefs.getBytes("datos", &D, sizeof(D)) == sizeof(D) &&
      D.magia == MAGIA_DATOS && D.nEq <= MAX_EQUIPOS) {
    Serial.printf("Datos cargados: %u equipos\n", D.nEq);
  } else {
    Serial.println("Sin datos guardados: se crean equipos iniciales");
    datosDeFabrica();
  }
}

void guardarSiHaceFalta() {    // agrupa cambios para no desgastar la flash
  if (guardarPendiente && millis() - tCambio > 1000) {
    guardarPendiente = false;
    prefs.putBytes("datos", &D, sizeof(D));
  }
}

// Cuenta un intento. ms = 0 significa intento nulo (no llegó / descalificado)
void contarIntento(int i, uint32_t ms) {
  Equipo &e = D.eq[i];
  e.mejorPrevio = e.mejor; e.ultimoPrevio = e.ultimo;
  e.usados++;
  e.ultimo = ms;
  if (ms && (e.mejor == 0 || ms < e.mejor)) e.mejor = ms;
  e.deshacible = 1;
  cambio();
}

// Anula el último intento del equipo: se devuelve el intento y el tiempo previo
bool anularUltimo(int i) {
  Equipo &e = D.eq[i];
  if (!e.deshacible || e.usados == 0) return false;
  e.usados--; e.mejor = e.mejorPrevio; e.ultimo = e.ultimoPrevio; e.deshacible = 0;
  cambio();
  return true;
}

int posicionDe(int id) {       // 1 = primero; 0 = sin tiempo
  int i = buscar(id); if (i < 0 || !D.eq[i].mejor) return 0;
  int pos = 1;
  for (int k = 0; k < D.nEq; k++)
    if (k != i && D.eq[k].mejor && D.eq[k].mejor < D.eq[i].mejor) pos++;
  return pos;
}

// Siguiente equipo (en orden de turno) que aún tenga intentos
int siguienteConIntentos(int desde) {
  for (int k = 1; k <= D.nEq; k++) {
    int j = (desde + k) % D.nEq;
    if (tieneIntentos(j)) return j;
  }
  return -1;
}

// =====================================================================
//  Estado de la carrera (compartido con el callback ESP-NOW y el sensor)
// =====================================================================
enum Estado : uint8_t { LISTO, CORRIENDO, TERMINADO };
static volatile Estado   estado = LISTO;
static volatile int64_t  tInicioUs = 0;
static volatile int16_t  idActivo = -1;          // copia de D.activo para el callback
static volatile bool     activoPuedeCorrer = false;
static volatile int16_t  idEnCarrera = -1;       // equipo dueño de la carrera
static volatile bool     llegadaNueva = false;
static volatile uint32_t llegadaMs = 0;
static volatile uint32_t avisoMs = 0;            // largada ignorada
static volatile uint8_t  avisoMotivo = 0;        // 1 = no estaba LISTO, 2 = sin equipo/intentos
// "Armado": el operador presionó LISTO en la página. Solo así se acepta una
// largada. Se consume al largar: cada intento hay que habilitarlo de nuevo.
static volatile bool     armado = false;

// Resultado del último intento terminado (para la web)
static int16_t  ultId = -1;
static uint32_t ultMs = 0;
static uint8_t  ultNum = 0;           // nº de intento (1..N)
static bool     ultNulo = false, ultRecord = false, ultLider = false;
static int      ultPos = 0;
static uint8_t  ultResultado = RES_VALIDO;   // válido / nulo / tiempo excedido
static uint32_t resSeq = 0;                  // cambia con cada resultado nuevo

// ------------------------- Portal de inicio --------------------------
static uint8_t  macInicio[6];
static volatile bool     inicioConocido = false;
static volatile uint32_t ultimoLatidoMs = 0;
static volatile uint16_t inicioDist = 0, inicioBase = 0;
static volatile uint8_t  inicioOk = 0;
static volatile uint32_t ultimaSeq = 0xFFFFFFFF;
static volatile bool     responderHola = false;
static volatile bool     registrarInicio = false;  // inicio ya emparejado tras reiniciar la meta
static volatile bool     enviarRecal = false;
static volatile bool     pedirRecalibrar = false;

// =====================================================================
//  ESP-NOW
// =====================================================================
void alRecibir(RECV_CB_ARGS) {
  int64_t tRx = esp_timer_get_time();     // ¡lo primero: marcar la hora!
  if (len != sizeof(Mensaje)) { radioTamanoMalo = radioTamanoMalo + 1; return; }
  Mensaje m; memcpy(&m, data, sizeof(m));
  if (m.red != RED_ID) { radioOtraRed = radioOtraRed + 1; return; }
  radioRecibidos = radioRecibidos + 1;

  portENTER_CRITICAL(&mux);
  memcpy(macInicio, RECV_MAC, 6);
  inicioDist = m.distMm; inicioBase = m.baseMm; inicioOk = m.sensorOk;
  ultimoLatidoMs = millis();

  if (m.tipo == MSG_HOLA) {
    responderHola = true;                 // se responde desde loop()
  } else if (!inicioConocido) {
    registrarInicio = true;               // la meta se reinició: volver a registrar al inicio
  }

  if (m.tipo == MSG_LARGADA && m.seq != ultimaSeq) {
    ultimaSeq = m.seq;                    // descarta reintentos duplicados
    if (estado != CORRIENDO) {            // si ya corre, se ignora
      if (armado && activoPuedeCorrer) {
        tInicioUs = tRx - (int64_t)m.edadUs;
        idEnCarrera = idActivo;
        estado = CORRIENDO;
        armado = false;                   // el próximo intento hay que habilitarlo de nuevo
      } else {
        avisoMs = millis();               // largada ignorada
        avisoMotivo = activoPuedeCorrer ? 1 : 2;
      }
    }
  }
  portEXIT_CRITICAL(&mux);
}

void agregarPeerInicio() {
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, macInicio, 6);
  p.channel = WIFI_CANAL;
  p.ifidx   = WIFI_IF_AP;
  if (esp_now_is_peer_exist(macInicio)) esp_now_del_peer(macInicio);
  esp_now_add_peer(&p);
  inicioConocido = true;
}

void enviarAInicio(TipoMsg tipo) {
  if (!inicioConocido) return;
  Mensaje m = {};
  m.red = RED_ID; m.tipo = tipo;
  esp_now_send(macInicio, (uint8_t *)&m, sizeof(m));
}

// =====================================================================
//  Tarea del sensor de META
// =====================================================================
void tareaSensor(void *) {
  for (;;) {
    if (pedirRecalibrar) { pedirRecalibrar = false; sensor.calibrar(); }

    int64_t t;
    if (sensor.poll(t)) {
      portENTER_CRITICAL(&mux);
      if (estado == CORRIENDO && t > tInicioUs) {
        llegadaMs = (uint32_t)((t - tInicioUs + 500) / 1000);
        llegadaNueva = true;
        estado = TERMINADO;
      }
      portEXIT_CRITICAL(&mux);
    }
    if (!sensor.ok) { vTaskDelay(pdMS_TO_TICKS(500)); sensor.begin(); }
  }
}

// Registra el resultado de un intento (ms = 0 -> nulo) y avanza el turno
void finalizarIntento(int id, uint32_t ms, uint8_t resultado) {
  if (ms == 0 && resultado == RES_VALIDO) resultado = RES_NULO;
  ultId = id; ultMs = ms; ultNulo = (ms == 0);
  ultResultado = resultado; resSeq++;
  ultNum = 0; ultRecord = ultLider = false; ultPos = 0;
  int i = buscar(id);
  Serial.printf("Intento %s: %lu ms%s\n", i >= 0 ? D.eq[i].nombre : "?", ms,
                resultado == RES_TIEMPO_EXCEDIDO ? " (TIEMPO EXCEDIDO)" : resultado == RES_NULO ? " (NULO)" : "");
  if (i < 0) { rev++; return; }

  uint32_t mejorAntes = D.eq[i].mejor;
  int posAntes = posicionDe(id);
  contarIntento(i, ms);
  ultNum = D.eq[i].usados;
  ultRecord = ms && (mejorAntes == 0 || ms < mejorAntes);
  ultPos = posicionDe(id);
  ultLider = ultRecord && ultPos == 1 && posAntes != 1;

  // Al agotar los intentos, pasa al siguiente equipo que tenga intentos
  if (D.autoAvanzar && !tieneIntentos(i)) {
    int j = siguienteConIntentos(i);
    if (j >= 0) { D.activo = D.eq[j].id; cambio(); }
  }
}

void procesarLlegada() {
  portENTER_CRITICAL(&mux);
  bool hay = llegadaNueva; llegadaNueva = false;
  uint32_t ms = llegadaMs; int16_t id = idEnCarrera;
  portEXIT_CRITICAL(&mux);
  if (!hay) return;
  // Llegó, pero después del tiempo límite: la vuelta no vale
  if (ms > TIEMPO_LIMITE_MS) finalizarIntento(id, 0, RES_TIEMPO_EXCEDIDO);
  else                       finalizarIntento(id, ms, RES_VALIDO);
}

// Revisa si la carrera en curso superó el tiempo límite -> intento nulo
void revisarTiempoLimite() {
  portENTER_CRITICAL(&mux);
  bool excedido = (estado == CORRIENDO) &&
                  (esp_timer_get_time() - tInicioUs) / 1000 > (int64_t)TIEMPO_LIMITE_MS;
  if (excedido) estado = TERMINADO;
  int16_t id = idEnCarrera;
  portEXIT_CRITICAL(&mux);
  if (excedido) finalizarIntento(id, 0, RES_TIEMPO_EXCEDIDO);
}

// ---------------------------------------------------------------------
//  Estado de la carrera para las tiras LED (el mismo para ambos portales)
// ---------------------------------------------------------------------
EstadoTira estadoParaTiras() {
  portENTER_CRITICAL(&mux);
  Estado e = estado; int64_t ini = tInicioUs;
  portEXIT_CRITICAL(&mux);
  EstadoTira t = {};
  t.estado    = (e == CORRIENDO) ? CARRERA_CORRIENDO : (e == TERMINADO) ? CARRERA_TERMINADO : CARRERA_LISTO;
  t.listo     = activoPuedeCorrer ? 1 : 0;
  t.resultado = ultResultado;
  t.record    = ultLider ? 2 : ultRecord ? 1 : 0;
  t.armado    = armado ? 1 : 0;
  t.tMs       = (e == CORRIENDO) ? (uint32_t)((esp_timer_get_time() - ini) / 1000) : ultMs;
  t.limiteMs  = TIEMPO_LIMITE_MS;
  t.resSeq    = resSeq;
  return t;
}

bool inicioOnline() {
  return ultimoLatidoMs != 0 && (millis() - ultimoLatidoMs) < 2000;
}

// Actualiza la tira propia y envía el mismo estado al portal de INICIO.
// Al INICIO se le envía al instante cuando algo cambia y cada 250 ms como latido.
void actualizarTiras() {
  EstadoTira t = estadoParaTiras();

  // Tira de este portal
  tiraSensorOk = sensor.ok;
  tiraDatosOk  = true;               // aquí vive el estado: siempre es confiable
  tiraEnlaceOk = inicioOnline();     // sin portal de inicio no se puede largar
  tiraActualizar(t);

  // Tira del portal de inicio
  static uint32_t tUltimo = 0;
  static uint8_t  estAnt = 0xFF, listoAnt = 0xFF, armAnt = 0xFF;
  static uint32_t seqAnt = 0xFFFFFFFF;
  if (!inicioConocido) return;
  bool cambio = (t.estado != estAnt) || (t.listo != listoAnt) || (t.resSeq != seqAnt) || (t.armado != armAnt);
  if (!cambio && millis() - tUltimo < 250) return;
  tUltimo = millis(); estAnt = t.estado; listoAnt = t.listo; seqAnt = t.resSeq; armAnt = t.armado;

  Mensaje m = {};
  m.red       = RED_ID;
  m.tipo      = MSG_ESTADO;
  m.seq       = t.resSeq;
  m.estado    = t.estado;
  m.listo     = t.listo;
  m.resultado = t.resultado;
  m.record    = t.record;
  m.armado    = t.armado;
  m.tMs       = t.tMs;
  m.limiteMs  = t.limiteMs;
  esp_now_send(macInicio, (uint8_t *)&m, sizeof(m));
}

// =====================================================================
//  Servidor web
// =====================================================================
void responderOk()                 { server.send(200, "text/plain", "ok"); }
void responderError(const char *m) { server.send(400, "text/plain", m); }
int  argId(const char *n = "id")   { return server.hasArg(n) ? server.arg(n).toInt() : -1; }

void jsonPortal(String &j, bool online, bool ok, uint16_t dist, uint16_t base) {
  j += "{\"online\":"; j += online ? "true" : "false";
  j += ",\"ok\":";     j += ok ? "true" : "false";
  j += ",\"dist\":";   j += dist;
  j += ",\"base\":";   j += base; j += "}";
}

// Estado liviano: se consulta cada 200 ms
void apiEstado() {
  portENTER_CRITICAL(&mux);
  Estado e = estado; int64_t ini = tInicioUs; int16_t enCarrera = idEnCarrera;
  bool online = ultimoLatidoMs != 0 && (millis() - ultimoLatidoMs) < 2000;
  bool aviso  = avisoMs != 0 && (millis() - avisoMs) < 5000;
  uint16_t iDist = inicioDist, iBase = inicioBase; uint8_t iOk = inicioOk;
  portEXIT_CRITICAL(&mux);

  uint32_t t = 0;
  if (e == CORRIENDO)      t = (uint32_t)((esp_timer_get_time() - ini) / 1000);
  else if (e == TERMINADO) t = ultMs;
  int iu = buscar(ultId);

  String j; j.reserve(450);
  j += "{\"estado\":\"";
  j += (e == LISTO ? "listo" : e == CORRIENDO ? "corriendo" : "terminado");
  j += "\",\"t\":"; j += t;
  j += ",\"rev\":"; j += rev;
  j += ",\"activo\":"; j += D.activo;
  j += ",\"enCarrera\":"; j += (e == LISTO ? -1 : enCarrera);
  j += ",\"aviso\":"; j += aviso ? "true" : "false";
  j += ",\"avisoMotivo\":"; j += avisoMotivo;
  j += ",\"armado\":"; j += armado ? "true" : "false";
  j += ",\"inicioOnline\":"; j += inicioOnline() ? "true" : "false";
  j += ",\"limite\":"; j += (uint32_t)TIEMPO_LIMITE_MS;
  j += ",\"ult\":{\"id\":"; j += (iu >= 0 ? ultId : -1);
  j += ",\"ms\":"; j += ultMs;
  j += ",\"num\":"; j += ultNum;
  j += ",\"nulo\":"; j += ultNulo ? "true" : "false";
  j += ",\"motivo\":"; j += ultResultado;
  j += ",\"record\":"; j += ultRecord ? "true" : "false";
  j += ",\"lider\":"; j += ultLider ? "true" : "false";
  j += ",\"anulable\":"; j += (iu >= 0 && D.eq[iu].deshacible) ? "true" : "false";
  j += ",\"pos\":"; j += ultPos; j += "}";
  j += ",\"inicio\":"; jsonPortal(j, online, online && iOk, iDist, iBase);
  j += ",\"meta\":";   jsonPortal(j, true, sensor.ok, sensor.distMm, sensor.baseMm);
  j += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

// Datos de equipos: solo se piden cuando cambia "rev"
void apiEquipos() {
  String j; j.reserve(200 + D.nEq * 110);
  j += "{\"rev\":"; j += rev;
  j += ",\"activo\":"; j += D.activo;
  j += ",\"auto\":"; j += D.autoAvanzar ? "true" : "false";
  j += ",\"intentos\":"; j += D.intentosMax;
  j += ",\"max\":"; j += MAX_EQUIPOS;
  j += ",\"equipos\":[";
  for (int i = 0; i < D.nEq; i++) {
    const Equipo &e = D.eq[i];
    if (i) j += ",";
    j += "{\"id\":"; j += e.id;
    j += ",\"nombre\":\""; j += e.nombre;
    j += "\",\"usados\":"; j += e.usados;
    j += ",\"mejor\":"; j += e.mejor;
    j += ",\"ultimo\":"; j += e.ultimo;
    j += ",\"anulable\":"; j += e.deshacible ? "true" : "false";
    j += "}";
  }
  j += "]}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

void apiActivo() {
  int id = argId();
  if (buscar(id) < 0) return responderError("Equipo no existe");
  D.activo = id; avisoMs = 0;
  armado = false;                  // al cambiar de equipo hay que volver a dar LISTO
  cambio(); responderOk();
}

// LISTO: habilita el próximo intento (portales en verde, se acepta la largada)
void apiArmar() {
  portENTER_CRITICAL(&mux);
  Estado e = estado;
  portEXIT_CRITICAL(&mux);
  if (e == CORRIENDO)              return responderError("Hay una carrera en curso");
  if (!tieneIntentos(buscar(D.activo)))
    return responderError("El equipo en pista no tiene intentos disponibles");
  if (!inicioOnline())             return responderError("El portal de inicio no está conectado");
  portENTER_CRITICAL(&mux);
  estado = LISTO;                  // el resultado anterior ya quedó guardado
  armado = true;
  portEXIT_CRITICAL(&mux);
  avisoMs = 0; rev++; responderOk();
}

// Vuelve a "esperando" sin largar
void apiDesarmar() {
  armado = false; rev++; responderOk();
}

// ---- Durante / después de un intento ----
// Repetir: si está corriendo, se cancela sin contar. Si ya terminó, se anula
// ese intento (se devuelve) y el mismo equipo vuelve a la pista.
void apiRepetir() {
  portENTER_CRITICAL(&mux);
  Estado e = estado;
  if (e == CORRIENDO) estado = LISTO;
  int16_t enCarrera = idEnCarrera;
  armado = false;                  // tras repetir, hay que volver a dar LISTO
  portEXIT_CRITICAL(&mux);

  if (e == CORRIENDO) {
    if (buscar(enCarrera) >= 0) D.activo = enCarrera;
    rev++; return responderOk();
  }
  int i = buscar(ultId);
  if (i < 0 || !anularUltimo(i)) return responderError("No hay un intento para repetir");
  D.activo = ultId;
  ultId = -1; ultMs = 0;
  portENTER_CRITICAL(&mux);
  estado = LISTO;
  portEXIT_CRITICAL(&mux);
  responderOk();
}

// Nulo: el robot no llegó / fue descalificado. El intento cuenta, sin tiempo.
void apiNulo() {
  portENTER_CRITICAL(&mux);
  bool corriendo = (estado == CORRIENDO);
  if (corriendo) estado = TERMINADO;
  int16_t id = idEnCarrera;
  portEXIT_CRITICAL(&mux);
  if (!corriendo) return responderError("No hay un intento en curso");
  finalizarIntento(id, 0, RES_NULO);
  responderOk();
}

// ---- Gestión de equipos ----
void apiNuevo() {
  String n = limpiarNombre(server.arg("nombre"));
  if (n.isEmpty()) n = "Equipo " + String(D.sigId);
  if (agregarEquipo(n) < 0) return responderError("Se alcanzó el máximo de equipos");
  responderOk();
}

void apiRenombrar() {
  int i = buscar(argId());
  String n = limpiarNombre(server.arg("nombre"));
  if (i < 0 || n.isEmpty()) return responderError("Datos inválidos");
  strlcpy(D.eq[i].nombre, n.c_str(), LARGO_NOMBRE); cambio(); responderOk();
}

void apiMover() {                  // cambia el orden de turno (dir = -1 / 1)
  int i = buscar(argId()), d = server.arg("dir").toInt();
  int k = i + (d < 0 ? -1 : 1);
  if (i < 0 || k < 0 || k >= D.nEq) return responderError("No se puede mover");
  Equipo tmp = D.eq[i]; D.eq[i] = D.eq[k]; D.eq[k] = tmp;
  cambio(); responderOk();
}

void apiEliminar() {
  int i = buscar(argId());
  if (i < 0) return responderError("Equipo no existe");
  int id = D.eq[i].id;
  for (int k = i; k < D.nEq - 1; k++) D.eq[k] = D.eq[k + 1];
  D.nEq--;
  if (D.activo == id) D.activo = D.nEq ? D.eq[min(i, D.nEq - 1)].id : -1;
  if (ultId == id) ultId = -1;
  cambio(); responderOk();
}

void apiReiniciarEquipo() {        // borra mejor tiempo e intentos de un equipo
  int i = buscar(argId());
  if (i < 0) return responderError("Equipo no existe");
  limpiarEquipo(D.eq[i]);
  if (ultId == D.eq[i].id) ultId = -1;
  cambio(); responderOk();
}

void apiAnularEquipo() {           // anula el último intento de un equipo
  int i = buscar(argId());
  if (i < 0 || !anularUltimo(i)) return responderError("Ese intento ya no se puede anular");
  if (ultId == D.eq[i].id) ultId = -1;
  responderOk();
}

void apiDevolver() {               // devuelve un intento (sin tocar el mejor)
  int i = buscar(argId());
  if (i < 0 || D.eq[i].usados == 0) return responderError("No tiene intentos usados");
  D.eq[i].usados--; D.eq[i].deshacible = 0; cambio(); responderOk();
}

void apiReiniciarTodo() {
  for (int i = 0; i < D.nEq; i++) limpiarEquipo(D.eq[i]);
  if (D.nEq) D.activo = D.eq[0].id;
  ultId = -1; ultMs = 0; cambio(); responderOk();
}

void apiFabrica() { datosDeFabrica(); ultId = -1; ultMs = 0; responderOk(); }

void apiConfig() {
  if (server.hasArg("auto")) D.autoAvanzar = server.arg("auto").toInt() ? 1 : 0;
  if (server.hasArg("intentos")) {
    int n = server.arg("intentos").toInt();
    if (n < 1 || n > MAX_INTENTOS) return responderError("Intentos fuera de rango (1-20)");
    D.intentosMax = n;
  }
  cambio(); responderOk();
}

void apiRecalibrar() { pedirRecalibrar = true; enviarRecal = true; responderOk(); }

void apiCsv() {                    // CSV separado por ";" (abre bien en Excel)
  // orden por posición
  int orden[MAX_EQUIPOS]; int n = D.nEq;
  for (int i = 0; i < n; i++) orden[i] = i;
  for (int a = 0; a < n; a++) for (int b = a + 1; b < n; b++) {
    uint32_t ma = D.eq[orden[a]].mejor, mb = D.eq[orden[b]].mejor;
    if ((ma == 0 && mb) || (ma && mb && mb < ma)) { int t = orden[a]; orden[a] = orden[b]; orden[b] = t; }
  }
  String c = "\xEF\xBB\xBF" "Posicion;Equipo;Mejor tiempo (ms);Mejor tiempo;Intentos usados\n";
  for (int k = 0; k < n; k++) {
    const Equipo &e = D.eq[orden[k]];
    char tf[16] = "";
    if (e.mejor) snprintf(tf, sizeof(tf), "%lu:%02lu.%03lu", e.mejor / 60000, (e.mejor / 1000) % 60, e.mejor % 1000);
    c += e.mejor ? String(posicionDe(e.id)) : String("-"); c += ";";
    c += e.nombre; c += ";";
    c += e.mejor ? String(e.mejor) : String(""); c += ";";
    c += tf; c += ";";
    c += e.usados; c += "/"; c += D.intentosMax; c += "\n";
  }
  server.sendHeader("Content-Disposition", "attachment; filename=resultados.csv");
  server.send(200, "text/csv; charset=utf-8", c);
}

// =====================================================================
void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) delay(10);   // da tiempo a abrir el Monitor Serie
  Serial.println("\n=== PORTAL META arrancando ===");
  led(40, 0, 0);
  cargarDatos();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS, WIFI_CANAL, 0, 8);
  esp_wifi_set_ps(WIFI_PS_NONE);
  WiFi.setTxPower(WIFI_POTENCIA_TX);      // antena débil: potencia baja = transmisión estable
  Serial.printf("Red WiFi: %s  /  clave: %s\n", AP_SSID, AP_PASS);
  Serial.printf("Abrir: http://%s\n", WiFi.softAPIP().toString().c_str());

  if (esp_now_init() != ESP_OK) { Serial.println("Error ESP-NOW"); ESP.restart(); }
  esp_now_register_recv_cb(alRecibir);

  if (MDNS.begin("cronometro")) MDNS.addService("http", "tcp", 80);

  server.on("/",         []() { server.send_P(200, "text/html", PAGINA_HTML); });
  server.on("/pantalla", []() { server.send_P(200, "text/html", PANTALLA_HTML); });
  server.on("/api/estado",  apiEstado);
  server.on("/api/equipos", apiEquipos);
  server.on("/api/csv",     apiCsv);
  server.on("/api/activo",           HTTP_POST, apiActivo);
  server.on("/api/repetir",          HTTP_POST, apiRepetir);
  server.on("/api/armar",            HTTP_POST, apiArmar);
  server.on("/api/desarmar",         HTTP_POST, apiDesarmar);
  server.on("/api/nulo",             HTTP_POST, apiNulo);
  server.on("/api/equipo/nuevo",     HTTP_POST, apiNuevo);
  server.on("/api/equipo/renombrar", HTTP_POST, apiRenombrar);
  server.on("/api/equipo/mover",     HTTP_POST, apiMover);
  server.on("/api/equipo/eliminar",  HTTP_POST, apiEliminar);
  server.on("/api/equipo/reiniciar", HTTP_POST, apiReiniciarEquipo);
  server.on("/api/equipo/anular",    HTTP_POST, apiAnularEquipo);
  server.on("/api/equipo/devolver",  HTTP_POST, apiDevolver);
  server.on("/api/reiniciar-todo",   HTTP_POST, apiReiniciarTodo);
  server.on("/api/fabrica",          HTTP_POST, apiFabrica);
  server.on("/api/config",           HTTP_POST, apiConfig);
  server.on("/api/recalibrar",       HTTP_POST, apiRecalibrar);
  server.onNotFound([]() { server.sendHeader("Location", "/"); server.send(302); });
  server.begin();

  Serial.println("Calibrando sensor de META: deja el portal despejado...");
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
  portENTER_CRITICAL(&mux);
  Estado e = estado; uint32_t latido = ultimoLatidoMs;
  portEXIT_CRITICAL(&mux);
  int ia = buscar(D.activo);
  uint32_t cuadros = tiraCuadros;
  Serial.println("---------------- DIAGNOSTICO META ----------------");
  if (!inicioConocido)  Serial.println(" Portal INICIO : nunca se ha conectado (¿está encendido? ¿mismo firmware?)");
  else if (!latido)     Serial.println(" Portal INICIO : registrado, pero sin mensajes");
  else Serial.printf(" Portal INICIO : ultimo mensaje hace %lu ms %s\n", millis() - latido,
                     millis() - latido < 2000 ? "(CONECTADO)" : "(DESCONECTADO)");
  uint8_t canal = 0; wifi_second_chan_t sec; esp_wifi_get_channel(&canal, &sec);
  Serial.printf(" Radio         : MAC AP %s, canal %u | recibidos OK %lu, tamano distinto %lu, otra red %lu\n",
                WiFi.softAPmacAddress().c_str(), canal, radioRecibidos, radioTamanoMalo, radioOtraRed);
  Serial.printf(" Carrera       : %s | %s\n", e == CORRIENDO ? "CORRIENDO" : e == TERMINADO ? "TERMINADO" : "SIN CARRERA",
                armado ? "LISTO (armado)" : "ESPERANDO (falta presionar LISTO)");
  if (ia >= 0) Serial.printf(" Equipo pista  : %s (%u/%u intentos usados)\n", D.eq[ia].nombre, D.eq[ia].usados, D.intentosMax);
  else         Serial.println(" Equipo pista  : ninguno");
  Serial.printf(" Sensor        : %s, %u mm (piso %u mm)\n", sensor.ok ? "OK" : "FALLA", sensor.distMm, sensor.baseMm);
  Serial.printf(" Tira          : %s%s\n", nombreModoTira(tiraModo),
                cuadros == cuadrosAnt ? "  <-- ¡la tarea de la tira NO avanza!" : "");
  cuadrosAnt = cuadros;
}

void loop() {
  server.handleClient();
  procesarLlegada();
  revisarTiempoLimite();                  // vuelta que supera el límite -> nula
  guardarSiHaceFalta();

  // Lo que necesita saber el callback de ESP-NOW sobre el equipo en pista
  int ia = buscar(D.activo);
  idActivo = D.activo;
  activoPuedeCorrer = tieneIntentos(ia);
  if (armado && !activoPuedeCorrer) armado = false;   // p. ej. se borró o agotó el equipo

  if (registrarInicio && !inicioConocido) {
    registrarInicio = false;
    agregarPeerInicio();
    Serial.println("Portal de INICIO re-registrado");
  }
  if (responderHola) {
    responderHola = false;
    agregarPeerInicio();
    enviarAInicio(MSG_HOLA_OK);
    Serial.println("Portal de INICIO emparejado");
  }
  if (enviarRecal) { enviarRecal = false; enviarAInicio(MSG_RECALIBRAR); }

  actualizarTiras();                      // luces de ambos portales
  diagnostico();
  delay(1);
}
