@echo off
title Arduino Home Automation Server
color 0A
echo.
echo ========================================
echo    Arduino Home Automation Server
echo ========================================
echo.
echo Checking Node.js installation...
node --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Node.js is not installed or not in PATH
    echo Please install Node.js from https://nodejs.org/
    echo.
    pause
    exit /b 1
)
echo Node.js found!
echo.

echo Installing/updating dependencies...
call npm install
if %errorlevel% neq 0 (
    echo.
    echo WARNING: npm install encountered issues
    echo Continuing anyway...
    echo.
)

echo.
echo Starting Arduino Home Automation Server...
echo.
echo Web Interface: http://localhost:3000
echo WebSocket Server: ws://localhost:3001
echo.
echo Instructions:
echo 1. Make sure your Arduino is connected via USB
echo 2. Close Arduino IDE Serial Monitor if open
echo 3. Open browser to http://localhost:3000
echo 4. Click "Connect to Arduino" if not auto-connected
echo.
echo Press Ctrl+C to stop the server
echo ========================================
echo.

node server.js

echo.
echo Server stopped.
pause