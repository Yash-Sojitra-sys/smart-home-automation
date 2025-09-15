# Smart Home Automation System

Arduino-based smart home automation with web control panel that can be accessed from anywhere.

## Features

- **Web Interface**: Modern, responsive web app for device control
- **User Authentication**: Secure login and registration system
- **Device Control**: Control Arduino relays via web interface
- **Real-time Status**: Live device status updates
- **Arduino Integration**: WiFi-enabled Arduino code for hardware control
- **Cloud Deployment**: Deployed on Vercel for global access

## Hardware Setup

- Arduino with WiFi capability (ESP8266/ESP32)
- 2-Channel Relay Module
- PIR Motion Sensor
- Connected devices (lights, fans, etc.)

## Pin Connections

- Relay IN1 → Arduino Pin 12
- Relay IN2 → Arduino Pin 11  
- PIR OUT → Arduino Pin 13

## Deployment

This project is configured for deployment on Vercel with serverless functions.

## Usage

1. Access the web interface
2. Create an account and login
3. Select rooms and control devices
4. Arduino automatically syncs with cloud server

## Files

- `api/index.js` - Serverless API backend
- `public/index.html` - Web interface
- `arduino_cloud_integration.ino` - Arduino code with WiFi
- `vercel.json` - Deployment configuration
