/*
  Smart Home Automation - Cloud Integration
  This Arduino code connects your hardware to the deployed web server
  
  Hardware Connections:
  - Relay IN1 -> Arduino Pin 12
  - Relay IN2 -> Arduino Pin 11
  - PIR OUT -> Arduino Pin 13
  - ESP8266/WiFi Module for internet connectivity
  
  Required Libraries:
  - ESP8266WiFi (for ESP8266)
  - ESP8266HTTPClient
  - ArduinoJson
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// WiFi credentials - UPDATE THESE WITH YOUR WIFI DETAILS
const char* ssid = "iPhone";                   // Your WiFi network name
const char* password = "12345678";             // Your WiFi password

// Server URL - Your deployed Vercel URL
const char* serverURL = "https://smart-home-automation-mu.vercel.app";

// Pin definitions
const int PIR_PIN = 13;        // PIR sensor output pin
const int RELAY1_PIN = 12;     // Relay channel 1 (IN1)
const int RELAY2_PIN = 11;     // Relay channel 2 (IN2)

// Variables
int pirState = LOW;
int lastPirState = LOW;
unsigned long motionTime = 0;
const unsigned long DELAY_TIME = 5000; // 5 seconds delay
bool relay1On = false;
bool relay2On = false;
bool manualMode = false;
String inputString = "";

// HTTP client
WiFiClient wifiClient;
HTTPClient http;

// Timing variables
unsigned long lastStatusUpdate = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 30000; // 30 seconds

void setup() {
  Serial.begin(9600);
  Serial.println("Smart Home Automation - Cloud Integration Starting...");
  
  // Set pin modes
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  
  // Initialize relays to OFF state
  digitalWrite(RELAY1_PIN, HIGH);  // Turn OFF relay 1
  digitalWrite(RELAY2_PIN, HIGH);  // Turn OFF relay 2
  
  // Connect to WiFi
  connectToWiFi();
  
  Serial.println("System initialized and connected to cloud!");
  printCommands();
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    connectToWiFi();
  }
  
  // Check for serial commands
  checkSerialCommands();
  
  // Check for cloud commands
  checkCloudCommands();
  
  // Only process PIR sensor if not in manual mode
  if (!manualMode) {
    pirState = digitalRead(PIR_PIN);
    
    if (pirState == HIGH) {
      motionTime = millis();
      
      if (!relay1On || !relay2On) {
        turnOnAllRelays();
        sendStatusToCloud();
      }
    }
    
    if ((relay1On || relay2On) && (millis() - motionTime > DELAY_TIME)) {
      turnOffAllRelays();
      sendStatusToCloud();
    }
    
    if (pirState != lastPirState) {
      if (pirState == HIGH) {
        Serial.println("Motion detected!");
      } else {
        Serial.println("Motion stopped. Starting countdown...");
      }
      lastPirState = pirState;
    }
  }
  
  // Send periodic status updates
  if (millis() - lastStatusUpdate > STATUS_UPDATE_INTERVAL) {
    sendStatusToCloud();
    lastStatusUpdate = millis();
  }
  
  delay(100);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
  }
}

void checkCloudCommands() {
  // This function would poll the server for commands
  // For now, we'll rely on serial commands and PIR sensor
  // In a full implementation, you could add HTTP polling here
}

void sendStatusToCloud() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  http.begin(wifiClient, String(serverURL) + "/api/arduino/status");
  http.addHeader("Content-Type", "application/json");
  
  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["device"] = "R1";
  doc["state"] = relay1On ? "ON" : "OFF";
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Status sent to cloud: " + response);
  } else {
    Serial.println("Error sending status to cloud");
  }
  
  http.end();
  
  // Send R2 status
  http.begin(wifiClient, String(serverURL) + "/api/arduino/status");
  http.addHeader("Content-Type", "application/json");
  
  doc.clear();
  doc["device"] = "R2";
  doc["state"] = relay2On ? "ON" : "OFF";
  doc["timestamp"] = millis();
  
  serializeJson(doc, jsonString);
  httpResponseCode = http.POST(jsonString);
  http.end();
}

void turnOnAllRelays() {
  turnOnRelay(1);
  turnOnRelay(2);
  Serial.println(">>> ALL RELAYS ON - Lights/Devices Activated");
  if (manualMode) {
    Serial.println("(Manual Control Active)");
  }
}

void turnOffAllRelays() {
  turnOffRelay(1);
  turnOffRelay(2);
  Serial.println(">>> ALL RELAYS OFF - Lights/Devices Deactivated");
  if (manualMode) {
    Serial.println("(Manual Control Active)");
  }
}

void turnOnRelay(int relayNum) {
  if (relayNum == 1) {
    digitalWrite(RELAY1_PIN, LOW);
    relay1On = true;
    Serial.println(">>> RELAY 1 ON");
  } else if (relayNum == 2) {
    digitalWrite(RELAY2_PIN, LOW);
    relay2On = true;
    Serial.println(">>> RELAY 2 ON");
  }
}

void turnOffRelay(int relayNum) {
  if (relayNum == 1) {
    digitalWrite(RELAY1_PIN, HIGH);
    relay1On = false;
    Serial.println(">>> RELAY 1 OFF");
  } else if (relayNum == 2) {
    digitalWrite(RELAY2_PIN, HIGH);
    relay2On = false;
    Serial.println(">>> RELAY 2 OFF");
  }
}

void checkSerialCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      processCommand(inputString);
      inputString = "";
    } else {
      inputString += c;
    }
  }
}

void processCommand(String command) {
  command.trim();
  command.toUpperCase();
  
  if (command == "ON") {
    manualMode = true;
    turnOnAllRelays();
    sendStatusToCloud();
    Serial.println("Manual mode: ON - PIR sensor disabled");
    
  } else if (command == "OFF") {
    manualMode = true;
    turnOffAllRelays();
    sendStatusToCloud();
    Serial.println("Manual mode: OFF - PIR sensor disabled");
    
  } else if (command == "R1ON") {
    manualMode = true;
    turnOnRelay(1);
    sendStatusToCloud();
    Serial.println("Manual mode: Relay 1 ON - PIR sensor disabled");
    
  } else if (command == "R1OFF") {
    manualMode = true;
    turnOffRelay(1);
    sendStatusToCloud();
    Serial.println("Manual mode: Relay 1 OFF - PIR sensor disabled");
    
  } else if (command == "R2ON") {
    manualMode = true;
    turnOnRelay(2);
    sendStatusToCloud();
    Serial.println("Manual mode: Relay 2 ON - PIR sensor disabled");
    
  } else if (command == "R2OFF") {
    manualMode = true;
    turnOffRelay(2);
    sendStatusToCloud();
    Serial.println("Manual mode: Relay 2 OFF - PIR sensor disabled");
    
  } else if (command == "AUTO") {
    manualMode = false;
    Serial.println("Automatic mode: ON - PIR sensor enabled");
    Serial.println("Both relays will be controlled by motion detection");
    
  } else if (command == "STATUS") {
    printStatus();
    
  } else if (command == "WIFI") {
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
    }
    
  } else if (command == "HELP") {
    printCommands();
    
  } else if (command.length() > 0) {
    Serial.println("Unknown command: " + command);
    Serial.println("Type 'HELP' for available commands");
  }
}

void printStatus() {
  Serial.println("=== SYSTEM STATUS ===");
  Serial.print("Mode: ");
  Serial.println(manualMode ? "MANUAL" : "AUTOMATIC");
  Serial.print("Relay 1: ");
  Serial.println(relay1On ? "ON" : "OFF");
  Serial.print("Relay 2: ");
  Serial.println(relay2On ? "ON" : "OFF");
  Serial.print("PIR Sensor: ");
  Serial.println(digitalRead(PIR_PIN) ? "MOTION DETECTED" : "NO MOTION");
  Serial.print("WiFi: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  Serial.print("Server: ");
  Serial.println(serverURL);
  Serial.println("====================");
}

void printCommands() {
  Serial.println("=== AVAILABLE COMMANDS ===");
  Serial.println("ON     - Turn ON both relays manually");
  Serial.println("OFF    - Turn OFF both relays manually");
  Serial.println("R1ON   - Turn ON relay 1 only");
  Serial.println("R1OFF  - Turn OFF relay 1 only");
  Serial.println("R2ON   - Turn ON relay 2 only");
  Serial.println("R2OFF  - Turn OFF relay 2 only");
  Serial.println("AUTO   - Return to automatic PIR control");
  Serial.println("STATUS - Show current system status");
  Serial.println("WIFI   - Show WiFi connection status");
  Serial.println("HELP   - Show this help menu");
  Serial.println("=========================");
}
