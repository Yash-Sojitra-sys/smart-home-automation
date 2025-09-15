const express = require('express');
const path = require('path');
const crypto = require('crypto');

const app = express();

// In-memory storage (in production, use a database like MongoDB or PostgreSQL)
const users = new Map();
const sessions = new Map();
const deviceStates = new Map();

// Initialize device states
deviceStates.set('R1', 'OFF');
deviceStates.set('R2', 'OFF');

app.use(express.json());
app.use(express.static(path.join(__dirname, '../public')));

// Serve the main HTML file
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, '../public', 'index.html'));
});

// API Routes
app.post('/api/register', (req, res) => {
    try {
        const { name, email, password } = req.body;
        
        if (users.has(email)) {
            return res.status(400).json({ error: 'User already exists' });
        }
        
        // Hash password
        const hashedPassword = crypto.createHash('sha256').update(password).digest('hex');
        
        // Store user
        users.set(email, {
            name,
            email,
            password: hashedPassword,
            registered: new Date()
        });
        
        console.log(`👤 New user registered: ${email}`);
        res.json({ success: true, message: 'User registered successfully' });
        
    } catch (error) {
        console.error('Registration error:', error);
        res.status(500).json({ error: 'Registration failed' });
    }
});

app.post('/api/login', (req, res) => {
    try {
        const { email, password } = req.body;
        
        const user = users.get(email);
        if (!user) {
            return res.status(400).json({ error: 'User not found' });
        }
        
        const hashedPassword = crypto.createHash('sha256').update(password).digest('hex');
        if (user.password !== hashedPassword) {
            return res.status(400).json({ error: 'Invalid password' });
        }
        
        // Generate session token
        const sessionToken = crypto.randomBytes(32).toString('hex');
        sessions.set(sessionToken, {
            email,
            loginTime: new Date(),
            authenticated: true
        });
        
        console.log(`🔑 Login successful: ${email}`);
        res.json({ 
            success: true, 
            sessionToken, 
            user: { name: user.name, email: user.email },
            message: 'Login successful' 
        });
        
    } catch (error) {
        console.error('Login error:', error);
        res.status(500).json({ error: 'Login failed' });
    }
});

// Arduino control endpoint
app.post('/api/control', (req, res) => {
    try {
        const { sessionToken, command, room, appliance } = req.body;
        
        // Verify authentication
        const session = sessions.get(sessionToken);
        if (!session || !session.authenticated) {
            return res.status(401).json({ error: 'Authentication required' });
        }
        
        console.log(`⚡ Control command from ${session.email}: "${command}" for ${room}/${appliance}`);
        
        // Process Arduino command
        const response = processArduinoCommand(command);
        
        res.json({ 
            success: true,
            message: `Command "${command}" executed successfully`,
            arduino_response: response,
            device_states: Object.fromEntries(deviceStates)
        });
        
    } catch (error) {
        console.error('Control error:', error);
        res.status(500).json({ error: 'Control command failed' });
    }
});

// Get system status
app.get('/api/status', (req, res) => {
    try {
        const { sessionToken } = req.query;
        
        const session = sessions.get(sessionToken);
        if (!session || !session.authenticated) {
            return res.status(401).json({ error: 'Authentication required' });
        }
        
        const status = {
            timestamp: new Date().toISOString(),
            devices: Object.fromEntries(deviceStates),
            system: 'Online'
        };
        
        res.json({ success: true, status });
        
    } catch (error) {
        console.error('Status error:', error);
        res.status(500).json({ error: 'Failed to get status' });
    }
});

// Arduino webhook endpoint (for Arduino to report status)
app.post('/api/arduino/status', (req, res) => {
    try {
        const { device, state, timestamp } = req.body;
        
        // Update device state
        if (device && state) {
            deviceStates.set(device, state);
            console.log(`📡 Arduino status update: ${device} = ${state}`);
        }
        
        res.json({ success: true, message: 'Status updated' });
        
    } catch (error) {
        console.error('Arduino status error:', error);
        res.status(500).json({ error: 'Failed to update status' });
    }
});

// Process Arduino commands
function processArduinoCommand(command) {
    const responses = {
        'ON': 'All devices turned ON',
        'OFF': 'All devices turned OFF',
        'R1ON': 'Relay 1 turned ON',
        'R1OFF': 'Relay 1 turned OFF',
        'R2ON': 'Relay 2 turned ON',
        'R2OFF': 'Relay 2 turned OFF',
        'STATUS': 'System Status: R1=' + deviceStates.get('R1') + ', R2=' + deviceStates.get('R2'),
        'AUTO': 'Automatic mode enabled'
    };
    
    // Update device states based on command
    switch (command) {
        case 'ON':
            deviceStates.set('R1', 'ON');
            deviceStates.set('R2', 'ON');
            break;
        case 'OFF':
            deviceStates.set('R1', 'OFF');
            deviceStates.set('R2', 'OFF');
            break;
        case 'R1ON':
            deviceStates.set('R1', 'ON');
            break;
        case 'R1OFF':
            deviceStates.set('R1', 'OFF');
            break;
        case 'R2ON':
            deviceStates.set('R2', 'ON');
            break;
        case 'R2OFF':
            deviceStates.set('R2', 'OFF');
            break;
    }
    
    return responses[command] || 'Command processed';
}

// Export for Vercel
module.exports = app;
