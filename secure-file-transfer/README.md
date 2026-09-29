# SocketVault (Secure File Transfer System)

A secure client-server file transfer system built entirely in **C (POSIX Sockets)**, featuring a modern **React + Vite Web Dashboard** for managing files.

This project is perfectly tailored for a **Computer Networks Lab**, demonstrating raw TCP socket programming along with a lightweight C-based HTTP API server to power the UI.

---

## 🏗️ Architecture
- **TCP Transfer Server**: `server.c` listening on port `9000`. Handles raw chunked TCP file uploads/downloads.
- **HTTP API Server**: `http_api.c` listening on port `8000`. Spun up as a thread by `server.c`. Parses basic HTTP requests to power the frontend (lists files, handles web uploads/downloads).
- **Frontend Dashboard**: React + Vite running on port `5173`. Communicates with the C HTTP API.

---

## 🚀 How to Run the Project Locally

### 1. Start the C Backend (TCP + HTTP)
First, compile and run the C server. This single executable handles both the raw TCP connections and the HTTP REST API.

```bash
cd secure-file-transfer/c_backend
gcc -o server server.c http_api.c -lpthread
./server
```
*(The server will output that it is listening on TCP port 9000 and HTTP port 8000).*

### 2. Start the React Frontend
In a new terminal window, start the beautiful web dashboard:

```bash
cd secure-file-transfer/frontend
npm install
npm run dev
```
*(Open your browser to `http://localhost:5173` to view the dashboard).*

---

## 💻 How to Transfer Files

You have **two** ways to transfer files in this project:

### Method A: Via the Web Dashboard (HTTP)
Simply open the dashboard at `http://localhost:5173`, click **Upload File**, and select a file. You can also click the **Download** button next to any file in the list.

### Method B: Via the Raw C Client (TCP Sockets - For Lab Demo)
To demonstrate raw TCP socket programming for your professor, use the compiled C client.

First, compile it (if not already compiled):
```bash
cd secure-file-transfer/c_backend
gcc -o client client.c
```

**To Upload a file via TCP:**
```bash
# Usage: ./client <IP> <U/D> <filename>
./client 127.0.0.1 U my_test_file.pdf
```
*(Once uploaded, hit "Refresh" in the web dashboard, and you will see it instantly appear!)*

**To Download a file via TCP:**
```bash
./client 127.0.0.1 D my_test_file.pdf
```

---

## 🌐 Deployment (Free Tiers)
1. **Frontend**: The `frontend` folder can be directly connected to **Vercel** or **Netlify** for free static hosting.
2. **C Backend**: You can host the C server on **Render** (Web Service). 
   - Render gives you a free Linux container. You can set the Build Command to `gcc -o server server.c http_api.c -lpthread` and the Start Command to `./server`.
