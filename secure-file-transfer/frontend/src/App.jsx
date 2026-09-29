import { useState, useEffect } from 'react'
import './index.css'

function App() {
  const [files, setFiles] = useState([])
  const [username, setUsername] = useState('demo_user')
  const [password, setPassword] = useState('password')
  const [token, setToken] = useState(null)
  const [isLoading, setIsLoading] = useState(false)
  
  const login = async () => {
    setIsLoading(true)
    const formData = new FormData()
    formData.append('username', username)
    formData.append('password', password)
    
    try {
      const res = await fetch('http://localhost:8000/api/auth/login', {
        method: 'POST',
        body: formData
      })
      if(res.ok) {
        const data = await res.json()
        setToken(data.access_token)
        fetchFiles(data.access_token)
      } else {
        alert('Login failed. Ensure C server is running and try registering.')
      }
    } catch(err) {
      console.error(err)
    } finally {
      setIsLoading(false)
    }
  }
  
  const register = async () => {
    setIsLoading(true)
    try {
      const res = await fetch('http://localhost:8000/api/auth/register', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({username, password})
      })
      if(res.ok || res.status === 200) {
        login()
      } else {
        alert('Registration failed')
      }
    } catch(err) {
      console.error(err)
    } finally {
      setIsLoading(false)
    }
  }

  const fetchFiles = async (authToken) => {
    try {
      const res = await fetch('http://localhost:8000/api/files', {
        headers: {'Authorization': `Bearer ${authToken}`}
      })
      if(res.ok) {
        const data = await res.json()
        setFiles(data)
      }
    } catch(err) {
      console.error(err)
    }
  }
  
  const deleteFile = async (id) => {
    try {
      const res = await fetch(`http://localhost:8000/api/files/${id}`, {
        method: 'DELETE',
        headers: {'Authorization': `Bearer ${token}`}
      })
      if(res.ok) {
        fetchFiles(token)
      }
    } catch(err) {
      console.error(err)
    }
  }

  const formatBytes = (bytes) => {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  }

  const uploadFile = async (event) => {
    const file = event.target.files[0];
    if (!file) return;

    setIsLoading(true);
    try {
      const res = await fetch('http://localhost:8000/api/upload', {
        method: 'POST',
        headers: {
          'X-Filename': file.name,
          'Content-Length': file.size.toString()
        },
        body: file
      });
      if (res.ok) {
        alert('File uploaded successfully!');
        fetchFiles(token);
      } else {
        alert('Upload failed.');
      }
    } catch(err) {
      console.error(err);
      alert('Upload error.');
    } finally {
      setIsLoading(false);
      event.target.value = null; // reset input
    }
  }

  const downloadFile = (filename) => {
    window.open(`http://localhost:8000/api/download/${filename}`, '_blank');
  }

  return (
    <div className="container">
      <h1>SocketVault</h1>
      
      {!token ? (
        <div className="card" style={{maxWidth: '400px', margin: '0 auto'}}>
          <h2>Welcome Back</h2>
          <div className="input-group">
            <input 
              value={username} 
              onChange={e=>setUsername(e.target.value)} 
              placeholder="Username" 
            />
          </div>
          <div className="input-group">
            <input 
              type="password" 
              value={password} 
              onChange={e=>setPassword(e.target.value)} 
              placeholder="Password" 
            />
          </div>
          <div style={{display: 'flex', gap: '10px', marginTop: '1.5rem'}}>
            <button className="btn" style={{flex: 1}} onClick={login} disabled={isLoading}>
              {isLoading ? '...' : 'Login'}
            </button>
            <button className="btn btn-secondary" style={{flex: 1}} onClick={register} disabled={isLoading}>
              Register
            </button>
          </div>
        </div>
      ) : (
        <div className="card">
          <div className="header-actions">
            <div>
              <h2>Dashboard</h2>
              <span style={{color: 'var(--text-muted)'}}>Logged in as <strong>{username}</strong></span>
            </div>
            <div style={{display: 'flex', gap: '10px'}}>
              <button className="btn btn-secondary" onClick={() => fetchFiles(token)}>
                <svg style={{width: '18px', marginRight: '6px'}} fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" /></svg>
                Refresh
              </button>
              <button className="btn btn-secondary" onClick={() => setToken(null)}>Logout</button>
            </div>
          </div>
          
          <div className="notice" style={{display: 'flex', justifyContent: 'space-between', alignItems: 'center'}}>
            <div>
              <strong>Web Upload:</strong> You can upload files directly via the browser now.<br/>
              <span style={{fontSize: '0.85rem'}}>For raw TCP upload, use: <code>./client 127.0.0.1 U my_file.pdf</code></span>
            </div>
            <div>
              <input type="file" id="file-upload" style={{display: 'none'}} onChange={uploadFile} />
              <label htmlFor="file-upload" className="btn" style={{cursor: 'pointer', background: '#10b981'}}>
                Upload File
              </label>
            </div>
          </div>
          
          <div className="table-wrapper">
            <table>
              <thead>
                <tr>
                  <th>Filename</th>
                  <th>Size</th>
                  <th>Status</th>
                  <th style={{textAlign: 'right'}}>Actions</th>
                </tr>
              </thead>
              <tbody>
                {files.map(f => (
                  <tr key={f.id}>
                    <td style={{fontWeight: 500}}>{f.filename}</td>
                    <td>{formatBytes(f.size)}</td>
                    <td><span className="badge">Verified TCP</span></td>
                    <td style={{textAlign: 'right', display: 'flex', gap: '8px', justifyContent: 'flex-end'}}>
                      <button className="btn btn-secondary" style={{padding: '0.25rem 0.75rem', fontSize: '0.85rem'}} onClick={() => downloadFile(f.filename)}>
                        Download
                      </button>
                      <button className="btn btn-danger" onClick={() => deleteFile(f.id)}>
                        Delete
                      </button>
                    </td>
                  </tr>
                ))}
                {files.length === 0 && (
                  <tr>
                    <td colSpan="4" style={{textAlign: 'center', padding: '3rem 1rem', color: 'var(--text-muted)'}}>
                      No files found. Try uploading a file via the C TCP client.
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>
        </div>
      )}
    </div>
  )
}

export default App
