# Manual Deployment Guide - Smart Home Automation

## Method 1: Vercel Web Interface (Easiest)

### Step 1: Prepare Project Files
Your project is ready with these key files:
- `api/index.js` - Backend API
- `public/index.html` - Frontend
- `vercel.json` - Configuration
- `package.json` - Dependencies

### Step 2: Create Project Archive
1. **Select all files** in your project folder:
   - api/
   - public/
   - .gitignore
   - arduino_cloud_integration.ino
   - deploy.md
   - netlify.toml
   - package.json
   - README.md
   - server.js
   - vercel.json

2. **Right-click** → **"Send to"** → **"Compressed (zipped) folder"**
3. **Name it**: `smart-home-automation.zip`

### Step 3: Deploy to Vercel
1. **Go to [vercel.com](https://vercel.com)** and sign up/login
2. **Click "Add New..."** → **"Project"**
3. **Look for "Deploy from template" or "Import from archive"** option
4. **Upload your zip file** or drag & drop it
5. **Configure settings:**
   - Project Name: `smart-home-automation`
   - Framework: **Other**
   - Root Directory: `./`
   - Build Command: (leave empty)
   - Output Directory: `public`
   - Install Command: `npm install`

### Step 4: Deploy
Click **"Deploy"** and wait 1-2 minutes.

## Method 2: Alternative - Netlify

If Vercel doesn't work:

1. **Go to [netlify.com](https://netlify.com)** and sign up
2. **Drag and drop** your project folder onto the deploy area
3. **Configure:**
   - Build command: (leave empty)
   - Publish directory: `public`
4. **Deploy**

## After Deployment

You'll get a URL like: `https://smart-home-automation-abc123.vercel.app`

### Update Arduino Code:
1. Open `arduino_cloud_integration.ino`
2. Replace:
   ```cpp
   const char* serverURL = "https://your-app-name.vercel.app";
   ```
   With your actual URL:
   ```cpp
   const char* serverURL = "https://smart-home-automation-abc123.vercel.app";
   ```
3. Update WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
4. Upload to Arduino

## Test Your System
1. Open your deployed web app
2. Create account and login
3. Test device controls
4. Verify Arduino responds to commands

Your smart home automation system will be live and accessible globally! 🏠✨
