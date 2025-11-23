import './style.css'

// DOM Elements
const statusEl = document.getElementById('connection-status') as HTMLElement;
const fpsEl = document.getElementById('fps-counter') as HTMLElement;
const btnToggle = document.getElementById('btn-toggle') as HTMLButtonElement;
const btnSnapshot = document.getElementById('btn-snapshot') as HTMLButtonElement;
const videoContainer = document.getElementById('video-container') as HTMLElement;
const lowSlider = document.getElementById('low-slider') as HTMLInputElement;
const highSlider = document.getElementById('high-slider') as HTMLInputElement;
const lowVal = document.getElementById('low-val') as HTMLElement;
const highVal = document.getElementById('high-val') as HTMLElement;

// State
let socket: WebSocket | null = null;
let isConnected = false;

// Create Input for IP
const ipInputContainer = document.createElement('div');
ipInputContainer.style.position = 'absolute';
ipInputContainer.style.zIndex = '20';
ipInputContainer.style.display = 'flex';
ipInputContainer.style.flexDirection = 'column';
ipInputContainer.style.alignItems = 'center';
ipInputContainer.style.gap = '10px';

const ipInput = document.createElement('input');
ipInput.type = 'text';
ipInput.placeholder = 'Enter Phone IP (e.g., 192.168.1.5)';
ipInput.style.padding = '10px';
ipInput.style.borderRadius = '5px';
ipInput.style.border = '1px solid #00f3ff';
ipInput.style.background = 'rgba(0,0,0,0.8)';
ipInput.style.color = '#fff';
ipInput.style.fontFamily = 'monospace';

const connectBtn = document.createElement('button');
connectBtn.innerText = "CONNECT TO CONTROLLER";
connectBtn.className = "btn";
connectBtn.style.padding = "10px 20px";

ipInputContainer.appendChild(ipInput);
ipInputContainer.appendChild(connectBtn);
videoContainer.appendChild(ipInputContainer);

connectBtn.addEventListener('click', () => {
  const ip = ipInput.value.trim();
  if (!ip) return;

  connectWebSocket(ip);
});

function connectWebSocket(input: string) {
  let url = input;
  // If input doesn't start with ws:// or wss://, assume it's an IP and prepend ws://
  if (!input.startsWith('ws://') && !input.startsWith('wss://')) {
    url = `ws://${input}`;
  }
  // If input doesn't have a port (and isn't just a protocol+ip), append default port
  // Simple check: if it ends with a digit, append port
  if (/:\d+$/.test(url) === false) {
    url += ':8081';
  }

  console.log("Connecting to:", url);
  socket = new WebSocket(url);

  socket.onopen = () => {
    isConnected = true;
    statusEl.innerText = "CONNECTED";
    statusEl.style.color = "#00f3ff";
    ipInputContainer.style.display = 'none';
    document.getElementById('placeholder')!.style.display = 'none';

    // Start Stream
    const streamIp = url.replace('ws://', '').replace('wss://', '').split(':')[0];
    const imgEl = document.createElement('img');
    imgEl.src = `http://${streamIp}:8080`;
    imgEl.style.width = '100%';
    imgEl.style.height = '100%';
    imgEl.style.objectFit = 'contain';
    imgEl.id = 'stream-img';
    videoContainer.appendChild(imgEl);
  };

  socket.onclose = () => {
    isConnected = false;
    statusEl.innerText = "DISCONNECTED";
    statusEl.style.color = "#ffaa00";
    ipInputContainer.style.display = 'flex';
    document.getElementById('placeholder')!.style.display = 'flex';

    const imgEl = document.getElementById('stream-img');
    if (imgEl) imgEl.remove();
  };

  socket.onmessage = (event) => {
    console.log("Received message:", event.data);
    try {
      const data = JSON.parse(event.data);
      if (data.type === 'fps') {
        fpsEl.innerText = data.value;
      }
    } catch (e) {
      console.error("Error parsing message", e);
    }
  };

  socket.onerror = (err) => {
    console.error("WebSocket error", err);
  };
}

function sendThresholds() {
  if (socket && socket.readyState === WebSocket.OPEN) {
    const low = parseInt(lowSlider.value);
    const high = parseInt(highSlider.value);

    lowVal.innerText = low.toString();
    highVal.innerText = high.toString();

    const payload = JSON.stringify({
      type: 'threshold',
      low: low,
      high: high
    });
    socket.send(payload);
  }
}

lowSlider.addEventListener('input', sendThresholds);
highSlider.addEventListener('input', sendThresholds);

// Event Listeners
btnToggle.addEventListener('click', () => {
  const imgEl = document.getElementById('stream-img') as HTMLImageElement;
  if (imgEl) {
    if (imgEl.style.display === 'none') {
      imgEl.style.display = 'block';
      btnToggle.innerText = "PAUSE STREAM";
    } else {
      imgEl.style.display = 'none';
      btnToggle.innerText = "RESUME STREAM";
    }
  }
});

btnSnapshot.addEventListener('click', () => {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify({ type: 'snapshot' }));

    // Flash effect
    const flash = document.createElement('div');
    flash.style.position = 'fixed';
    flash.style.top = '0';
    flash.style.left = '0';
    flash.style.width = '100%';
    flash.style.height = '100%';
    flash.style.backgroundColor = 'white';
    flash.style.opacity = '0.8';
    flash.style.zIndex = '100';
    flash.style.transition = 'opacity 0.2s';
    document.body.appendChild(flash);

    setTimeout(() => {
      flash.style.opacity = '0';
      setTimeout(() => flash.remove(), 200);
    }, 50);
  }
});
