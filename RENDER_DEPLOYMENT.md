# Faculty Management System - Render Deployment Guide

## Linux Server (server2.cpp) Setup

The `server2.cpp` file is a Linux-compatible version of your server using POSIX sockets instead of Winsock2.

### Key Differences from Windows Version:
```
Windows (server.cpp)          →  Linux (server2.cpp)
#include <winsock2.h>         →  #include <netinet/in.h>
WSAStartup()                  →  Removed (not needed on Linux)
WSACleanup()                  →  Removed
SOCKET                        →  int (file descriptor)
closesocket()                 →  close()
setsockopt() with SO_REUSEADDR →  Same function, works on Linux
```

### Compilation on Linux:
```bash
# Install dependencies (if needed)
sudo apt-get update
sudo apt-get install g++ make

# Compile server2.cpp
g++ -std=c++17 -pthread -o server2 server2.cpp

# Run locally
./server2
```

---

## Render Deployment Steps

### 1. **Create a GitHub Repository**
   - Push your project to GitHub (including server2.cpp, public/, CSS, JS files)
   - Structure:
     ```
     your-repo/
     ├── server2.cpp
     ├── public/
     │   ├── index.html
     │   ├── dashboard.html
     │   ├── css/
     │   └── js/
     ├── users.txt
     └── package.json (we'll create this)
     ```

### 2. **Create package.json** (required by Render)
   Create a `package.json` in your project root:
   ```json
   {
     "name": "faculty-management-system",
     "version": "1.0.0",
     "description": "Faculty Management System",
     "scripts": {
       "build": "g++ -std=c++17 -pthread -o server2 server2.cpp",
       "start": "./server2"
     },
     "engines": {
       "node": "18.x"
     }
   }
   ```

### 3. **Create a Render Web Service**
   - Go to [render.com](https://render.com)
   - Sign up or log in with GitHub
   - Click **"New +"** → **"Web Service"**
   - Select your GitHub repository
   - Configure settings:

#### Build Settings:
   ```
   Build Command:    npm run build
   Start Command:    npm start
   ```

#### Environment Variables:
   ```
   PORT=8080
   ```

### 4. **Deploy Configuration**
   - **Name:** faculty-management-system
   - **Environment:** C++
   - **Instance Type:** Free tier is fine for testing
   - **Auto-deploy:** Check "Yes" to auto-deploy on git push

### 5. **Deploy**
   - Click **"Create Web Service"**
   - Wait for deployment (2-3 minutes)
   - Your app will be available at: `https://your-service-name.onrender.com`

---

## Testing Your Live Server

```bash
# Test if server is running
curl https://your-service-name.onrender.com/

# Test faculty API
curl https://your-service-name.onrender.com/api/faculty/list

# Test with search
curl "https://your-service-name.onrender.com/api/faculty/list?search=john"
```

---

## Important Notes

### For Render Free Tier:
- Server spins down after 15 minutes of inactivity
- First request takes 30+ seconds to wake up
- Upgrade to paid tier for always-on service

### File Paths:
- Update file paths in server2.cpp if needed:
  ```cpp
  ifstream file("../public/index.html", ios::binary);  // Adjust path for Render
  ```

### Port Configuration:
- Render automatically sets `PORT` environment variable
- server2.cpp reads it: `const char *portEnv = getenv("PORT");`
- Always bind to `0.0.0.0` (INADDR_ANY) for Render

### Persistent Data:
- `users.txt` must be in root directory
- Use environment variables for credentials
- Consider upgrading to PostgreSQL if you need persistent data

---

## Common Issues & Solutions

| Issue | Solution |
|-------|----------|
| Build fails: `g++: command not found` | Render needs a build script; ensure `package.json` exists |
| Port 8080 already in use | Render handles port assignment; use `getenv("PORT")` |
| Files not found (404 errors) | Check relative paths in `server2.cpp`; adjust `../` as needed |
| Server crashes on startup | Check `users.txt` exists; ensure file paths are correct |
| CORS errors in frontend | server2.cpp includes `Access-Control-Allow-Origin: *` header |

---

## Monitoring & Logs

1. In Render dashboard, click your service
2. Go to **"Logs"** tab to see real-time output
3. Use `cout << "message"` for debugging

---

## Alternative: Deploy on Railway, Heroku, or AWS

If you prefer other platforms:
- **Railway:** Similar process, very easy
- **Heroku:** Similar but requires Procfile
- **AWS EC2:** Full VM, more control, costs money

Would you like help setting up any of these alternatives?
