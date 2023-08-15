#include <ESP32Servo.h>
#include <WiFi.h>                                  
#include <WebServer.h>
#include <WebSocketsServer.h> 
#include <ArduinoJson.h>
#include <AccelStepper.h>

//Wifi
const char* ssid = "esp";
const char* password = "12345678";
// Configure IP addresses of the local access point
IPAddress local_IP(192,168,1,22);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
// end wifi

// Server & Websocket
WebServer server(80);
String website = "";
WebSocketsServer webSocket = WebSocketsServer(81);
//end Server & Websocket

// Servos & their Function
#define step 5
Servo servo1;
Servo servo2;
int servo1Pin = 26;
int servo2Pin = 27;

void servo1step(int speed){
  servo1.write(90 + speed);
  delay(250);
  servo1.write(90);
}

void servo2step(int speed){
  servo2.write(90 + speed);
  delay(250);
  servo2.write(90);
}
//End Servos


void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      break;
    case WStype_TEXT:                                 // if a client has sent data, then type == WStype_TEXT
    //define the json object
      StaticJsonDocument<200> doc; 
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        // parsing the json
        const char* obj = doc["obj"];
        Serial.println("Object: " + String(obj));
        

        if (strcmp(obj, "Servo 1") == 0){
          const char* dir = doc["direction"];
          Serial.println("catched servo1");
          if (strcmp(dir, "up") == 0){
            servo1step(10);
          }else{
            servo1step(-5);
          }
          break;
        }else if (strcmp(obj, "Servo 2") == 0){
          const char* dir = doc["direction"];
          Serial.println("catched servo2");
          if (strcmp(dir, "up") == 0){
            servo2step(-20);
          }else{
              servo2step(20);
          }
          
          break;
      }else if (strcmp(obj, "stepper") == 0){
        Serial.println("Catched Stepper");
        const int angle = doc["angle"];
        Serial.println("angle: " + String(angle));
        int steps = angle / 1.8;
        Serial.println("steps: " + String(steps));
      }
      /*
      HERE, the place for the stepper code
      Start with else if (strcmp(obj, "stepper") == 0)
      Parse the JSON if any new field is needed
      call the stepper function whatever it will do
      */

        break;
    }
  }
}





void setup() {
  Serial.begin(115200);                               // init serial port for debugging
  //wifi
  Serial.print("Setting up Access Point ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Starting Access Point ... ");
  Serial.println(WiFi.softAP(ssid, password) ? "Ready" : "Failed!");

  Serial.print("IP address = ");
  Serial.println(WiFi.softAPIP());
  //end wifi

// server shit
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  delay(2000);
 website = "<!DOCTYPE html><html><head> <title>Servo and Stepper Motor Control</title> <style> body { font-family: Arial, sans-serif; background-color: #f5f5f5; } .container { display: flex; justify-content: space-between; margin-top: 50px; width: 800px; margin: 50px auto; } .servo-container, .stepper-container { text-align: center; padding: 20px; background-color: #fff; box-shadow: 0 2px 6px rgba(0, 0, 0, 0.1); border-radius: 6px; } .servo-container { flex-basis: 45%; } .stepper-container { flex-basis: 45%; } h2 { margin-bottom: 10px; } .arrow { display: inline-block; width: 30px; height: 30px; border: 1px solid #000; text-align: center; line-height: 28px; cursor: pointer; margin: 0 5px; } .arrow:before { content: ''; display: inline-block; vertical-align: middle; } .up-arrow:before { border-left: 6px solid transparent; border-right: 6px solid transparent; border-bottom: 6px solid #000; } .down-arrow:before { border-left: 6px solid transparent; border-right: 6px solid transparent; border-top: 6px solid #000; } .stepper-input { width: 80%; padding: 5px; margin: 10px 0; } .rotate-button { background-color: #4CAF50; color: #fff; border: none; padding: 10px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; cursor: pointer; border-radius: 4px; } .websocket-status { display: inline-block; margin-top: 20px; padding: 10px 20px; font-weight: bold; border-radius: 6px; } .connected { background-color: #4CAF50; color: #fff; } .connecting { background-color: #FFA500; color: #fff; } .disconnected { background-color: #FF0000; color: #fff; } </style></head><body> <div class='container'> <div class='servo-container'> <h2>Servo 1</h2> <div class='arrow up-arrow'></div> <div class='arrow down-arrow'></div> </div> <div class='servo-container'> <h2>Servo 2</h2> <div class='arrow up-arrow'></div> <div class='arrow down-arrow'></div> </div> </div> <div class='container'> <div class='stepper-container'> <h2>Stepper Motor</h2> <input type='number' class='stepper-input' id='angle-input' placeholder='Enter angle' /> <button class='rotate-button' id='rotate-button'>Rotate</button> </div> </div> <div class='websocket-status' id='websocket-status'></div> <script> const socket = new WebSocket('ws://' + window.location.hostname + ':81'); const websocketStatus = document.getElementById('websocket-status'); function sendServoData(servoId, direction) { const data = { obj: servoId, direction: direction }; socket.send(JSON.stringify(data)); } function sendStepperData(angle) { const data = { obj: 'stepper', angle: angle }; socket.send(JSON.stringify(data)); } const upArrows = document.querySelectorAll('.up-arrow'); const downArrows = document.querySelectorAll('.down-arrow'); upArrows.forEach((arrow) => { arrow.addEventListener('click', () => { const servoId = arrow.parentElement.querySelector('h2').textContent; sendServoData(servoId, 'up'); }); }); downArrows.forEach((arrow) => { arrow.addEventListener('click', () => { const servoId = arrow.parentElement.querySelector('h2').textContent; sendServoData(servoId, 'down'); }); }); const rotateButton = document.getElementById('rotate-button'); rotateButton.addEventListener('click', () => { const angleInput = document.getElementById('angle-input'); const angle = angleInput.value; sendStepperData(angle); }); socket.onopen = function() { websocketStatus.textContent = 'WebSocket status: Connected'; websocketStatus.className = 'websocket-status connected'; }; socket.onclose = function() { websocketStatus.textContent = 'WebSocket status: Disconnected'; websocketStatus.className = 'websocket-status disconnected'; }; socket.onerror = function() { websocketStatus.textContent = 'WebSocket status: Connecting...'; websocketStatus.className = 'websocket-status connecting'; }; </script></body></html>";
  server.on("/", []() {
  server.send(200, "text/html", website);
  });
  server.begin(); // init the server
  
  //end server shit
  


  // Attach the servos & make sure that their speed = 0
  servo1.attach(servo1Pin);
	servo2.attach(servo2Pin);
  servo1.write(90);
  servo2.write(90);

}

void loop() {
  server.handleClient();
  webSocket.loop();
  //delay(2000);
}


