int a='0';

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {

 //ejemplo de mensajes hacia el arduino
  if(Serial.available()){
    char r=Serial.read();
    Serial.print("Lo que leí es: ");
    Serial.println(r);
  }
  
  

//Ejemplo de mensajes desde el arduino
/*  Serial.print("Mi variable es: ");
  Serial.println(a);
  a=a+1;
  delay(1000);*/
}
