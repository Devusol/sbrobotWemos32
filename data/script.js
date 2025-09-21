// script.js - JavaScript for Robot Control Interface with WebSocket

let statusInterval;
let currentPID = { kp: 5.0, ki: 0.0, kd: 0.5 };
let ws;
let consoleElement;

// Initialize the page
window.onload = function() {
    loadPIDValues();
    setupEventListeners();
    statusInterval = setInterval(updateStatus, 5000); // Update status every 5 seconds
    initWebSocket();
    initAngleChart();
};

// Initialize WebSocket connection
function initWebSocket() {
    consoleElement = document.getElementById('serialConsole');
    ws = new WebSocket('ws://' + window.location.host + '/ws');

    ws.onopen = function(event) {
        console.log('WebSocket connected');
        consoleElement.textContent = 'WebSocket connected. Waiting for serial data...\n';
        // Request current buffer
        ws.send('get-buffer');
    };

    ws.onmessage = function(event) {
        const data = event.data;
        try {
            const jsonData = JSON.parse(data);
            if (jsonData.type === 'angle') {
                updateAnglePlot(jsonData.current, jsonData.target);
            }
        } catch (e) {
            // Not JSON, treat as serial data
            if (consoleElement) {
                consoleElement.textContent += data;
                consoleElement.scrollTop = consoleElement.scrollHeight;
            }
        }
    };

    ws.onclose = function(event) {
        console.log('WebSocket disconnected');
        if (consoleElement) {
            consoleElement.textContent += '\nWebSocket disconnected. Reconnecting...\n';
        }
        setTimeout(initWebSocket, 5000);
    };

    ws.onerror = function(error) {
        console.error('WebSocket error:', error);
    };
}

// Load current PID values from the robot
function loadPIDValues() {
    fetch('/get-pid')
        .then(response => response.json())
        .then(data => {
            currentPID.kp = data.kp;
            currentPID.ki = data.ki;
            currentPID.kd = data.kd;
            updatePIDDisplays();
        })
        .catch(error => console.error('Error loading PID values:', error));
}

// Update PID value displays
function updatePIDDisplays() {
    document.getElementById('kpValue').textContent = currentPID.kp.toFixed(3);
    document.getElementById('kiValue').textContent = currentPID.ki.toFixed(3);
    document.getElementById('kdValue').textContent = currentPID.kd.toFixed(3);
}

// Adjust PID values with buttons
function adjustPID(param, delta) {
    // Apply limits
    const limits = {
        kp: { min: 0, max: 20 },
        ki: { min: 0, max: 10 },
        kd: { min: 0, max: 5 }
    };

    currentPID[param] += delta;
    currentPID[param] = Math.max(limits[param].min, Math.min(limits[param].max, currentPID[param]));

    // Round to appropriate precision
    const precision = param === 'kp' ? 1 : 2;
    currentPID[param] = Math.round(currentPID[param] * Math.pow(10, precision)) / Math.pow(10, precision);

    updatePIDDisplays();
}

// Setup event listeners
function setupEventListeners() {
    // PID buttons
    document.getElementById('updatePID').addEventListener('click', updatePID);
    document.getElementById('calibrate').addEventListener('click', calibrateSensors);
    document.getElementById('resetPID').addEventListener('click', resetPID);

    // Robot control buttons
    document.getElementById('forward').addEventListener('click', () => sendCommand('forward'));
    document.getElementById('backward').addEventListener('click', () => sendCommand('backward'));
    document.getElementById('left').addEventListener('click', () => sendCommand('left'));
    document.getElementById('right').addEventListener('click', () => sendCommand('right'));
    document.getElementById('stop').addEventListener('click', () => sendCommand('stop'));
}

// Update PID values on the robot
function updatePID() {
    fetch('/set-pid', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `kp=${currentPID.kp}&ki=${currentPID.ki}&kd=${currentPID.kd}`
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            alert('PID values updated successfully!');
        } else {
            alert('Failed to update PID values: ' + data.message);
        }
    })
    .catch(error => {
        console.error('Error updating PID:', error);
        alert('Error updating PID values');
    });
}

// Calibrate sensors
function calibrateSensors() {
    if (confirm('Calibration will take about 10 seconds. Keep the robot stationary and level. Continue?')) {
        document.getElementById('calibrate').disabled = true;
        document.getElementById('calibrate').textContent = 'Calibrating...';

        fetch('/calibrate', {
            method: 'POST'
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                alert('Calibration completed successfully!');
            } else {
                alert('Calibration failed: ' + data.message);
            }
        })
        .catch(error => {
            console.error('Error calibrating:', error);
            alert('Error during calibration');
        })
        .finally(() => {
            document.getElementById('calibrate').disabled = false;
            document.getElementById('calibrate').textContent = 'Calibrate Sensors';
        });
    }
}

// Reset PID to default values
function resetPID() {
    currentPID.kp = 5.0;
    currentPID.ki = 0.0;
    currentPID.kd = 0.5;
    updatePIDDisplays();
    updatePID();
}

// Send robot control commands
function sendCommand(command) {
    fetch('/control', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `command=${command}&value=`
    })
    .catch(error => console.error('Error sending command:', error));
}

// Update status display
function updateStatus() {
    fetch('/status')
        .then(response => response.json())
        .then(data => {
            let statusHtml = "<strong>WiFi Status:</strong> ";
            if (data.mode === "AP") {
                statusHtml += `Access Point Mode - IP: ${data.ip}`;
            } else if (data.connected) {
                statusHtml += `Connected to ${data.ssid} - IP: ${data.ip}`;
            } else {
                statusHtml += "Not Connected";
            }
            document.getElementById('statusDisplay').innerHTML = statusHtml;
        })
        .catch(error => console.error('Error updating status:', error));
}

// Clear console
function clearConsole() {
    if (consoleElement) {
        consoleElement.textContent = '';
    }
    fetch('/clear-console')
        .then(response => response.json())
        .catch(error => console.error('Error clearing console:', error));
}

// Scroll console to bottom
function scrollToBottom() {
    if (consoleElement) {
        consoleElement.scrollTop = consoleElement.scrollHeight;
    }
}
