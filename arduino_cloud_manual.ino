/*
  Smart Home Automation - Cloud Integration (Manual Control Only)
  Arduino UNO/ESP8266 - Manual Control via Web Interface
  
  Hardware Connections:
  - Relay IN1 -> Arduino Pin 12 (Lights)
  - Relay IN2 -> Arduino Pin 11 (Fans/Other devices)
  
  Required Libraries (Install via Arduino IDE Library Manager):
  - ESP8266WiFi (for ESP8266/NodeMCU)
  - ESP8266HTTPClient
  - ArduinoJson
  
  Setup Instructions:
  1. Upload this code to your ESP8266/NodeMCU board
  2. Access your web interface at: https://smart-home-automation-mu.vercel.app/
  3. Control your devices remotely!
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "iPhone";                   // Your WiFi network name
const char* password = "12345678";             // Your WiFi password

// Server URL - Your deployed Vercel URL
const char* serverURL = "https://smart-home-automation-mu.vercel.app";

// Pin definitions
const int RELAY1_PIN = 12;     // Relay channel 1 (IN1) - Lights
const int RELAY2_PIN = 11;     // Relay channel 2 (IN2) - Fans/Other devices

// Variables
bool relay1On = false;
bool relay2On = false;
String inputString = "";

// HTTP client
WiFiClient wifiClient;
HTTPClient http;

// Timing variables
unsigned long lastStatusUpdate = 0;
unsigned long lastCommandCheck = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 30000; // 30 seconds
const unsigned long COMMAND_CHECK_INTERVAL = 10000;  // 10 seconds

void setup() {
  Serial.begin(9600);
  Serial.println("\n=== Smart Home Automation - Cloud Manual Control ===");
  Serial.println("Starting system...");
  
  // Set pin modes
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  
  // Initialize relays to OFF state (Most relay modules are active LOW)
  digitalWrite(RELAY1_PIN, HIGH);  // Turn OFF relay 1
  digitalWrite(RELAY2_PIN, HIGH);  // Turn OFF relay 2
  
  // Connect to WiFi
  connectToWiFi();
  
  Serial.println("=== System Ready ===");
  Serial.println("Manual Control Only - No PIR sensor");
  Serial.println("Web Control: https://smart-home-automation-mu.vercel.app/");
  Serial.println("Commands: ON, OFF, R1ON, R1OFF, R2ON, R2OFF, STATUS, HELP");
  Serial.println("=====================================");
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    connectToWiFi();
  }
  
  // Check for serial commands
  checkSerialCommands();
  
  // Check for cloud commands periodically
  if (millis() - lastCommandCheck > COMMAND_CHECK_INTERVAL) {
    checkCloudCommands();
    lastCommandCheck = millis();
  }
  
  // Send periodic status updates to cloud
  if (millis() - lastStatusUpdate > STATUS_UPDATE_INTERVAL) {
    sendStatusToCloud();
    lastStatusUpdate = millis();
  }
  
  delay(100);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("🌐 Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi connected successfully!");
    Serial.print("📡 IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("🌍 Server: ");
    Serial.println(serverURL);
  } else {
    Serial.println();
    Serial.println("❌ WiFi connection failed! Check credentials.");
  }
}

void checkCloudCommands() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  // Check for pending commands from the web interface
  http.begin(wifiClient, String(serverURL) + "/api/arduino/status");
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    
    // Parse JSON response
    StaticJsonDocument<500> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc["success"]) {
      JsonObject status = doc["status"];
      
      // Update relay states based on cloud status
      bool cloudRelay1 = status["relay1"];
      bool cloudRelay2 = status["relay2"];
      
      // Only update if state changed
      if (cloudRelay1 != relay1On) {
        if (cloudRelay1) {
          turnOnRelay(1);
        } else {
          turnOffRelay(1);
        }
      }
      
      if (cloudRelay2 != relay2On) {
        if (cloudRelay2) {
          turnOnRelay(2);
        } else {
          turnOffRelay(2);
        }
      }
    }
  }
  
  http.end();
}

void sendStatusToCloud() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  // Send R1 status
  http.begin(wifiClient, String(serverURL) + "/api/arduino/status");
  http.addHeader("Content-Type", "application/json");
  
  StaticJsonDocument<200> doc;
  doc["device"] = "R1";
  doc["state"] = relay1On ? "ON" : "OFF";
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    Serial.println("📤 Status sent to cloud: R1=" + String(relay1On ? "ON" : "OFF"));
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
  
  if (httpResponseCode > 0) {
    Serial.println("📤 Status sent to cloud: R2=" + String(relay2On ? "ON" : "OFF"));
  }
  
  http.end();
}

void turnOnAllRelays() {
  turnOnRelay(1);
  turnOnRelay(2);
  Serial.println("🔛 ALL RELAYS ON - Devices Activated");
}

void turnOffAllRelays() {
  turnOffRelay(1);
  turnOffRelay(2);
  Serial.println("⏹️ ALL RELAYS OFF - Devices Deactivated");
}

void turnOnRelay(int relayNum) {
  if (relayNum == 1) {
    digitalWrite(RELAY1_PIN, LOW);  // Active LOW
    relay1On = true;
    Serial.println("💡 RELAY 1 ON (Lights)");
  } else if (relayNum == 2) {
    digitalWrite(RELAY2_PIN, LOW);  // Active LOW
    relay2On = true;
    Serial.println("🌪️ RELAY 2 ON (Fans/Other)");
  }
}

void turnOffRelay(int relayNum) {
  if (relayNum == 1) {
    digitalWrite(RELAY1_PIN, HIGH); // Active LOW
    relay1On = false;
    Serial.println("💡 RELAY 1 OFF (Lights)");
  } else if (relayNum == 2) {
    digitalWrite(RELAY2_PIN, HIGH); // Active LOW
    relay2On = false;
    Serial.println("🌪️ RELAY 2 OFF (Fans/Other)");
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
  
  Serial.println("📨 Command received: " + command);
  
  if (command == "ON") {
    turnOnAllRelays();
    sendStatusToCloud();
    
  } else if (command == "OFF") {
    turnOffAllRelays();
    sendStatusToCloud();
    
  } else if (command == "R1ON") {
    turnOnRelay(1);
    sendStatusToCloud();
    
  } else if (command == "R1OFF") {
    turnOffRelay(1);
    sendStatusToCloud();
    
  } else if (command == "R2ON") {
    turnOnRelay(2);
    sendStatusToCloud();
    
  } else if (command == "R2OFF") {
    turnOffRelay(2);
    sendStatusToCloud();
    
  } else if (command == "STATUS") {
    printStatus();
    
  } else if (command == "WIFI") {
    Serial.print("📶 WiFi Status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected ✅" : "Disconnected ❌");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("📡 IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("📊 Signal Strength: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    }
    
  } else if (command == "HELP") {
    printCommands();
    
  } else if (command.length() > 0) {
    Serial.println("❓ Unknown command: " + command);
    Serial.println("💡 Type 'HELP' for available commands");
  }
}

void printStatus() {
  Serial.println("\n=== 📊 SYSTEM STATUS ===");
  Serial.println("🔧 Mode: MANUAL ONLY");
  Serial.print("💡 Relay 1 (Lights): ");
  Serial.println(relay1On ? "ON ✅" : "OFF ❌");
  Serial.print("🌪️ Relay 2 (Fans): ");
  Serial.println(relay2On ? "ON ✅" : "OFF ❌");
  Serial.print("📶 WiFi: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "Connected ✅" : "Disconnected ❌");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("📡 IP: ");
    Serial.println(WiFi.localIP());
  }
  Serial.print("🌍 Server: ");
  Serial.println(serverURL);
  Serial.println("========================\n");
}

void printCommands() {
  Serial.println("\n=== 📋 AVAILABLE COMMANDS ===");
  Serial.println("ON     - Turn ON both relays");
  Serial.println("OFF    - Turn OFF both relays");
  Serial.println("R1ON   - Turn ON relay 1 (lights)");
  Serial.println("R1OFF  - Turn OFF relay 1 (lights)");
  Serial.println("R2ON   - Turn ON relay 2 (fans)");
  Serial.println("R2OFF  - Turn OFF relay 2 (fans)");
  Serial.println("STATUS - Show current system status");
  Serial.println("WIFI   - Show WiFi connection details");
  Serial.println("HELP   - Show this help menu");
  Serial.println("=============================\n");
}
