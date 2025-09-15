const express = require('express');
const crypto = require('crypto');
const path = require('path');

const app = express();

// Middleware
app.use(express.json());
app.use(express.static(path.join(__dirname, '..', 'public')));

// Serve mobile app
app.get('/mobile', (req, res) => {
  res.sendFile(path.join(__dirname, '..', 'public', 'mobile-app.html'));
});

// In-memory storage (use a database in production)
const users = new Map();
const sessions = new Map();
const deviceStates = {
  relay1: false,
  relay2: false,
  mode: 'MANUAL',
  lastUpdate: Date.now(),
  connected: false
};

// CORS headers for API requests
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
  res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept, Authorization');
  
  if (req.method === 'OPTIONS') {
    res.sendStatus(200);
  } else {
    next();
  }
});

// Helper function to generate session token
function generateSessionToken() {
  return crypto.randomBytes(32).toString('hex');
}

// Helper function to hash password
function hashPassword(password) {
  return crypto.createHash('sha256').update(password).digest('hex');
}

// User registration
app.post('/api/register', (req, res) => {
  const { email, password, name } = req.body;
  
  if (!email || !password || !name) {
    return res.status(400).json({
      success: false,
      message: 'Email, password, and name are required'
    });
  }
  
  if (users.has(email)) {
    return res.status(400).json({
      success: false,
      message: 'User already exists'
    });
  }
  
  const hashedPassword = hashPassword(password);
  users.set(email, {
    email,
    password: hashedPassword,
    name,
    createdAt: new Date().toISOString()
  });
  
  res.json({
    success: true,
    message: 'User registered successfully'
  });
});

// User login
app.post('/api/login', (req, res) => {
  const { email, password } = req.body;
  
  if (!email || !password) {
    return res.status(400).json({
      success: false,
      message: 'Email and password are required'
    });
  }
  
  const user = users.get(email);
  if (!user || user.password !== hashPassword(password)) {
    return res.status(401).json({
      success: false,
      message: 'Invalid credentials'
    });
  }
  
  const sessionToken = generateSessionToken();
  sessions.set(sessionToken, {
    email: user.email,
    name: user.name,
    createdAt: Date.now()
  });
  
  res.json({
    success: true,
    message: 'Login successful',
    sessionToken,
    user: {
      email: user.email,
      name: user.name
    }
  });
});

// Middleware to verify session
function verifySession(req, res, next) {
  const sessionToken = req.headers.authorization?.replace('Bearer ', '');
  
  if (!sessionToken || !sessions.has(sessionToken)) {
    return res.status(401).json({
      success: false,
      message: 'Invalid or expired session'
    });
  }
  
  req.user = sessions.get(sessionToken);
  next();
}

// Get device status
app.get('/api/status', verifySession, (req, res) => {
  res.json({
    success: true,
    devices: deviceStates
  });
});

// Get Arduino status (public endpoint)
app.get('/api/arduino/status', (req, res) => {
  res.json({
    success: true,
    connected: deviceStates.connected,
    status: deviceStates
  });
});

// Control device
app.post('/api/control/:device/:action', verifySession, (req, res) => {
  const { device, action } = req.params;
  
  // Update device states
  if (device === 'relay1' || device === 'lights') {
    deviceStates.relay1 = action === 'on';
  } else if (device === 'relay2' || device === 'fans') {
    deviceStates.relay2 = action === 'on';
  } else if (device === 'all') {
    deviceStates.relay1 = action === 'on';
    deviceStates.relay2 = action === 'on';
  } else {
    return res.status(400).json({
      success: false,
      message: 'Invalid device'
    });
  }
  
  deviceStates.lastUpdate = Date.now();
  
  res.json({
    success: true,
    message: `${device} turned ${action}`,
    devices: deviceStates,
    command: getArduinoCommand(device, action)
  });
});

// Helper function to get Arduino command
function getArduinoCommand(device, action) {
  if (device === 'relay1' || device === 'lights') {
    return action === 'on' ? 'R1ON' : 'R1OFF';
  } else if (device === 'relay2' || device === 'fans') {
    return action === 'on' ? 'R2ON' : 'R2OFF';
  } else if (device === 'all') {
    return action === 'on' ? 'ON' : 'OFF';
  }
  return '';
}

// Send command to Arduino (for cloud integration)
app.post('/api/arduino/command', (req, res) => {
  const { command } = req.body;
  
  if (!command) {
    return res.status(400).json({
      success: false,
      message: 'Command is required'
    });
  }
  
  // Update device states based on command
  const cmd = command.toUpperCase();
  if (cmd === 'ON') {
    deviceStates.relay1 = true;
    deviceStates.relay2 = true;
  } else if (cmd === 'OFF') {
    deviceStates.relay1 = false;
    deviceStates.relay2 = false;
  } else if (cmd === 'R1ON') {
    deviceStates.relay1 = true;
  } else if (cmd === 'R1OFF') {
    deviceStates.relay1 = false;
  } else if (cmd === 'R2ON') {
    deviceStates.relay2 = true;
  } else if (cmd === 'R2OFF') {
    deviceStates.relay2 = false;
  }
  
  deviceStates.lastUpdate = Date.now();
  
  res.json({
    success: true,
    message: 'Command processed',
    command: command,
    devices: deviceStates
  });
});

// Arduino status update endpoint
app.post('/api/arduino/status', (req, res) => {
  const { device, state, timestamp } = req.body;
  
  if (device === 'R1') {
    deviceStates.relay1 = state === 'ON';
  } else if (device === 'R2') {
    deviceStates.relay2 = state === 'ON';
  }
  
  deviceStates.connected = true;
  deviceStates.lastUpdate = Date.now();
  
  res.json({
    success: true,
    message: 'Status updated'
  });
});

// Get user info
app.get('/api/user', verifySession, (req, res) => {
  res.json({
    success: true,
    user: req.user
  });
});

// Logout
app.post('/api/logout', verifySession, (req, res) => {
  const sessionToken = req.headers.authorization?.replace('Bearer ', '');
  sessions.delete(sessionToken);
  
  res.json({
    success: true,
    message: 'Logged out successfully'
  });
});

// Health check
app.get('/api/health', (req, res) => {
  res.json({
    success: true,
    message: 'Smart Home API is running',
    timestamp: new Date().toISOString(),
    mode: 'MANUAL_ONLY'
  });
});

// Serve main page
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, '../public', 'index.html'));
});

// Serve serial control page
app.get('/serial-control', (req, res) => {
  res.sendFile(path.join(__dirname, '../public', 'serial-control.html'));
});

module.exports = app;
