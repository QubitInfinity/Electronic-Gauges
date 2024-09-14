/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-nodemcu-access-point-ap-web-server/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


const char* ssid     = "ESP8266-Access-Point";
const char* password = "123456789";
const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0

int sensorValue = 0;  // value read from the pot
float temp = 50.0;
float fuel = 50.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 100;  

const char index_html[] PROGMEM = R"rawliteral()rawliteral";
//const char index_html[] PROGMEM = R"rawliteral(
//<!DOCTYPE HTML><html>
//<head>
//  <meta name="viewport" content="width=device-width, initial-scale=1">
//  <style>
//    html {
//     font-family: Arial;
//     display: inline-block;
//     margin: 0px auto;
//     text-align: center;
//    }
//    h2 { font-size: 3.0rem; }
//    p { font-size: 3.0rem; }
//    .units { font-size: 1.2rem; }
//    .dht-labels{
//      font-size: 1.5rem;
//      vertical-align:middle;
//      padding-bottom: 15px;
//    }
//  </style>
//</head>
//<body>
//  <h2>ESP8266 Server</h2>
//  <p>
//    <span class="dht-labels">Temperature</span> 
//    <span id="temperature">%TEMPERATURE%</span>
//    <sup class="units">&deg;C</sup>
//  </p>
//  <p>
//    <span class="dht-labels">Fuel</span>
//    <span id="fuel">%FUEL%</span>
//    <sup class="units">%</sup>
//  </p>
//
//  <a href="https://bernii.github.io/gauge.js/" target="_blank">Gauge.js Project Page</a>
//
//  
//</body>
//<script>
//setInterval(function ( ) {
//  var xhttp = new XMLHttpRequest();
//  xhttp.onreadystatechange = function() {
//    if (this.readyState == 4 && this.status == 200) {
//      document.getElementById("temperature").innerHTML = this.responseText;
//    }
//  };
//  xhttp.open("GET", "/temperature", true);
//  xhttp.send();
//}, 1000 ) ;
//
//setInterval(function ( ) {
//  var xhttp = new XMLHttpRequest();
//  xhttp.onreadystatechange = function() {
//    if (this.readyState == 4 && this.status == 200) {
//      document.getElementById("fuel").innerHTML = this.responseText;
//    }
//  };
//  xhttp.open("GET", "/fuel", true);
//  xhttp.send();
//}, 1000 ) ;
//
//</script>
//</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(temp);
  }
  else if(var == "FUEL"){
    return String(fuel);
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
//  dht.begin();
  
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temp).c_str());
  });
  server.on("/fuel", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(fuel).c_str());
  });

  // Start server
  server.begin();
}
 
void loop(){  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    sensorValue = analogRead(analogInPin);
    temp = sensorValue/8.53;
//    Serial.print("temp = ");
//    Serial.print(temp);
//    Serial.print("\n");
    
    // save the last time you updated the DHT values
    previousMillis = currentMillis;

    if (isnan(temp)) {
//      Serial.println("Failed to read from DHT sensor!");
    }
    else {
//      Serial.println(temp);
    };
    // if fuel read failed, don't change h value 
    if (isnan(fuel)) {
//      Serial.println("Failed to read from DHT sensor!");
    }
    else {
//      Serial.println(fuel);
    }
  }
}
