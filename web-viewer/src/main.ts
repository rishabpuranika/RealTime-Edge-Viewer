import './style.css'

// DOM Elements
const statusEl = document.getElementById('connection-status') as HTMLElement;
const fpsEl = document.getElementById('fps-counter') as HTMLElement;
const btnToggle = document.getElementById('btn-toggle') as HTMLButtonElement;
const btnSnapshot = document.getElementById('btn-snapshot') as HTMLButtonElement;
const placeholderEl = document.getElementById('placeholder') as HTMLElement;

// State
let isConnected = false;
let isStreaming = true;
let fps = 0;

// Simulate Connection
setTimeout(() => {
  isConnected = true;
  statusEl.innerText = "CONNECTED";
  statusEl.style.color = "#00f3ff";
  placeholderEl.innerHTML = '<div class="message" style="color: #00f3ff">SIGNAL RECEIVED</div>';

  // Start FPS simulation
  setInterval(updateStats, 1000);
}, 2000);

function updateStats() {
  if (!isConnected || !isStreaming) return;

  // Simulate fluctuating FPS
  fps = 28 + Math.floor(Math.random() * 5);
  fpsEl.innerText = fps.toString();
}

// Event Listeners
btnToggle.addEventListener('click', () => {
  isStreaming = !isStreaming;
  btnToggle.innerText = isStreaming ? "PAUSE STREAM" : "RESUME STREAM";

  if (!isStreaming) {
    statusEl.innerText = "PAUSED";
    statusEl.style.color = "#ffaa00";
  } else {
    statusEl.innerText = "CONNECTED";
    statusEl.style.color = "#00f3ff";
  }
});

btnSnapshot.addEventListener('click', () => {
  if (!isConnected) return;

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

  console.log("Snapshot taken");
});
