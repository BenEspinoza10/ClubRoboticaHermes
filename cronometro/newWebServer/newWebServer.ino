#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebSocketsServer.h>
#include <WebServer.h>

String old_value, value;

WiFiMulti WiFiMulti;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);


//ESTADO DEL PROGRAMA
int globalState = 1;
// PINES LÁSER
int sensor1Pin = 32;
int sensor2Pin = 35;

//VARIABLES PARA TIEMPO
unsigned long globalStartTime = 0;
unsigned long totalStartTime = 0;
unsigned long totalTime = 0;
unsigned long globalTime = 0;
String JSONtxt;

char html_template[] PROGMEM = R"=====(
<html lang="en">
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Cronómetro FIRVI</title>      
    </head>
    <style>
        html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
        #btnRonda
        {
          background-color: #4CAF50; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;display: block;
        }
        #btnIntento
        {
          background-color: #cc0605; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;display: block;
        }         
        h1   {color: rgba(0, 0, 255, 0.87); font-size: 50px;}
      </style>
      <body style="max-width:400px;margin: auto;font-family:Arial, Helvetica, sans-serif;text-align:center">
          <div><h1><br />FIRVI 2023<br/></h1></div>
          <div><h2><br />Categoría velocistas<br/></h2></div>
          <div><p id="sensor1"></p><p id="sensor2"><p></div>
          <table>
           <tr>
            <td>
              <h2 id="rondaHeader"></h2>
            <td>
            <td>
              <h2 id="intentoHeader"></h2>
            <td>
           </tr>
           <tr>
              <td>
                <p id="tiempoRonda" style="font-size:100px;margin:0"></p>
              </td>
              <td>
                <p id="tiempoIntento" style="font-size:100px;margin:0"></p>
              </td>
           </tr>
          </table>                   
          <div><a href="#" id="btnRonda" ONCLICK='button()'> </a></div>
          <div><a href="#" id="btnIntento" ONCLICK='button2()'> </a></div>


          <script>
          InitWebSocket()
          function InitWebSocket(){
              socket = new WebSocket("ws:/" + "/" + location.host + ":81");
              socket.onopen = function(e) {  console.log("[socket] socket.onopen "); };
              socket.onerror = function(e) {  console.log("[socket] socket.onerror "); };
              socket.onmessage = function(e) {  
                  console.log("[socket] " + e.data);
                  JSONobj = JSON.parse(e.data);
                  
                  if(JSONobj.State == '1')
                  {
                    document.getElementById('btnRonda').style.background='#12a14b';                    
                    document.getElementById('btnRonda').innerHTML = "Comenzar Ronda";
                    document.getElementById('tiempoRonda').innerHTML = "";
                    document.getElementById('rondaHeader').innerHTML = "";
                    document.getElementById('tiempoIntento').innerHTML = "";
                    document.getElementById('intentoHeader').innerHTML = "";
                    document.getElementById('sensor1').innerHTML = "Sensor 1: "+JSONobj.sensor1;
                    document.getElementById('sensor2').innerHTML = "Sensor 2: "+JSONobj.sensor2;
                    document.getElementById('btnIntento').innerHTML = "";
                    document.getElementById('btnIntento').style.background='#ffffff'; 
                  }
                  else if (JSONobj.State == '2')
                  {
                    document.getElementById('btnRonda').style.background='#555555';                    
                    document.getElementById('btnRonda').innerHTML = "Reiniciar Ronda";
                    document.getElementById('tiempoRonda').innerHTML = JSONobj.globalTime;
                    document.getElementById('sensor1').innerHTML = "Sensor 1: "+JSONobj.sensor1;
                    document.getElementById('sensor2').innerHTML = "Sensor 2: "+JSONobj.sensor2;
                    document.getElementById('rondaHeader').innerHTML = "Tiempo total de ronda";
                    document.getElementById('tiempoIntento').innerHTML = "";
                    document.getElementById('intentoHeader').innerHTML = "";
                    document.getElementById('btnIntento').innerHTML = "";
                    document.getElementById('btnIntento').style.background='#ffffff';    
                  }
                  else if (JSONobj.State == '3')
                  {
                    document.getElementById('btnRonda').style.background='#555555';                    
                    document.getElementById('btnRonda').innerHTML = "Reiniciar Ronda";
                    document.getElementById('tiempoRonda').innerHTML = JSONobj.globalTime;
                    document.getElementById('sensor1').innerHTML = "Sensor 1: "+JSONobj.sensor1;
                    document.getElementById('sensor2').innerHTML = "Sensor 2: "+JSONobj.sensor2;
                    document.getElementById('rondaHeader').innerHTML = "Tiempo total de ronda";
                    document.getElementById('tiempoIntento').innerHTML = JSONobj.Time;
                    document.getElementById('intentoHeader').innerHTML = "Tiempo Intento";
                    document.getElementById('btnIntento').innerHTML = "Reiniciar Intento";
                    document.getElementById('btnIntento').style.background='#4CAF50';    
                  }
                  else if (JSONobj.State == '4')
                  {
                    document.getElementById('btnRonda').style.background='#555555';                    
                    document.getElementById('btnRonda').innerHTML = "Reiniciar Ronda";
                    document.getElementById('tiempoRonda').innerHTML = JSONobj.globalTime;
                    document.getElementById('sensor1').innerHTML = "Sensor 1: "+JSONobj.sensor1;
                    document.getElementById('sensor2').innerHTML = "Sensor 2: "+JSONobj.sensor2;
                    document.getElementById('rondaHeader').innerHTML = "Tiempo total de ronda";
                    document.getElementById('tiempoIntento').innerHTML = JSONobj.Time;
                    document.getElementById('intentoHeader').innerHTML = "Tiempo Final Intento";
                    document.getElementById('btnIntento').innerHTML = "Reiniciar Intento";
                    document.getElementById('btnIntento').style.background='#4CAF50';    
                  }
                }
              }
          
              function button(){
                if(document.getElementById('btnRonda').innerHTML == 'Comenzar Ronda')
                {
                  btn = 'status=2';
                }else if (document.getElementById('btnRonda').innerHTML == 'Reiniciar Ronda'){
                  btn = 'status=1';
                }
                socket.send(btn);
              };

              function button2(){
                btn = 'status=3'
                socket.send(btn);
              }
          </script> 
      </body>
             
</html>
)=====";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  String payloadString = (const char *)payload;
  Serial.print("payloadString= ");
  Serial.println(payloadString);
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // send message to client
        webSocket.sendTXT(num, "0");
      }
      break;

    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      byte separator=payloadString.indexOf('=');
      String var = payloadString.substring(0,separator);
      Serial.print("var= ");
      Serial.println(var);
      String val = payloadString.substring(separator+1);
      Serial.print("val= ");
      Serial.println(val);
      Serial.println(" ");

      if(var == "status")
      {     
        //comienzo de ronda   
        if(val == "2"){
          globalState = 2;
          globalStartTime = millis();
        }
        //comienzo de intento /reinicio intento
        else if(val == "3"){
          globalState = 2;
        }
        //reinicioronda
        else{
          globalState = 1;
        }              
      }
      break;       
  }
}

void handleMain() {
  server.send_P(200, "text/html", html_template);
}
void handleNotFound() {
  server.send(404, "text/html", "<html><body><p>404 Error</p></body></html>");
}

void setup() {

  Serial.begin(115200);
  delay(50);

  WiFiMulti.addAP("Multivac", "hola1234");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(50);
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/", handleMain);
  server.onNotFound(handleNotFound);
  server.begin();
}




void loop() {

  webSocket.loop();
  server.handleClient();


  //Estado de inicio
  if (globalState == 1){ 
    JSONtxt = "{\"State\":\""+(String)globalState+"\", \"Time\":\""+(String)totalTime+"\", \"globalTime\":\""+(String)globalTime+"\", \"sensor1\":\""+String(analogRead(sensor1Pin))+"\", \"sensor2\":\""+String(analogRead(sensor2Pin))+"\"   }";
    webSocket.broadcastTXT(JSONtxt);
  //Inicio ronda
  }else if(globalState == 2){
    //tiempo ronda
    globalTime = (360000 - (millis()-globalStartTime))/1000;
    int segundos = globalTime % 60;
    globalTime /= 60;
    int minutos = globalTime % 60;    
    JSONtxt = "{\"State\":\""+(String)globalState+"\", \"Time\":\""+(String)totalTime+"\", \"globalTime\":\""+(String)minutos+":"+ (String)segundos+"\", \"sensor1\":\""+String(analogRead(sensor1Pin))+"\", \"sensor2\":\""+String(analogRead(sensor2Pin))+"\"   }";
    webSocket.broadcastTXT(JSONtxt);
    //detección laser 1 (inicio)
    if (analogRead(sensor1Pin)<300){
      globalState = 3;
      totalStartTime= millis();
    }
  }
  //Inicio Intento
  else if(globalState == 3){
    //tiempo ronda
    globalTime = (360000 - (millis()-globalStartTime))/1000;
    int segundos = globalTime % 60;
    globalTime /= 60;
    int minutos = globalTime % 60;    
    //tiempo intento
    totalTime = millis()-totalStartTime;
    String stringTotalTime = (String)totalTime;
    if (totalTime < 1000){
      JSONtxt = "{\"State\":\""+(String)globalState+"\", \"Time\":\""+stringTotalTime+"\", \"globalTime\":\""+(String)minutos+":"+ (String)segundos+"\", \"sensor1\":\""+String(analogRead(sensor1Pin))+"\", \"sensor2\":\""+String(analogRead(sensor2Pin))+"\"   }";
    }else if(totalTime < 10000){      
      JSONtxt = "{\"State\":\""+(String)globalState+"\", \"Time\":\""+stringTotalTime.substring(0,1)+","+stringTotalTime.substring(1)+"\", \"globalTime\":\""+(String)minutos+":"+ (String)segundos+"\", \"sensor1\":\""+String(analogRead(sensor1Pin))+"\", \"sensor2\":\""+String(analogRead(sensor2Pin))+"\"   }";
    }else{
      JSONtxt = "{\"State\":\""+(String)globalState+"\", \"Time\":\""+stringTotalTime.substring(0,2)+","+stringTotalTime.substring(2)+"\", \"globalTime\":\""+(String)minutos+":"+ (String)segundos+"\", \"sensor1\":\""+String(analogRead(sensor1Pin))+"\", \"sensor2\":\""+String(analogRead(sensor2Pin))+"\"   }";
    }    
    webSocket.broadcastTXT(JSONtxt);
    //detección laser 2 (fin)
    if (analogRead(sensor2Pin)<300){
      globalState = 4;      
    }
  }//Fin intento
  else if(globalState == 4){
    //tiempo ronda
    globalTime = (360000 - (millis()-globalStartTime))/1000;
    int segundos = globalTime % 60;
    globalTime /= 60;
    int minutos = globalTime % 60;    
    //tiempo intento    
    String stringTotalTime = (String)totalTime;
    if (totalTime < 1000){
      JSONtxt = "{\"State\":\""+(String)globalState+"\", \"Time\":\"0,"+stringTotalTime+"\", \"globalTime\":\""+(String)minutos+":"+ (String)segundos+"\", \"sensor1\":\""+String(analogRead(sensor1Pin))+"\", \"sensor2\":\""+String(analogRead(sensor2Pin))+"\"   }";
    }else if(totalTime < 10000){      
      JSONtxt = "{\"State\":\""+(String)globalState+"\", \"Time\":\""+stringTotalTime.substring(0,1)+","+stringTotalTime.substring(1)+"\", \"globalTime\":\""+(String)minutos+":"+ (String)segundos+"\", \"sensor1\":\""+String(analogRead(sensor1Pin))+"\", \"sensor2\":\""+String(analogRead(sensor2Pin))+"\"   }";
    }else{
      JSONtxt = "{\"State\":\""+(String)globalState+"\", \"Time\":\""+stringTotalTime.substring(0,2)+","+stringTotalTime.substring(2)+"\", \"globalTime\":\""+(String)minutos+":"+ (String)segundos+"\", \"sensor1\":\""+String(analogRead(sensor1Pin))+"\", \"sensor2\":\""+String(analogRead(sensor2Pin))+"\"   }";
    }   
    webSocket.broadcastTXT(JSONtxt);
  }
  delay(50);
}

