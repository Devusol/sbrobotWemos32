// script.js - JavaScript for Robot Control Interface

let eventSource;
let consoleElement;
let scannedNetworks = [];

// Initialize on page load
window.onload = function() {
    consoleElement = document.getElementById('serialConsole');
    initSSE();
    loadWiFiStatus();
    setupEventListeners();
};

// Set up event listeners for buttons and forms
function setupEventListeners() {
    // WiFi form
    document.getElementById('wifiForm').addEventListener('submit', saveWiFi);
    document.getElementById('scanBtn').addEventListener('click', scanNetworks);
    document.getElementById('networkSelect').addEventListener('change', function() {
        document.getElementById('ssid').value = this.value;
    });

    // Robot control buttons
    document.getElementById('forwardBtn').addEventListener('click', () => sendCommand('forward'));
    document.getElementById('backwardBtn').addEventListener('click', () => sendCommand('backward'));
    document.getElementById('leftBtn').addEventListener('click', () => sendCommand('left'));
    document.getElementById('rightBtn').addEventListener('click', () => sendCommand('right'));
    document.getElementById('stopBtn').addEventListener('click', () => sendCommand('stop'));

    // Speed slider
    document.getElementById('speedSlider').addEventListener('input', function() {
        document.getElementById('speedValue').textContent = this.value;
        sendCommand('speed', this.value);
    });
}

// Initialize Server-Sent Events for console
function initSSE() {
    eventSource = new EventSource('/events');
    eventSource.onmessage = function(event) {
        if (consoleElement) {
            consoleElement.textContent += event.data;
            consoleElement.scrollTop = consoleElement.scrollHeight;
        }
    };
    eventSource.onerror = function() {
        setTimeout(initSSE, 5000); // Reconnect after 5 seconds
    };
}

// Load current WiFi status
function loadWiFiStatus() {
    fetch('/status')
        .then(response => response.json())
        .then(data => {
            updateStatusDisplay(data);
        })
        .catch(error => console.error('Error loading status:', error));
}

// Update status display
function updateStatusDisplay(data) {
    const statusAlert = document.getElementById('statusAlert');
    if (data.mode === 'AP') {
        statusAlert.innerHTML = `<div class="alert alert-info">Status: Access Point Mode - ${data.ssid} (${data.ip})</div>`;
    } else if (data.connected) {
        statusAlert.innerHTML = `<div class="alert alert-success">Status: Connected to ${data.ssid} (${data.ip})</div>`;
    } else {
        statusAlert.innerHTML = `<div class="alert alert-warning">Status: Not Connected</div>`;
    }
    document.getElementById('ssid').value = data.ssid || '';
}

// Save WiFi credentials
function saveWiFi(event) {
    event.preventDefault();
    const ssid = document.getElementById('ssid').value;
    const password = document.getElementById('password').value;

    fetch('/save-wifi', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid, password })
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message);
        loadWiFiStatus();
    })
    .catch(error => console.error('Error saving WiFi:', error));
}

// Scan for networks
function scanNetworks() {
    document.getElementById('scanBtn').disabled = true;
    document.getElementById('scanBtn').textContent = 'Scanning...';

    fetch('/scan-networks')
        .then(response => response.json())
        .then(data => {
            scannedNetworks = data.networks;
            updateNetworkSelect();
            document.getElementById('scanBtn').disabled = false;
            document.getElementById('scanBtn').textContent = 'Scan Networks';
        })
        .catch(error => {
            console.error('Error scanning networks:', error);
            document.getElementById('scanBtn').disabled = false;
            document.getElementById('scanBtn').textContent = 'Scan Networks';
        });
}

// Update network select dropdown
function updateNetworkSelect() {
    const select = document.getElementById('networkSelect');
    select.innerHTML = '<option value="">Select from scanned networks...</option>';
    scannedNetworks.forEach(network => {
        const option = document.createElement('option');
        option.value = network.ssid;
        option.textContent = `${network.ssid} (${network.rssi}dBm) ${network.encryption}`;
        select.appendChild(option);
    });
}

// Send robot control command
function sendCommand(command, value = null) {
    const data = { command };
    if (value !== null) data.value = value;

    fetch('/control', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    })
    .then(response => response.json())
    .then(data => {
        if (!data.success) {
            console.error('Command failed:', data.message);
        }
    })
    .catch(error => console.error('Error sending command:', error));
}

// Clear console
function clearConsole() {
    if (consoleElement) {
        consoleElement.textContent = '';
    }
    fetch('/clear-console');
}

// Scroll console to bottom
function scrollToBottom() {
    if (consoleElement) {
        consoleElement.scrollTop = consoleElement.scrollHeight;
    }
}
