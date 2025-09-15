const express = require('express');
const path = require('path');
const app = express();
const PORT = 3002;

// Enable CORS and serve static files
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
  res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');
  next();
});

app.use(express.static(path.join(__dirname, 'public')));

// Test endpoint
app.get('/test', (req, res) => {
  res.json({ 
    message: 'Server is working!', 
    timestamp: new Date().toISOString(),
    ip: req.ip 
  });
});

// Mobile app route
app.get('/mobile', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'mobile-app.html'));
});

// Root route
app.get('/', (req, res) => {
  res.send(`
    <h1>Smart Home Test Server</h1>
    <p>Server is running on port ${PORT}</p>
    <p><a href="/mobile">Mobile App</a></p>
    <p><a href="/test">Test API</a></p>
  `);
});

app.listen(PORT, '0.0.0.0', () => {
  console.log(`🚀 Test server running on port ${PORT}`);
  console.log(`📱 Access from phone: http://172.20.10.3:${PORT}/mobile`);
  console.log(`🌐 Local access: http://localhost:${PORT}/mobile`);
});
