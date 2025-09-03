// script.js - JavaScript for Robot Control Interface with WebSockets

let eventSource;
let consoleElement;
let webSocket;
let scannedNetworks = [];

// Initialize on page load
window.onload = function() {
    consoleElement = document.getElementById('serialConsole');
    initSSE();
    initWebSocket();
    loadWiFiStatus();
    setupEventListeners();
};

// Initialize WebSocket connection
function initWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = protocol + '//' + window.location.host + ':81';

    console.log('Connecting to WebSocket at:', wsUrl);
    webSocket = new WebSocket(wsUrl);

    webSocket.onopen = function(event) {
        console.log('WebSocket connected');
        updateConnectionStatus('Connected', 'success');
    };

    webSocket.onmessage = function(event) {
        console.log('WebSocket message:', event.data);
        handleWebSocketMessage(event.data);
    };

    webSocket.onclose = function(event) {
        console.log('WebSocket disconnected');
        updateConnectionStatus('Disconnected', 'danger');
        // Auto-reconnect after 3 seconds
        setTimeout(initWebSocket, 3000);
    };

    webSocket.onerror = function(error) {
        console.error('WebSocket error:', error);
        updateConnectionStatus('Connection Error', 'warning');
    };
}

// Handle incoming WebSocket messages
function handleWebSocketMessage(message) {
    try {
        const data = JSON.parse(message);

        if (data.status === 'received') {
            console.log('Command acknowledged:', data.command);
        } else if (data.response === 'pong') {
            console.log('Ping response received');
        } else {
            console.log('Unknown message:', data);
        }
    } catch (e) {
        // Not JSON, treat as plain text
        console.log('Text message:', message);
    }
}

// Update connection status display
function updateConnectionStatus(status, type) {
    const statusAlert = document.getElementById('statusAlert');
    if (statusAlert) {
        // Add WebSocket status to existing alert
        const wsStatus = document.createElement('div');
        wsStatus.id = 'wsStatus';
        wsStatus.className = 'mt-1';
        wsStatus.innerHTML = `<small><strong>WebSocket:</strong> <span class="badge bg-${type}">${status}</span></small>`;
        statusAlert.appendChild(wsStatus);
    }
}

// Set up event listeners for buttons and forms
function setupEventListeners() {
    // WiFi form
    document.getElementById('wifiForm').addEventListener('submit', saveWiFi);
    document.getElementById('scanBtn').addEventListener('click', scanNetworks);
    document.getElementById('networkSelect').addEventListener('change', function() {
        document.getElementById('ssid').value = this.value;
    });

    // Robot control buttons
    document.getElementById('forwardBtn').addEventListener('click', () => sendCommandWithFeedback('forward', 'forwardBtn'));
    document.getElementById('backwardBtn').addEventListener('click', () => sendCommandWithFeedback('backward', 'backwardBtn'));
    document.getElementById('leftBtn').addEventListener('click', () => sendCommandWithFeedback('left', 'leftBtn'));
    document.getElementById('rightBtn').addEventListener('click', () => sendCommandWithFeedback('right', 'rightBtn'));
    document.getElementById('stopBtn').addEventListener('click', () => sendCommandWithFeedback('stop', 'stopBtn'));

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

// Send robot control command with visual feedback
function sendCommandWithFeedback(command, buttonId) {
    const button = document.getElementById(buttonId);
    const originalText = button.textContent;
    
    // Visual feedback
    button.textContent = 'Sending...';
    button.disabled = true;
    button.classList.add('btn-warning');
    
    sendCommand(command).finally(() => {
        // Reset button after short delay
        setTimeout(() => {
            button.textContent = originalText;
            button.disabled = false;
            button.classList.remove('btn-warning');
        }, 200);
    });
}

// Send robot control command
function sendCommand(command, value = null) {
    if (webSocket && webSocket.readyState === WebSocket.OPEN) {
        const data = { command };
        if (value !== null) data.value = value;

        const message = JSON.stringify(data);
        console.log('Sending WebSocket command:', message);
        webSocket.send(message);

        return Promise.resolve({ success: true });
    } else {
        console.error('WebSocket not connected');
        return Promise.reject(new Error('WebSocket not connected'));
    }
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
