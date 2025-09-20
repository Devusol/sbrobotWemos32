// script.js - JavaScript for Robot Control Interface with WebSocket

let statusInterval;
let currentPID = { kp: 5.0, ki: 0.0, kd: 0.5 };
let ws;
let consoleElement;
let angleChart;
let angleData = {
    labels: [],
    current: [],
    target: []
};

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

// Initialize custom canvas chart for angle plotting
function initAngleChart() {
    const canvas = document.getElementById('angleChart');
    const ctx = canvas.getContext('2d');

    // Chart properties
    angleChart = {
        canvas: canvas,
        ctx: ctx,
        width: canvas.width,
        height: canvas.height,
        data: angleData,
        minAngle: 70,
        maxAngle: 110,

        // Draw the chart
        draw: function() {
            const ctx = this.ctx;
            const width = this.width;
            const height = this.height;

            // Clear canvas
            ctx.clearRect(0, 0, width, height);

            // Set background
            ctx.fillStyle = '#ffffff';
            ctx.fillRect(0, 0, width, height);

            // Draw grid and labels
            this.drawGrid();

            // Draw data lines
            this.drawLine(this.data.current, '#4bc0c0', 'Current Angle');
            this.drawLine(this.data.target, '#ff6384', 'Target Angle');

            // Draw legend
            this.drawLegend();
        },

        drawGrid: function() {
            const ctx = this.ctx;
            const width = this.width;
            const height = this.height;

            ctx.strokeStyle = '#e0e0e0';
            ctx.lineWidth = 1;

            // Vertical grid lines (time)
            const numTimeLines = 10;
            for (let i = 0; i <= numTimeLines; i++) {
                const x = (i / numTimeLines) * (width - 60) + 50;
                ctx.beginPath();
                ctx.moveTo(x, 20);
                ctx.lineTo(x, height - 30);
                ctx.stroke();

                // Time labels
                if (i < this.data.labels.length) {
                    ctx.fillStyle = '#666';
                    ctx.font = '10px Arial';
                    ctx.textAlign = 'center';
                    ctx.fillText(this.data.labels[i], x, height - 10);
                }
            }

            // Horizontal grid lines (angles)
            const numAngleLines = 5;
            for (let i = 0; i <= numAngleLines; i++) {
                const y = 20 + (i / numAngleLines) * (height - 50);
                const angle = this.maxAngle - (i / numAngleLines) * (this.maxAngle - this.minAngle);

                ctx.beginPath();
                ctx.moveTo(50, y);
                ctx.lineTo(width - 10, y);
                ctx.stroke();

                // Angle labels
                ctx.fillStyle = '#666';
                ctx.font = '10px Arial';
                ctx.textAlign = 'right';
                ctx.fillText(angle.toFixed(1) + '°', 45, y + 3);
            }

            // Axis labels
            ctx.fillStyle = '#333';
            ctx.font = '12px Arial';
            ctx.textAlign = 'center';
            ctx.fillText('Time', width / 2, height - 5);

            ctx.save();
            ctx.translate(15, height / 2);
            ctx.rotate(-Math.PI / 2);
            ctx.fillText('Angle (degrees)', 0, 0);
            ctx.restore();
        },

        drawLine: function(dataPoints, color, label) {
            const ctx = this.ctx;
            const width = this.width;
            const height = this.height;

            if (dataPoints.length < 2) return;

            ctx.strokeStyle = color;
            ctx.lineWidth = 2;
            ctx.beginPath();

            for (let i = 0; i < dataPoints.length; i++) {
                const x = 50 + (i / Math.max(dataPoints.length - 1, 1)) * (width - 60);
                const normalizedAngle = (dataPoints[i] - this.minAngle) / (this.maxAngle - this.minAngle);
                const y = height - 30 - normalizedAngle * (height - 50);

                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            }

            ctx.stroke();
        },

        drawLegend: function() {
            const ctx = this.ctx;
            const width = this.width;

            // Current angle
            ctx.strokeStyle = '#4bc0c0';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(width - 120, 15);
            ctx.lineTo(width - 80, 15);
            ctx.stroke();

            ctx.fillStyle = '#333';
            ctx.font = '12px Arial';
            ctx.textAlign = 'left';
            ctx.fillText('Current', width - 75, 20);

            // Target angle
            ctx.strokeStyle = '#ff6384';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(width - 120, 35);
            ctx.lineTo(width - 80, 35);
            ctx.stroke();

            ctx.fillText('Target', width - 75, 40);
        },

        update: function() {
            this.draw();
        }
    };

    // Initial draw
    angleChart.draw();
}

// Update angle plot with new data
function updateAnglePlot(currentAngle, targetAngle) {
    const now = new Date();
    const timeLabel = now.getHours() + ':' + now.getMinutes() + ':' + now.getSeconds();

    // Keep only last 50 data points
    if (angleData.labels.length > 50) {
        angleData.labels.shift();
        angleData.current.shift();
        angleData.target.shift();
    }

    angleData.labels.push(timeLabel);
    angleData.current.push(currentAngle);
    angleData.target.push(targetAngle);

    // Update chart
    if (angleChart && angleChart.update) {
        angleChart.update();
    }
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
