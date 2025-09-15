const WebSocket = require('ws');
const express = require('express');
const path = require('path');
const crypto = require('crypto');

// Configuration for Vercel deployment
const CONFIG = {
    WEB_PORT: process.env.PORT || 3000,
    NODE_ENV: process.env.NODE_ENV || 'development'
};

// In-memory storage (use a proper database in production)
const users = new Map();
const sessions = new Map();

// Express server for serving static files
const app = express();
const server = http.createServer(app);

app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// Serve the main HTML file
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
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

// WebSocket server
const wss = new WebSocket.Server({ port: CONFIG.WEBSOCKET_PORT });

console.log(`🚀 WebSocket server started on port ${CONFIG.WEBSOCKET_PORT}`);

wss.on('connection', (ws, req) => {
    console.log('📱 New client connected');
    
    ws.on('message', async (message) => {
        try {
            const data = JSON.parse(message);
            console.log('📨 Received:', data);
            
            switch (data.type) {
                case 'authenticate':
                    await handleAuthentication(ws, data);
                    break;
                    
                case 'control_command':
                    await handleControlCommand(ws, data);
                    break;
                    
                case 'get_status':
                    await handleGetStatus(ws, data);
                    break;
                    
                default:
                    ws.send(JSON.stringify({ 
                        type: 'error', 
                        message: 'Unknown command type' 
                    }));
            }
        } catch (error) {
            console.error('Message handling error:', error);
            ws.send(JSON.stringify({ 
                type: 'error', 
                message: 'Invalid message format' 
            }));
        }
    });
    
    ws.on('close', () => {
        console.log('📱 Client disconnected');
    });
    
    ws.on('error', (error) => {
        console.error('WebSocket error:', error);
    });
});

// Handle client authentication
async function handleAuthentication(ws, data) {
    const { sessionToken } = data;
    
    const session = sessions.get(sessionToken);
    if (!session || !session.authenticated) {
        ws.send(JSON.stringify({ 
            type: 'auth_failed', 
            message: 'Authentication required' 
        }));
        return;
    }
    
    const user = users.get(session.email);
    ws.send(JSON.stringify({ 
        type: 'auth_success', 
        user: { name: user.name, email: user.email } 
    }));
}

// Handle control commands
async function handleControlCommand(ws, data) {
    const { sessionToken, command, room, appliance } = data;
    
    // Verify authentication
    const session = sessions.get(sessionToken);
    if (!session || !session.authenticated) {
        ws.send(JSON.stringify({ 
            type: 'error', 
            message: 'Authentication required' 
        }));
        return;
    }
    
    console.log(`⚡ Control command from ${session.email}: "${command}" for ${room}/${appliance}`);
    
    // Simulate Arduino response (since we're removing hardware connection for deployment)
    const response = simulateArduinoResponse(command);
    
    ws.send(JSON.stringify({ 
        type: 'command_sent', 
        message: `Command "${command}" executed successfully`,
        arduino_response: response
    }));
    
    // Broadcast to all clients
    broadcastToClients({
        type: 'device_update',
        command,
        room,
        appliance,
        response
    });
}

// Handle status requests
async function handleGetStatus(ws, data) {
    const { sessionToken } = data;
    
    const session = sessions.get(sessionToken);
    if (!session || !session.authenticated) {
        ws.send(JSON.stringify({ 
            type: 'error', 
            message: 'Authentication required' 
        }));
        return;
    }
    
    // Simulate system status
    const status = {
        timestamp: new Date().toISOString(),
        devices: {
            'R1': 'OFF',
            'R2': 'OFF'
        },
        system: 'Online'
    };
    
    ws.send(JSON.stringify({ 
        type: 'status_response', 
        status 
    }));
}

// Simulate Arduino responses for demo purposes
function simulateArduinoResponse(command) {
    const responses = {
        'ON': 'All devices turned ON',
        'OFF': 'All devices turned OFF',
        'R1ON': 'Relay 1 turned ON',
        'R1OFF': 'Relay 1 turned OFF',
        'R2ON': 'Relay 2 turned ON',
        'R2OFF': 'Relay 2 turned OFF',
        'STATUS': 'System Status: R1=OFF, R2=OFF',
        'AUTO': 'Automatic mode enabled'
    };
    
    return responses[command] || 'Command processed';
}

// Broadcast message to all connected clients
function broadcastToClients(message) {
    const messageStr = JSON.stringify(message);
    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(messageStr);
        }
    });
}

// Start the express server
server.listen(CONFIG.WEB_PORT, () => {
    console.log(`🌐 Web server started on port ${CONFIG.WEB_PORT}`);
    console.log(`📱 Access the app at: http://localhost:${CONFIG.WEB_PORT}`);
});

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\n🛑 Shutting down gracefully...');
    
    server.close(() => {
        console.log('👋 Server closed');
        process.exit(0);
    });
});

// Error handling
process.on('uncaughtException', (error) => {
    console.error('💥 Uncaught Exception:', error);
});

process.on('unhandledRejection', (reason, promise) => {
    console.error('💥 Unhandled Rejection at:', promise, 'reason:', reason);
});