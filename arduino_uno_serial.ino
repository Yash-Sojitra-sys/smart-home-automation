/*
  Home Automation System - Arduino UNO
  2-Channel Relay Manual Control
  Serial Communication for Web Control
  
  Hardware Connections:
  - Relay IN1 -> Arduino Pin 12
  - Relay IN2 -> Arduino Pin 11
  
  Upload this code to your Arduino UNO
  Then run the serial bridge server to connect with web interface
*/

// Pin definitions
const int RELAY1_PIN = 12;     // Relay channel 1 (IN1)
const int RELAY2_PIN = 11;     // Relay channel 2 (IN2)

// Variables
bool relay1On = false;         // Track relay 1 status
bool relay2On = false;         // Track relay 2 status
String inputString = "";       // String to hold incoming serial data

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("Home Automation System Starting...");
  
  // Set pin modes
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  
  // Initialize relays to OFF state
  // Note: Most relay modules are active LOW (LOW = ON, HIGH = OFF)
  digitalWrite(RELAY1_PIN, HIGH);  // Turn OFF relay 1
  digitalWrite(RELAY2_PIN, HIGH);  // Turn OFF relay 2
  
  Serial.println("System initialized. Manual control only.");
  Serial.println("PIN Configuration:");
  Serial.println("Relay IN1 -> Pin 12");
  Serial.println("Relay IN2 -> Pin 11");
  Serial.println("------------------------");
  Serial.println("Serial Commands:");
  Serial.println("'ON' - Turn ON both relays");
  Serial.println("'OFF' - Turn OFF both relays");
  Serial.println("'R1ON' - Turn ON relay 1 only");
  Serial.println("'R1OFF' - Turn OFF relay 1 only");
  Serial.println("'R2ON' - Turn ON relay 2 only");
  Serial.println("'R2OFF' - Turn OFF relay 2 only");
  Serial.println("'STATUS' - Show current status");
  Serial.println("------------------------");
  Serial.println("Web Control: Available via Serial Bridge");
  Serial.println("Ready for commands!");
}

void loop() {
  // Check for serial commands only
  checkSerialCommands();
  
  // Small delay to prevent excessive serial output
  delay(100);
}

void turnOnAllRelays() {
  turnOnRelay(1);
  turnOnRelay(2);
  Serial.println(">>> ALL RELAYS ON - Lights/Devices Activated");
}

void turnOffAllRelays() {
  turnOffRelay(1);
  turnOffRelay(2);
  Serial.println(">>> ALL RELAYS OFF - Lights/Devices Deactivated");
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

// Function to check and process serial commands
void checkSerialCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      processCommand(inputString);
      inputString = ""; // Clear the string
    } else {
      inputString += c;
    }
  }
}

// Function to process received commands
void processCommand(String command) {
  command.trim(); // Remove whitespace
  command.toUpperCase(); // Convert to uppercase
  
  Serial.println("Command received: " + command);
  
  if (command == "ON") {
    turnOnAllRelays();
    
  } else if (command == "OFF") {
    turnOffAllRelays();
    
  } else if (command == "R1ON") {
    turnOnRelay(1);
    
  } else if (command == "R1OFF") {
    turnOffRelay(1);
    
  } else if (command == "R2ON") {
    turnOnRelay(2);
    
  } else if (command == "R2OFF") {
    turnOffRelay(2);
    
  } else if (command == "STATUS") {
    printStatus();
    
  } else if (command == "HELP") {
    printHelp();
    
  } else if (command.length() > 0) {
    Serial.println("Unknown command: " + command);
    Serial.println("Type 'HELP' for available commands");
  }
}

// Function to print current system status
void printStatus() {
  Serial.println("=== SYSTEM STATUS ===");
  Serial.println("Mode: MANUAL ONLY");
  Serial.print("Relay 1: ");
  Serial.println(relay1On ? "ON" : "OFF");
  Serial.print("Relay 2: ");
  Serial.println(relay2On ? "ON" : "OFF");
  Serial.println("====================");
}

// Function to print help information
void printHelp() {
  Serial.println("=== AVAILABLE COMMANDS ===");
  Serial.println("ON     - Turn ON both relays");
  Serial.println("OFF    - Turn OFF both relays");
  Serial.println("R1ON   - Turn ON relay 1 only");
  Serial.println("R1OFF  - Turn OFF relay 1 only");
  Serial.println("R2ON   - Turn ON relay 2 only");
  Serial.println("R2OFF  - Turn OFF relay 2 only");
  Serial.println("STATUS - Show current system status");
  Serial.println("HELP   - Show this help menu");
  Serial.println("=========================");
}
