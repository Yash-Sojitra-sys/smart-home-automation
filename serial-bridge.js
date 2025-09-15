/*
  Serial Communication Bridge for Arduino UNO
  Connects web app to Arduino via COM Port 9
  
  This server creates a bridge between your web interface and Arduino
  allowing remote control of your home automation system
*/

const express = require('express');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const WebSocket = require('ws');
const path = require('path');
const cors = require('cors');

const app = express();
const PORT = process.env.PORT || 3001;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// Arduino Serial Connection
const ARDUINO_PORT = 'COM9';
const BAUD_RATE = 9600;

let serialPort;
let parser;
let isArduinoConnected = false;
let lastArduinoStatus = {
  relay1: false,
  relay2: false,
  pirState: false,
  mode: 'AUTOMATIC',
  connected: false
};

// WebSocket server for real-time updates
const wss = new WebSocket.Server({ port: 8081 });

// Initialize Arduino connection
function initializeArduino() {
  try {
    serialPort = new SerialPort({
      path: ARDUINO_PORT,
      baudRate: BAUD_RATE,
    });

    parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));

    serialPort.on('open', () => {
      console.log(`✅ Arduino connected on ${ARDUINO_PORT}`);
      isArduinoConnected = true;
      lastArduinoStatus.connected = true;
      broadcastStatus();
      
      // Request initial status
      setTimeout(() => {
        sendToArduino('STATUS');
      }, 2000);
    });

    serialPort.on('error', (err) => {
      console.error('❌ Arduino connection error:', err.message);
      isArduinoConnected = false;
      lastArduinoStatus.connected = false;
      broadcastStatus();
    });

    serialPort.on('close', () => {
      console.log('🔌 Arduino disconnected');
      isArduinoConnected = false;
      lastArduinoStatus.connected = false;
      broadcastStatus();
    });

    // Parse Arduino responses
    parser.on('data', (data) => {
      const message = data.toString().trim();
      console.log('📨 Arduino:', message);
      
      // Parse status updates
      parseArduinoResponse(message);
      
      // Broadcast to all connected clients
      broadcastArduinoMessage(message);
    });

  } catch (error) {
    console.error('❌ Failed to initialize Arduino:', error.message);
    isArduinoConnected = false;
  }
}

// Parse Arduino responses to update status
function parseArduinoResponse(message) {
  const msg = message.toUpperCase();
  
  if (msg.includes('RELAY 1 ON')) {
    lastArduinoStatus.relay1 = true;
  } else if (msg.includes('RELAY 1 OFF')) {
    lastArduinoStatus.relay1 = false;
  }
  
  if (msg.includes('RELAY 2 ON')) {
    lastArduinoStatus.relay2 = true;
  } else if (msg.includes('RELAY 2 OFF')) {
    lastArduinoStatus.relay2 = false;
  }
  
  if (msg.includes('ALL RELAYS ON')) {
    lastArduinoStatus.relay1 = true;
    lastArduinoStatus.relay2 = true;
  } else if (msg.includes('ALL RELAYS OFF')) {
    lastArduinoStatus.relay1 = false;
    lastArduinoStatus.relay2 = false;
  }
  
  if (msg.includes('MOTION DETECTED')) {
    lastArduinoStatus.pirState = true;
  } else if (msg.includes('NO MOTION') || msg.includes('MOTION STOPPED')) {
    lastArduinoStatus.pirState = false;
  }
  
  if (msg.includes('AUTOMATIC MODE')) {
    lastArduinoStatus.mode = 'AUTOMATIC';
  } else if (msg.includes('MANUAL MODE')) {
    lastArduinoStatus.mode = 'MANUAL';
  }
  
  broadcastStatus();
}

// Send command to Arduino
function sendToArduino(command) {
  if (isArduinoConnected && serialPort) {
    console.log('📤 Sending to Arduino:', command);
    serialPort.write(command + '\n');
    return true;
  } else {
    console.error('❌ Arduino not connected');
    return false;
  }
}

// Broadcast status to all WebSocket clients
function broadcastStatus() {
  const statusMessage = JSON.stringify({
    type: 'status',
    data: lastArduinoStatus
  });
  
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(statusMessage);
    }
  });
}

// Broadcast Arduino messages to all WebSocket clients
function broadcastArduinoMessage(message) {
  const arduinoMessage = JSON.stringify({
    type: 'arduino_message',
    data: message
  });
  
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(arduinoMessage);
    }
  });
}

// WebSocket connection handling
wss.on('connection', (ws) => {
  console.log('🔗 New WebSocket client connected');
  
  // Send current status to new client
  ws.send(JSON.stringify({
    type: 'status',
    data: lastArduinoStatus
  }));
  
  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      console.log('📨 WebSocket command:', data);
      
      if (data.type === 'arduino_command' && data.command) {
        sendToArduino(data.command);
      }
    } catch (error) {
      console.error('❌ Invalid WebSocket message:', error.message);
    }
  });
  
  ws.on('close', () => {
    console.log('🔌 WebSocket client disconnected');
  });
});

// REST API Endpoints

// Get Arduino status
app.get('/api/arduino/status', (req, res) => {
  res.json({
    success: true,
    connected: isArduinoConnected,
    status: lastArduinoStatus
  });
});

// API endpoint to send commands to Arduino
app.post('/api/command', (req, res) => {
  const { command } = req.body;
  
  if (!command) {
    return res.status(400).json({ error: 'Command is required' });
  }

  console.log(`🎮 Received command: ${command}`);
  
  // Ensure command has proper format for Arduino
  let arduinoCommand = command;
  if (command === 'R1ON' || command === 'R1OFF') {
    arduinoCommand = command; // Keep as is
  } else if (command === 'R2ON' || command === 'R2OFF') {
    arduinoCommand = command; // Keep as is
  } else if (command.includes('ON')) {
    arduinoCommand = command;
  } else if (command.includes('OFF')) {
    arduinoCommand = command;
  }
  
  if (serialPort && serialPort.isOpen) {
    serialPort.write(arduinoCommand + '\n', (err) => {
      if (err) {
        console.error('❌ Error writing to Arduino:', err);
        return res.status(500).json({ error: 'Failed to send command to Arduino' });
      }
      
      console.log(`📤 Sent to Arduino: ${arduinoCommand}`);
      
      // Update device states based on command
      // updateDeviceState(arduinoCommand);
      
      // Broadcast to all WebSocket clients
      broadcastArduinoMessage(arduinoCommand);
      
      res.json({ 
        success: true, 
        command: arduinoCommand,
        message: 'Command sent to Arduino successfully'
      });
    });
  } else {
    console.error('❌ Arduino not connected');
    res.status(503).json({ error: 'Arduino not connected' });
  }
});

// Control specific devices
app.post('/api/control/:device/:action', (req, res) => {
  const { device, action } = req.params;
  let command = '';
  
  switch (device.toLowerCase()) {
    case 'lights':
    case 'relay1':
    case 'r1':
      command = action.toLowerCase() === 'on' ? 'R1ON' : 'R1OFF';
      break;
    case 'fans':
    case 'relay2':
    case 'r2':
      command = action.toLowerCase() === 'on' ? 'R2ON' : 'R2OFF';
      break;
    case 'all':
      command = action.toLowerCase() === 'on' ? 'ON' : 'OFF';
      break;
    default:
      return res.status(400).json({
        success: false,
        message: 'Invalid device. Use: lights, fans, or all'
      });
  }
  
  const success = sendToArduino(command);
  
  res.json({
    success: success,
    message: success ? `${device} ${action} command sent` : 'Arduino not connected',
    device: device,
    action: action,
    command: command
  });
});

// Set mode (AUTO/MANUAL)
app.post('/api/mode/:mode', (req, res) => {
  const { mode } = req.params;
  let command = '';
  
  if (mode.toLowerCase() === 'auto' || mode.toLowerCase() === 'automatic') {
    command = 'AUTO';
  } else {
    return res.status(400).json({
      success: false,
      message: 'Invalid mode. Use: auto or automatic'
    });
  }
  
  const success = sendToArduino(command);
  
  res.json({
    success: success,
    message: success ? `Mode set to ${mode}` : 'Arduino not connected',
    mode: mode,
    command: command
  });
});

// Serve main page
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Serve mobile app
app.get('/mobile', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'mobile-app.html'));
});

// Start server
app.listen(PORT, () => {
  console.log('🚀 Smart Home Serial Bridge Server Started');
  console.log(`📡 Web Server: http://localhost:${PORT}`);
  console.log(`🔌 WebSocket Server: ws://localhost:8081`);
  console.log(`🤖 Arduino Port: ${ARDUINO_PORT} @ ${BAUD_RATE} baud`);
  console.log('=====================================');
  
  // Initialize Arduino connection
  initializeArduino();
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n🛑 Shutting down server...');
  if (serialPort && serialPort.isOpen) {
    serialPort.close();
  }
  process.exit(0);
});

module.exports = app;
