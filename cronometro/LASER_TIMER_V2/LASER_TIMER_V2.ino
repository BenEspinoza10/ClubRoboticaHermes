
const int LDT[] = {32, 35};
int ll = 0;
double ti = 0, dt = 0;
double vuelta = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(200);
}

void loop() { 
  Serial.println("iniciando temporizador");
  delay(200);
  vuelta = timer();
  Serial.println(" --- --- ---");
  Serial.println("tiempo de vuelta:");
  Serial.println(vuelta);
  Serial.println(" ");
  delay(3000);  
}


double timer() {
  checkLaser(0);
  ti = millis();
  delay(3000);
  checkLaser(1);
  dt = millis() - ti;
  return dt;
}

void checkLaser(int i) {
  ll = analogRead(LDT[i]);
  Serial.print("valor LDT: ");  Serial.println(i);
  Serial.println("");
  Serial.println(ll);
  Serial.println("");
  while (ll > 1000) {
    ll = analogRead(LDT[i]);
  }
}
