#define MIZ A7
#define MDE A6
int lectura_MIZ;
int lectura_MDE;
void setup() {
  pinMode(MIZ, INPUT);
  pinMode(MDE, INPUT);
  Serial.begin(9600);
}

void loop() {
  lectura_MIZ = analogRead(MIZ);
  lectura_MDE = analogRead(MDE);
  Serial.print("Lo que lee el sensor izquierdo: ");
  Serial.print(lectura_MIZ);
  Serial.print("Lo que lee el sensor derecho: ");
  Serial.println(lectura_MDE);
  delay(100);
}
