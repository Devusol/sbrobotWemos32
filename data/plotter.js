// plotter.js - Real-time angle plotting functionality

let angleChart;
let angleData = {
    labels: [],
    current: [],
    target: []
};

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
        minAngle: 45,
        maxAngle: 135,

        // Draw the chart
        draw: function () {
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

        drawGrid: function () {
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
            const numAngleLines = 10;
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

        drawLine: function (dataPoints, color, label) {
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

        drawLegend: function () {
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

        update: function () {
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
