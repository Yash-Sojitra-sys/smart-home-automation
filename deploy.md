# Deploy to Vercel - Manual Instructions

## Method 1: Vercel Web Interface (Recommended)

1. **Go to [vercel.com](https://vercel.com)** and sign up/login
2. **Click "New Project"**
3. **Import from Git Repository:**
   - Upload your project folder OR
   - Connect to GitHub (push your code first)
4. **Configure Project:**
   - Framework Preset: **Other**
   - Root Directory: `./`
   - Build Command: `echo "No build required"`
   - Output Directory: `public`
5. **Deploy!**

## Method 2: Vercel CLI

```bash
# Install Vercel CLI globally
npm install -g vercel

# Login to Vercel
vercel login

# Deploy from project directory
cd c:\Users\sojit\Desktop\smart-home-automation
vercel

# Follow the prompts:
# - Set up and deploy? Y
# - Which scope? (your account)
# - Link to existing project? N
# - Project name: smart-home-automation
# - Directory: ./
# - Override settings? N
```

## Method 3: GitHub + Vercel Integration

1. **Create GitHub Repository:**
   ```bash
   # Create repo on GitHub, then:
   git remote add origin https://github.com/yourusername/smart-home-automation.git
   git branch -M main
   git push -u origin main
   ```

2. **Connect to Vercel:**
   - Go to vercel.com
   - Click "New Project"
   - Import from GitHub
   - Select your repository
   - Deploy

## After Deployment

1. **Get your deployment URL** (e.g., `https://smart-home-automation-abc123.vercel.app`)
2. **Update Arduino code:**
   - Replace `https://your-app-name.vercel.app` with your actual URL
   - Upload to Arduino
3. **Test the system:**
   - Open the web interface
   - Create an account
   - Try controlling devices

## Troubleshooting

- If deployment fails, check the build logs
- Ensure all files are committed to git
- Verify vercel.json configuration is correct
- Check that api/index.js exports the Express app correctly
