#include <Arduino.h>
#include "ThingsBoard.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include <DHT.h>
//#include <DHTesp.h> 

//Sensor DHT
#define DHTPIN 12  
#define DHTTYPE DHT22
DHT dhtSensor(DHTPIN, DHTTYPE);

//Inisialisasi DHT
//#define DHTPIN 12
//DHTesp dhtSensor;

WiFiClient wifiClient;
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

ThingsBoard tb(wifiClient);

// Search for parameter in HTTP POST request
const char* AP_SSID = "ssid";
const char* AP_PASS = "pass";
const char* AP_IP = "ip";
const char* AP_GATEWAY = "gateway";
const char* TB_SERVER = "tbserver";
const char* TB_TOKEN = "tbtoken";

//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;
String tbserver;
String tbtoken;

String readDHTTemperature() {
  float t = dhtSensor.readTemperature();
//  float t = dhtSensor.getTemperature();
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

//Membaca data Kelembapan dari sensor DHT
String readDHTHumidity() {
  float h = dhtSensor.readHumidity();
//  float h = dhtSensor.getHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(h);
    return String(h);
  }
}

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";
const char* serverPath = "/server.txt";
const char* tokenPath = "/token.txt";

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

// Timer variables
unsigned long previousMillis = 0;

unsigned long sendDataMillis = 0;

int send_delay = 2000;
unsigned long millis_counter;

const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return readDHTTemperature();
  }
  else if(var == "HUMIDITY"){
    return readDHTHumidity();
  }
  return String();
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());


  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

// Replaces placeholder with DHT state value


void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  dhtSensor.begin();
  
  initSPIFFS();
  
  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);

  if(initWiFi()) {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });

    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", readDHTTemperature().c_str());
    });
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", readDHTHumidity().c_str());
    });

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/config.html", "text/html");
    });

    server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == AP_SSID) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == AP_PASS) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == AP_IP) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == AP_GATEWAY) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          // HTTP POST tbserver value
          if (p->name() == TB_SERVER) {
            tbserver = p->value().c_str();
            Serial.print("Thingsboard Server set to: ");
            Serial.println(tbserver);
            // Write file to save value
            writeFile(SPIFFS, serverPath, tbserver.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
          // HTTP POST tbtoken value
          if (p->name() == TB_TOKEN) {
            tbtoken = p->value().c_str();
            Serial.print("Token Device set to: ");
            Serial.println(tbtoken);
            // Write file to save value
            writeFile(SPIFFS, tokenPath, tbtoken.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      
      request->send(200, "text/plain", "DONE!!!. ESP akan restart, koneksikan ke router " + ssid + "dan jalankan pada browser IP Address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
 
    WiFi.softAP("WIFI ESP32", NULL); //→ tanpa password

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html", "text/html");
    });
    
//    server.serveStatic("/", SPIFFS, "/");

     server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/config.html", "text/html");
    });
    
    server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == AP_SSID) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == AP_PASS) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == AP_IP) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == AP_GATEWAY) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          // HTTP POST tbserver value
          if (p->name() == TB_SERVER) {
            tbserver = p->value().c_str();
            Serial.print("Thingsboard Server set to: ");
            Serial.println(tbserver);
            // Write file to save value
            writeFile(SPIFFS, serverPath, tbserver.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
          // HTTP POST tbtoken value
          if (p->name() == TB_TOKEN) {
            tbtoken = p->value().c_str();
            Serial.print("Token Device set to: ");
            Serial.println(tbtoken);
            // Write file to save value
            writeFile(SPIFFS, tokenPath, tbtoken.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      
      request->send(200, "text/plain", "DONE!!!. ESP akan restart, koneksikan ke router " + ssid + "dan jalankan pada browser IP Address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
}

void loop() {
//     if (WiFi.status() != WL_CONNECTED) {
//        reconnect();
//      }
    if (!tb.connected()) {
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(TB_SERVER);
    Serial.print(" with token ");
    Serial.println(TB_TOKEN);
    if (!tb.connect(TB_SERVER, TB_TOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }

 

  // Check if it is a time to send DHT11 temperature and humidity
  if(millis()-millis_counter > send_delay) {
    Serial.println("Sending data...");

    // Uploads new telemetry to ThingsBoard using MQTT.
    // See https://thingsboard.io/docs/reference/mqtt-api/#telemetry-upload-api
    // for more details
    float h = dhtSensor.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dhtSensor.readTemperature();
    
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      Serial.print("Temperature:");
      Serial.print(t);
      Serial.print(" Humidity ");
      Serial.println(h);
      tb.sendTelemetryFloat("temperature", t);
      tb.sendTelemetryFloat("humidity", h);
    }

    millis_counter = millis(); //reset millis counter
  }
      // THINGSBOARD SENDING DATA
     
      tb.loop();
}

//void reconnect() {
//  // Loop until we're reconnected
//  status = WiFi.status();
//  if ( status != WL_CONNECTED) {
//    WiFi.begin(AP_SSID, AP_PASS);
//    while (WiFi.status() != WL_CONNECTED) {
//      delay(500);
//      Serial.print(".");
//    }
//    Serial.println("Connected to AP");
//  }
//}
