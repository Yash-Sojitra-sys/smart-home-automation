/*
  Smart Home Automation - Cloud Integration (Updated)
  This Arduino code connects your hardware to the deployed web server
  
  Hardware Connections:
  - Relay IN1 -> Arduino Pin 12 (Controls lights/devices)
  - Relay IN2 -> Arduino Pin 11 (Controls fans/other devices)
  - PIR OUT -> Arduino Pin 13 (Motion detection)
  - ESP8266/WiFi Module for internet connectivity
  
  Required Libraries (Install via Arduino IDE Library Manager):
  - ESP8266WiFi (for ESP8266/NodeMCU)
  - ESP8266HTTPClient
  - ArduinoJson
  
  Setup Instructions:
  1. Update WiFi credentials below
  2. Update serverURL with your deployed Vercel URL
  3. Upload to your ESP8266/NodeMCU board
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// ========== CONFIGURATION - UPDATE THESE ==========
// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";           // Replace with your WiFi network name
const char* password = "YOUR_WIFI_PASSWORD";   // Replace with your WiFi password

// Server URL - Replace with your actual Vercel deployment URL
const char* serverURL = "https://smart-home-automation-xyz.vercel.app";  // Get this from Vercel after deployment

// Pin definitions
const int PIR_PIN = 13;        // PIR sensor output pin
const int RELAY1_PIN = 12;     // Relay channel 1 (IN1) - Lights
const int RELAY2_PIN = 11;     // Relay channel 2 (IN2) - Fans/Other devices

// ========== VARIABLES ==========
int pirState = LOW;
int lastPirState = LOW;
unsigned long motionTime = 0;
const unsigned long DELAY_TIME = 5000; // 5 seconds delay before turning off
bool relay1On = false;
bool relay2On = false;
bool manualMode = false;
String inputString = "";

// HTTP client
WiFiClient wifiClient;
HTTPClient http;

// Timing variables
unsigned long lastStatusUpdate = 0;
unsigned long lastCommandCheck = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 30000; // 30 seconds
const unsigned long COMMAND_CHECK_INTERVAL = 5000;  // 5 seconds

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Smart Home Automation - Cloud Integration ===");
  Serial.println("Starting system...");
  
  // Set pin modes
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  
  // Initialize relays to OFF state (Most relay modules are active LOW)
  digitalWrite(RELAY1_PIN, HIGH);  // Turn OFF relay 1
  digitalWrite(RELAY2_PIN, HIGH);  // Turn OFF relay 2
  
  // Connect to WiFi
  connectToWiFi();
  
  Serial.println("=== System Ready ===");
  Serial.println("Commands: ON, OFF, R1ON, R1OFF, R2ON, R2OFF, AUTO, STATUS, WIFI, HELP");
  Serial.println("Web Control: Access your deployed web app to control remotely");
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
        Serial.println("🔍 Motion detected!");
      } else {
        Serial.println("⏰ Motion stopped. Starting countdown...");
      }
      lastPirState = pirState;
    }
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
  // This function could poll the server for pending commands
  // For now, commands are sent directly via the web interface
  // In a full implementation, you could add HTTP polling here
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
  } else {
    Serial.println("❌ Error sending R1 status to cloud");
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
  if (manualMode) {
    Serial.println("   (Manual Control Active)");
  }
}

void turnOffAllRelays() {
  turnOffRelay(1);
  turnOffRelay(2);
  Serial.println("⏹️ ALL RELAYS OFF - Devices Deactivated");
  if (manualMode) {
    Serial.println("   (Manual Control Active)");
  }
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
    manualMode = true;
    turnOnAllRelays();
    sendStatusToCloud();
    Serial.println("🔧 Manual mode: ON - PIR sensor disabled");
    
  } else if (command == "OFF") {
    manualMode = true;
    turnOffAllRelays();
    sendStatusToCloud();
    Serial.println("🔧 Manual mode: OFF - PIR sensor disabled");
    
  } else if (command == "R1ON") {
    manualMode = true;
    turnOnRelay(1);
    sendStatusToCloud();
    Serial.println("🔧 Manual mode: Relay 1 ON - PIR sensor disabled");
    
  } else if (command == "R1OFF") {
    manualMode = true;
    turnOffRelay(1);
    sendStatusToCloud();
    Serial.println("🔧 Manual mode: Relay 1 OFF - PIR sensor disabled");
    
  } else if (command == "R2ON") {
    manualMode = true;
    turnOnRelay(2);
    sendStatusToCloud();
    Serial.println("🔧 Manual mode: Relay 2 ON - PIR sensor disabled");
    
  } else if (command == "R2OFF") {
    manualMode = true;
    turnOffRelay(2);
    sendStatusToCloud();
    Serial.println("🔧 Manual mode: Relay 2 OFF - PIR sensor disabled");
    
  } else if (command == "AUTO") {
    manualMode = false;
    Serial.println("🤖 Automatic mode: ON - PIR sensor enabled");
    Serial.println("   Both relays will be controlled by motion detection");
    
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
  Serial.print("🔧 Mode: ");
  Serial.println(manualMode ? "MANUAL" : "AUTOMATIC");
  Serial.print("💡 Relay 1 (Lights): ");
  Serial.println(relay1On ? "ON ✅" : "OFF ❌");
  Serial.print("🌪️ Relay 2 (Fans): ");
  Serial.println(relay2On ? "ON ✅" : "OFF ❌");
  Serial.print("🔍 PIR Sensor: ");
  Serial.println(digitalRead(PIR_PIN) ? "MOTION DETECTED 🚶" : "NO MOTION 🚫");
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
  Serial.println("ON     - Turn ON both relays manually");
  Serial.println("OFF    - Turn OFF both relays manually");
  Serial.println("R1ON   - Turn ON relay 1 (lights) only");
  Serial.println("R1OFF  - Turn OFF relay 1 (lights) only");
  Serial.println("R2ON   - Turn ON relay 2 (fans) only");
  Serial.println("R2OFF  - Turn OFF relay 2 (fans) only");
  Serial.println("AUTO   - Return to automatic PIR control");
  Serial.println("STATUS - Show current system status");
  Serial.println("WIFI   - Show WiFi connection details");
  Serial.println("HELP   - Show this help menu");
  Serial.println("=============================\n");
}
