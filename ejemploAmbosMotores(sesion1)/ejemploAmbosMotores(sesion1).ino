#define A1A 5
#define A1B 6
#define B1A 10
#define B1B 9


void setup() {
  // put your setup code here, to run once:
  pinMode(A1A  , OUTPUT);
  pinMode(A1B  , OUTPUT);
  pinMode(B1A  , OUTPUT);
  pinMode(B1B  , OUTPUT);
}



void loop() {
  // put your main code here, to run repeatedly
  analogWrite(A1A, 150);
  analogWrite(A1B, 0);
  analogWrite(B1A, 150);
  analogWrite(B1B, 0);
  delay(1000);
  analogWrite(A1A, 0);
  analogWrite(A1B, 0);
  analogWrite(B1A, 0);
  analogWrite(B1B, 0);
  delay(1000);  
  analogWrite(A1A, 0);
  analogWrite(A1B, 150);
  analogWrite(B1A, 0);
  analogWrite(B1B, 150);
  delay(1000);
  analogWrite(A1A, 0);
  analogWrite(A1B, 0);
  analogWrite(B1A, 0);
  analogWrite(B1B, 0);
  delay(1000);
}
