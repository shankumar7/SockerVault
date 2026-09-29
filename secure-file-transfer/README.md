# SocketVault (Secure File Transfer System)

A secure client-server file transfer system using TCP and a Web Dashboard for the Computer Networks Lab.

## Architecture
- **Database**: SQLite (built-in, creates `app.db` locally). No external SQL servers required.
- **Backend API**: FastAPI (provides endpoints for the web dashboard).
- **TCP Server**: Raw Python Sockets (handles the actual file transfer and chunking).
- **Frontend**: React + Vite (dashboard for viewing the files).

## Running it Locally

### 1. Run the Backend & TCP Server
```bash
cd secure-file-transfer/backend
pip install -r requirements.txt
PYTHONPATH=. uvicorn app.main:app --host 0.0.0.0 --port 8000
```
*This starts BOTH the FastAPI server on port 8000 and the TCP Server on port 9000.*

### 2. Run the Frontend
```bash
cd secure-file-transfer/frontend
npm install
npm run dev
```
*The web dashboard will be available at http://localhost:5173.*

### 3. Uploading / Downloading Files via TCP Client
To actually transfer files using the raw TCP protocol:
```bash
cd secure-file-transfer
python client/client.py upload my_file.pdf
python client/client.py download my_file.pdf
```

## Deployment (Free Tiers)
1. **Frontend (Vercel/Netlify)**: Push the `frontend` folder to a GitHub repo and connect it to Vercel. It will deploy for free automatically.
2. **Backend + TCP (Render)**: Push the `backend` folder to a GitHub repo and create a "Web Service" on Render.
   - Set the start command to: `uvicorn app.main:app --host 0.0.0.0 --port $PORT`
   - Render allows you to expose a web port, and it will run the background TCP thread.
   - *Note: On free tiers, the local SQLite database will reset if the server sleeps. For a university presentation, this is completely fine as you can just upload files during the demo.*
