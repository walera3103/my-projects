/* ESP32 controller with Web Interface (AS5600 x5 + AccelStepper x5 + SD + WiFi) */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <AS5600.h>
#include <AccelStepper.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// ============= WiFi Settings =============
const char* ssid = "RobotArm";
const char* password = "12345678";
WebServer server(80);

// ============= Motor Control =============
#define NUM_MOTORS 5

// Motor pins
#define STEP_PIN_1 16
#define STEP_PIN_2 0
#define STEP_PIN_3 15
#define STEP_PIN_4 12
#define STEP_PIN_5 27

#define DIR_PIN_1 4
#define DIR_PIN_2 2
#define DIR_PIN_3 13
#define DIR_PIN_4 14
#define DIR_PIN_5 26

void PCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);
  Wire.write(1 << bus);
  Wire.endTransmission();
  delay(1);
}

AS5600 encoder;
AccelStepper stepper_1(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper stepper_2(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);
AccelStepper stepper_3(AccelStepper::DRIVER, STEP_PIN_3, DIR_PIN_3);
AccelStepper stepper_4(AccelStepper::DRIVER, STEP_PIN_4, DIR_PIN_4);
AccelStepper stepper_5(AccelStepper::DRIVER, STEP_PIN_5, DIR_PIN_5);
AccelStepper* steppers[NUM_MOTORS] = { &stepper_1, &stepper_2, &stepper_3, &stepper_4, &stepper_5 };

const float stepsPerRevolution = 200;
int microstepSetting = 1;
float desiredRPM = 300;
float MaxRPM = 300;
float speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
float Max_Speed_StepsPerSec = (microstepSetting * stepsPerRevolution * MaxRPM) / 60.0;

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
const int reducer_max_value_default = 40950;

int actual_encoder_points[NUM_MOTORS];
int previous_encoder_points[NUM_MOTORS];
long actual_reducer_points[NUM_MOTORS];
long previous_reducer_points[NUM_MOTORS];
int target_points[NUM_MOTORS];
bool motor_moving[NUM_MOTORS] = {false, false, false, false, false};
bool previous_motor_moving[NUM_MOTORS] = {false, false, false, false, false};
int motor_direction[NUM_MOTORS] = {0, 0, 0, 0, 0};

const int range = 100;
const int hysteresis_range = 800;
int delta;

long min_limit[NUM_MOTORS];
long max_limit[NUM_MOTORS];

unsigned long lastEncoderRead = 0;
unsigned long lastControlUpdate = 0;

String serialBuffer = "";
bool newData = false;

bool is_card_here = true;
bool calibration_mode = false;
bool request_log_write = false;

bool initial_encoder_read_done = false;

// ============= HTML Page =============
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Robot Arm Control</title>
  <style>
    /* Стили из вашего кода (оставить без изменений) */
    :root{
      --bg:#ffffff;
      --accent:#2563eb;
      --accent-700:#1d4ed8;
      --accent-100:#dbeafe;
      --text:#333333;
      --muted:#eaeaea;
      --ok:#11a36a;
      --err:#e03b3b;
      --shadow:0 6px 18px rgba(0,0,0,.06);
      --radius:16px;
    }
    html,body{height:100%;}
    body{
      margin:0;
      background:var(--bg);
      color:var(--text);
      font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Ubuntu, Cantarell, Noto Sans, "Helvetica Neue", Arial, "Apple Color Emoji", "Segoe UI Emoji";
    }
    .container{max-width:1200px; margin:24px auto; padding:0 16px;}
    .heading{display:flex; align-items:center; gap:12px; margin-bottom:16px}
    .heading .dot{width:12px; height:12px; border-radius:50%; background:var(--accent)}

    .grid{display:grid; gap:16px}
    @media (min-width: 900px){
      .grid.cols-2{grid-template-columns:1fr 1fr}
      .grid.cols-3{grid-template-columns:repeat(3,1fr)}
    }

    .card{
      background:#fff;
      border:1px solid var(--muted);
      border-left:6px solid var(--accent);
      border-radius:var(--radius);
      box-shadow:var(--shadow);
      padding:16px;
    }
    .card h3{margin:0 0 12px 0; font-size:1.1rem}
    .row{display:flex; flex-wrap:wrap; gap:12px; align-items:center}
    .row > *{flex:1}
    .row.tight > *{flex:0}
    .pill{display:inline-flex; align-items:center; gap:8px; padding:6px 10px; border-radius:999px; border:1px solid var(--muted); background:#fff}
    .pill .led{width:10px; height:10px; border-radius:50%}

    label{font-size:.9rem; color:#555}
    input[type="number"], input[type="text"], .readonly{
      width:100%; box-sizing:border-box; padding:10px 12px; border-radius:10px; border:1px solid var(--muted); background:#fff;
    }
    .readonly{background:#fafafa}

    .btn{
      appearance:none;
      border:none;
      cursor:pointer;
      padding:10px 14px;
      border-radius:12px;
      font-weight:600;
      transition:.15s transform, .15s box-shadow, .15s background;
      box-shadow:0 2px 0 rgba(0,0,0,.06);
    }
    .btn:active{transform:translateY(1px)}
    .btn.primary{background:var(--accent); color:#fff}
    .btn.primary:hover{background:var(--accent-700)}
    .btn.ghost{background:#fff; border:1px solid var(--accent); color:var(--accent)}
    .btn.ghost:hover{background:var(--accent-100)}
    .btn.link{background:transparent; color:var(--accent); padding:0}
    .btn.block{width:100%}
    .btn[disabled]{opacity:.55; cursor:not-allowed}

    .slider-wrap{display:grid; gap:8px}
    .range{
      -webkit-appearance:none;
      width:100%;
      height:10px;
      border-radius:999px;
      background:linear-gradient(90deg, var(--accent) var(--fill,50%), #ddd var(--fill,50%));
      outline:none;
      box-shadow:inset 0 1px 2px rgba(0,0,0,.1);
    }
    .range::-webkit-slider-thumb{
      -webkit-appearance:none;
      appearance:none;
      width:20px;
      height:20px;
      border-radius:50%;
      background:var(--accent);
      border:2px solid #fff;
      box-shadow:0 2px 6px rgba(0,0,0,.15);
    }
    .range::-moz-range-thumb{
      width:20px;
      height:20px;
      border-radius:50%;
      background:var(--accent);
      border:2px solid #fff;
    }
    .range[disabled]{pointer-events:none; opacity:0.8;}
    .minmax{display:flex; justify-content:space-between; color:#666; font-size:.85rem}

    .log{
      max-height:260px;
      overflow:auto;
      background:#fff;
      border:1px dashed var(--muted);
      border-radius:12px;
      padding:8px 12px;
      line-height:1.45;
      font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
    }
    .log .item{display:grid; grid-template-columns:auto 1fr; gap:8px; align-items:start; padding:6px 0; border-bottom:1px dashed #f0f0f0}
    .log .time{font-size:.8rem; color:#888}
    .log .msg strong{color:#111}

    .section-title{font-weight:800; letter-spacing:.2px; margin:8px 0 4px}
    .muted{color:#666}
    .sep{height:1px; background:linear-gradient(90deg, transparent, var(--muted), transparent); margin:8px 0 4px}
    .footnote{font-size:.85rem; color:#666}
  </style>
</head>
<body>
  <div class="container">
    <div class="heading">
      <div class="dot"></div>
      <h1 style="margin:0;font-size:1.6rem">Robot Arm Control Panel</h1>
    </div>

    <!-- System Status -->
    <section class="card" id="sysCard">
      <h3>System Status</h3>
      <div class="row">
        <div class="pill"><span class="led" id="sysLed" style="background: var(--err)"></span><span id="sysState">Inactive</span></div>
        <div class="pill"><span>🌡️</span><span>Temperature:</span><strong id="tempVal">-- °C</strong></div>
        <div class="pill"><span>⚡</span><span>Power:</span><strong id="powerVal">-- %</strong></div>
        <div class="row tight" style="gap:8px">
          <button class="btn ghost" id="btnToggleSys">Enable System</button>
          <button class="btn primary" id="btnRefresh">Refresh Status</button>
          <button class="btn ghost" id="btnCalibration">Calibration Mode</button>
        </div>
      </div>
    </section>

    <!-- Motor Control Sliders -->
    <section class="grid cols-2">
      <!-- Motor 1 -->
      <div class="card">
        <h3>Motor 1 (Base)</h3>
        <div class="slider-wrap">
          <label>Position (0-40950)</label>
          <input type="range" id="motor1-slider" class="range" min="0" max="40950" step="10" value="0">
          <div class="minmax"><span id="motor1-min">0</span><span id="motor1-val">0</span><span id="motor1-max">40950</span></div>
        </div>
        <div class="grid cols-2" style="margin-top:10px">
          <div><label>Encoder:</label><div id="enc1" class="readonly">0</div></div>
          <div><label>Target:</label><input id="target1" type="number" min="0" max="40950" value="0"></div>
        </div>
        <div class="row" style="margin-top:10px">
          <button class="btn ghost" onclick="setMin(1)">Set MIN</button>
          <button class="btn ghost" onclick="setMax(1)">Set MAX</button>
          <button class="btn primary" onclick="moveMotor(1)">Move</button>
        </div>
      </div>

      <!-- Motor 2 -->
      <div class="card">
        <h3>Motor 2 (Shoulder)</h3>
        <div class="slider-wrap">
          <label>Position (0-40950)</label>
          <input type="range" id="motor2-slider" class="range" min="0" max="40950" step="10" value="0">
          <div class="minmax"><span id="motor2-min">0</span><span id="motor2-val">0</span><span id="motor2-max">40950</span></div>
        </div>
        <div class="grid cols-2" style="margin-top:10px">
          <div><label>Encoder:</label><div id="enc2" class="readonly">0</div></div>
          <div><label>Target:</label><input id="target2" type="number" min="0" max="40950" value="0"></div>
        </div>
        <div class="row" style="margin-top:10px">
          <button class="btn ghost" onclick="setMin(2)">Set MIN</button>
          <button class="btn ghost" onclick="setMax(2)">Set MAX</button>
          <button class="btn primary" onclick="moveMotor(2)">Move</button>
        </div>
      </div>

      <!-- Motor 3 -->
      <div class="card">
        <h3>Motor 3 (Elbow)</h3>
        <div class="slider-wrap">
          <label>Position (0-40950)</label>
          <input type="range" id="motor3-slider" class="range" min="0" max="40950" step="10" value="0">
          <div class="minmax"><span id="motor3-min">0</span><span id="motor3-val">0</span><span id="motor3-max">40950</span></div>
        </div>
        <div class="grid cols-2" style="margin-top:10px">
          <div><label>Encoder:</label><div id="enc3" class="readonly">0</div></div>
          <div><label>Target:</label><input id="target3" type="number" min="0" max="40950" value="0"></div>
        </div>
        <div class="row" style="margin-top:10px">
          <button class="btn ghost" onclick="setMin(3)">Set MIN</button>
          <button class="btn ghost" onclick="setMax(3)">Set MAX</button>
          <button class="btn primary" onclick="moveMotor(3)">Move</button>
        </div>
      </div>

      <!-- Motor 4 -->
      <div class="card">
        <h3>Motor 4 (Wrist)</h3>
        <div class="slider-wrap">
          <label>Position (0-40950)</label>
          <input type="range" id="motor4-slider" class="range" min="0" max="40950" step="10" value="0">
          <div class="minmax"><span id="motor4-min">0</span><span id="motor4-val">0</span><span id="motor4-max">40950</span></div>
        </div>
        <div class="grid cols-2" style="margin-top:10px">
          <div><label>Encoder:</label><div id="enc4" class="readonly">0</div></div>
          <div><label>Target:</label><input id="target4" type="number" min="0" max="40950" value="0"></div>
        </div>
        <div class="row" style="margin-top:10px">
          <button class="btn ghost" onclick="setMin(4)">Set MIN</button>
          <button class="btn ghost" onclick="setMax(4)">Set MAX</button>
          <button class="btn primary" onclick="moveMotor(4)">Move</button>
        </div>
      </div>

      <!-- Motor 5 -->
      <div class="card">
        <h3>Motor 5 (Gripper)</h3>
        <div class="slider-wrap">
          <label>Position (0-40950)</label>
          <input type="range" id="motor5-slider" class="range" min="0" max="40950" step="10" value="0">
          <div class="minmax"><span id="motor5-min">0</span><span id="motor5-val">0</span><span id="motor5-max">40950</span></div>
        </div>
        <div class="grid cols-2" style="margin-top:10px">
          <div><label>Encoder:</label><div id="enc5" class="readonly">0</div></div>
          <div><label>Target:</label><input id="target5" type="number" min="0" max="40950" value="0"></div>
        </div>
        <div class="row" style="margin-top:10px">
          <button class="btn ghost" onclick="setMin(5)">Set MIN</button>
          <button class="btn ghost" onclick="setMax(5)">Set MAX</button>
          <button class="btn primary" onclick="moveMotor(5)">Move</button>
        </div>
      </div>
    </section>

    <!-- Control Panel -->
    <section class="grid cols-3">
      <div class="card">
        <h3>Quick Actions</h3>
        <div class="row"><button class="btn primary block" onclick="sendCommand('home')">↺ Home Position</button></div>
        <div class="row"><button class="btn ghost block" onclick="sendCommand('stop')">🛑 Emergency Stop</button></div>
        <div class="row"><button class="btn ghost block" onclick="calibrateAll()">Calibrate All Motors</button></div>
        <div class="row"><button class="btn ghost block" onclick="saveConfig()">💾 Save Configuration</button></div>
      </div>

      <div class="card">
        <h3>System Control</h3>
        <div class="row">
          <label for="speedInput">Speed (%):</label>
          <input type="range" id="speedInput" class="range" min="10" max="100" value="50">
          <span id="speedLabel">50%</span>
        </div>
        <div class="row"><button class="btn primary" onclick="updateSpeed()">Apply Speed</button></div>
        <div class="sep"></div>
        <div class="row"><button class="btn ghost" onclick="toggleCalibration()">Toggle Calibration</button></div>
      </div>

      <div class="card">
        <h3>Connection</h3>
        <div class="pill"><span class="led" id="wifiLed"></span><span id="wifiStatus">Disconnected</span></div>
        <div class="row"><label>IP:</label><div id="ipAddress" class="readonly">--</div></div>
        <div class="row"><label>Serial:</label><div id="serialStatus" class="readonly">Disconnected</div></div>
      </div>
    </section>

    <!-- Event Log -->
    <section class="card">
      <h3>Event Log</h3>
      <div class="row tight" style="justify-content:space-between; margin-bottom:8px">
        <span class="muted">System messages</span>
        <div class="row tight" style="gap:8px">
          <button class="btn ghost" id="btnLogPause">⏸️ Pause</button>
          <button class="btn ghost" id="btnLogClear">🧹 Clear</button>
        </div>
      </div>
      <div class="log" id="log"></div>
    </section>
  </div>

  <script>
    // System variables
    let sysOn = false;
    let logPaused = false;
    let calibrationMode = false;

    // DOM Elements
    const sysLed = document.getElementById('sysLed');
    const sysState = document.getElementById('sysState');
    const btnToggleSys = document.getElementById('btnToggleSys');
    const logBox = document.getElementById('log');
    const wifiLed = document.getElementById('wifiLed');
    const wifiStatus = document.getElementById('wifiStatus');

    // Log function
    function logEvent(msg) {
      if (logPaused) return;
      const row = document.createElement('div');
      row.className = 'item';
      const t = new Date();
      row.innerHTML = `<div class="time">${t.toLocaleTimeString()}</div><div class="msg">${msg}</div>`;
      logBox.prepend(row);
      if (logBox.children.length > 50) logBox.removeChild(logBox.lastChild);
    }

    // Toggle system
    function toggleSystem() {
      sysOn = !sysOn;
      sysLed.style.background = sysOn ? 'var(--ok)' : 'var(--err)';
      sysState.textContent = sysOn ? 'Active' : 'Inactive';
      btnToggleSys.textContent = sysOn ? 'Disable System' : 'Enable System';
      
      fetch('/cmd?system=' + (sysOn ? 'on' : 'off'))
        .then(r => r.text())
        .then(data => logEvent(`System ${sysOn ? 'activated' : 'deactivated'}: ${data}`));
    }

    // Move motor
    function moveMotor(num) {
      const slider = document.getElementById(`motor${num}-slider`);
      const target = document.getElementById(`target${num}`);
      const value = target.value || slider.value;
      
      fetch(`/cmd?motor=${num}&position=${value}`)
        .then(r => r.text())
        .then(data => {
          logEvent(`Motor ${num} → ${value}: ${data}`);
          updateMotorValue(num, value);
        });
    }

    // Set min limit
    function setMin(num) {
      const enc = document.getElementById(`enc${num}`);
      const minSpan = document.getElementById(`motor${num}-min`);
      fetch(`/cmd?calibration=set_min&motor=${num}&value=${enc.textContent}`)
        .then(r => r.text())
        .then(data => {
          logEvent(`Motor ${num} MIN set to ${enc.textContent}: ${data}`);
          minSpan.textContent = enc.textContent;
        });
    }

    // Set max limit
    function setMax(num) {
      const enc = document.getElementById(`enc${num}`);
      const maxSpan = document.getElementById(`motor${num}-max`);
      fetch(`/cmd?calibration=set_max&motor=${num}&value=${enc.textContent}`)
        .then(r => r.text())
        .then(data => {
          logEvent(`Motor ${num} MAX set to ${enc.textContent}: ${data}`);
          maxSpan.textContent = enc.textContent;
        });
    }

    // Update motor slider display
    function updateMotorValue(num, value) {
      const slider = document.getElementById(`motor${num}-slider`);
      const valSpan = document.getElementById(`motor${num}-val`);
      slider.value = value;
      valSpan.textContent = value;
      
      // Update slider gradient
      const min = parseInt(slider.min);
      const max = parseInt(slider.max);
      const pct = ((value - min) * 100 / (max - min)) + '%';
      slider.style.setProperty('--fill', pct);
    }

    // Send command
    function sendCommand(cmd) {
      fetch(`/cmd?command=${cmd}`)
        .then(r => r.text())
        .then(data => logEvent(`Command "${cmd}": ${data}`));
    }

    // Toggle calibration
    function toggleCalibration() {
      calibrationMode = !calibrationMode;
      fetch(`/cmd?calibration=${calibrationMode ? 'start' : 'stop'}`)
        .then(r => r.text())
        .then(data => {
          logEvent(`Calibration ${calibrationMode ? 'started' : 'stopped'}: ${data}`);
          document.getElementById('btnCalibration').textContent = 
            calibrationMode ? 'Exit Calibration' : 'Calibration Mode';
        });
    }

    // Calibrate all motors
    function calibrateAll() {
      if (confirm('Calibrate all motors? This will set min/max limits.')) {
        fetch('/cmd?command=calibrate_all')
          .then(r => r.text())
          .then(data => logEvent(`Calibrate all: ${data}`));
      }
    }

    // Save configuration
    function saveConfig() {
      fetch('/cmd?command=save')
        .then(r => r.text())
        .then(data => logEvent(`Configuration saved: ${data}`));
    }

    // Update speed
    function updateSpeed() {
      const speed = document.getElementById('speedInput').value;
      document.getElementById('speedLabel').textContent = speed + '%';
      fetch(`/cmd?speed=${speed}`)
        .then(r => r.text())
        .then(data => logEvent(`Speed set to ${speed}%: ${data}`));
    }

    // Initialize sliders
    function initSliders() {
      for (let i = 1; i <= 5; i++) {
        const slider = document.getElementById(`motor${i}-slider`);
        const target = document.getElementById(`target${i}`);
        
        slider.oninput = function() {
          const val = this.value;
          target.value = val;
          updateMotorValue(i, val);
        };
        
        target.oninput = function() {
          const val = this.value;
          slider.value = val;
          updateMotorValue(i, val);
        };
        
        updateMotorValue(i, slider.value);
      }
      
      // Speed slider
      const speedSlider = document.getElementById('speedInput');
      speedSlider.oninput = function() {
        document.getElementById('speedLabel').textContent = this.value + '%';
      };
    }

    // Update data from server
    function updateData() {
      fetch('/data')
        .then(r => r.json())
        .then(data => {
          if (data.status === 'ok') {
            // Update encoder values
            for (let i = 1; i <= 5; i++) {
              const enc = document.getElementById(`enc${i}`);
              if (data[`enc${i}`]) {
                enc.textContent = data[`enc${i}`];
              }
            }
            
            // Update limits
            for (let i = 1; i <= 5; i++) {
              const minSpan = document.getElementById(`motor${i}-min`);
              const maxSpan = document.getElementById(`motor${i}-max`);
              if (data[`min${i}`]) minSpan.textContent = data[`min${i}`];
              if (data[`max${i}`]) maxSpan.textContent = data[`max${i}`];
            }
            
            // Update system status
            wifiLed.style.background = data.wifi ? 'var(--ok)' : 'var(--err)';
            wifiStatus.textContent = data.wifi ? 'Connected' : 'Disconnected';
            document.getElementById('ipAddress').textContent = data.ip || '--';
            document.getElementById('serialStatus').textContent = data.serial ? 'Connected' : 'Disconnected';
          }
        })
        .catch(err => console.log('Update error:', err));
    }

    // Event listeners
    document.getElementById('btnToggleSys').onclick = toggleSystem;
    document.getElementById('btnLogClear').onclick = () => logBox.innerHTML = '';
    document.getElementById('btnLogPause').onclick = (e) => {
      logPaused = !logPaused;
      e.currentTarget.textContent = logPaused ? '▶️ Resume' : '⏸️ Pause';
    };
    document.getElementById('btnCalibration').onclick = toggleCalibration;
    document.getElementById('btnRefresh').onclick = updateData;

    // Initialize
    window.onload = function() {
      initSliders();
      updateData();
      setInterval(updateData, 1000); // Update every second
      logEvent('Robot Arm Control Panel loaded');
    };
  </script>
</body>
</html>
)rawliteral";

// ============= File Helpers =============
void writeFullLog() {
  if(!is_card_here) {
    Serial.println("No SD card - cannot write log");
    return;
  }

  File file = SD.open("/log.txt", FILE_WRITE);
  if(!file){
    Serial.println("Failed to open /log.txt for writing");
    return;
  }

  for (int i=0;i<NUM_MOTORS;i++){
    file.print("min limit for motor ");
    file.print(i+1);
    file.print(": ");
    file.println(min_limit[i]);

    file.print("max limit for motor ");
    file.print(i+1);
    file.print(": ");
    file.println(max_limit[i]);

    file.print("reducer position ");
    file.print(i+1);
    file.print(": ");
    file.println(actual_reducer_points[i]);
  }
  file.close();
  Serial.println("Log file overwritten (/log.txt).");
}

bool loadFromLog() {
  if(!is_card_here) return false;
  if(!SD.exists("/log.txt")) return false;

  File file = SD.open("/log.txt");
  if(!file) return false;

  Serial.println("=== READING LOG FILE ===");
  bool data_loaded = false;

  while(file.available()){
    String line = file.readStringUntil('\n');
    line.trim();
    if(line.length()==0) continue;

    if(line.startsWith("min limit for motor ")) {
      int idx = line.charAt(20) - '1';
      int colon = line.indexOf(':');
      if(idx>=0 && idx<NUM_MOTORS && colon>0){
        String val = line.substring(colon+1); 
        val.trim();
        min_limit[idx] = val.toInt();
        data_loaded = true;
      }
    } 
    else if(line.startsWith("max limit for motor ")) {
      int idx = line.charAt(20) - '1';
      int colon = line.indexOf(':');
      if(idx>=0 && idx<NUM_MOTORS && colon>0){
        String val = line.substring(colon+1); 
        val.trim();
        max_limit[idx] = val.toInt();
        data_loaded = true;
      }
    } 
    else if(line.startsWith("reducer position ")) {
      int idx = line.charAt(17) - '1';
      int colon = line.indexOf(':');
      if(idx>=0 && idx<NUM_MOTORS && colon>0){
        String val = line.substring(colon+1); 
        val.trim();
        actual_reducer_points[idx] = val.toInt();
        previous_reducer_points[idx] = actual_reducer_points[idx];
        target_points[idx] = actual_reducer_points[idx];
        data_loaded = true;
      }
    }
  }
  file.close();
  
  return data_loaded;
}

void initializeEncodersFromSavedPositions() {
  for (int i = 0; i < NUM_MOTORS; i++) {
    PCA9548A(i);
    int current_encoder_value = encoder.rawAngle();
    previous_encoder_points[i] = current_encoder_value;
  }
  initial_encoder_read_done = true;
}

// ============= Web Server Handlers =============
void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void handleCommand() {
  String response = "OK";
  
  if (server.hasArg("system")) {
    String cmd = server.arg("system");
    Serial.println("Web: System " + cmd);
    response = "System " + cmd;
  }
  
  if (server.hasArg("motor")) {
    int motor = server.arg("motor").toInt() - 1;
    if (motor >= 0 && motor < NUM_MOTORS) {
      if (server.hasArg("position")) {
        int position = server.arg("position").toInt();
        target_points[motor] = position;
        Serial.println("Web: Motor " + String(motor+1) + " → " + String(position));
        response = "Motor " + String(motor+1) + " set to " + String(position);
      }
    }
  }
  
  if (server.hasArg("calibration")) {
    String cmd = server.arg("calibration");
    if (cmd == "start") {
      calibration_mode = true;
      Serial.println("CALIBRATION_STARTED");
      response = "Calibration started";
    } else if (cmd == "stop") {
      calibration_mode = false;
      writeFullLog();
      Serial.println("CALIBRATION_STOPPED");
      response = "Calibration stopped and saved";
    } else if (cmd == "set_min") {
      int motor = server.arg("motor").toInt() - 1;
      int value = server.arg("value").toInt();
      if (motor >= 0 && motor < NUM_MOTORS) {
        min_limit[motor] = value;
        Serial.print("MIN_SET:");
        Serial.print(motor+1);
        Serial.print(":");
        Serial.println(min_limit[motor]);
        response = "Motor " + String(motor+1) + " MIN = " + String(value);
      }
    } else if (cmd == "set_max") {
      int motor = server.arg("motor").toInt() - 1;
      int value = server.arg("value").toInt();
      if (motor >= 0 && motor < NUM_MOTORS) {
        max_limit[motor] = value;
        Serial.print("MAX_SET:");
        Serial.print(motor+1);
        Serial.print(":");
        Serial.println(max_limit[motor]);
        response = "Motor " + String(motor+1) + " MAX = " + String(value);
      }
    }
  }
  
  if (server.hasArg("command")) {
    String cmd = server.arg("command");
    if (cmd == "home") {
      for (int i=0; i<NUM_MOTORS; i++) {
        target_points[i] = (min_limit[i] + max_limit[i]) / 2;
      }
      response = "Moving to home position";
    } else if (cmd == "stop") {
      for (int i=0; i<NUM_MOTORS; i++) {
        steppers[i]->setSpeed(0);
        motor_moving[i] = false;
      }
      response = "Emergency stop executed";
    } else if (cmd == "save") {
      writeFullLog();
      response = "Configuration saved";
    }
  }
  
  if (server.hasArg("speed")) {
    int speed = server.arg("speed").toInt();
    speedStepsPerSec = (microstepSetting * stepsPerRevolution * (desiredRPM * speed / 100.0)) / 60.0;
    response = "Speed set to " + String(speed) + "%";
  }
  
  server.send(200, "text/plain", response);
}

void handleData() {
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"wifi\":true,";
  json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"serial\":true,";
  
  for (int i=0; i<NUM_MOTORS; i++) {
    json += "\"enc" + String(i+1) + "\":\"" + String(actual_reducer_points[i]) + "\",";
    json += "\"min" + String(i+1) + "\":\"" + String(min_limit[i]) + "\",";
    json += "\"max" + String(i+1) + "\":\"" + String(max_limit[i]) + "\",";
    json += "\"target" + String(i+1) + "\":\"" + String(target_points[i]) + "\"";
    if (i < NUM_MOTORS-1) json += ",";
  }
  
  json += "}";
  server.send(200, "application/json", json);
}

// ============= Motor Control Functions =============
void emitStatusLines() {
  for (int i=0;i<NUM_MOTORS;i++){
    Serial.print("MOTOR_STATUS:");
    Serial.print(i+1);
    Serial.print(":");
    Serial.print(target_points[i]);
    Serial.print(":");
    Serial.println(actual_reducer_points[i]);
  }
}

void parseSerialData() {
  if (!newData) return;
  String cmd = serialBuffer;
  cmd.trim();

  if (cmd.length() == 0) { serialBuffer=""; newData=false; return; }

  if (cmd.startsWith("motor")) {
    int motorNumber = cmd.charAt(5) - '1';
    if (motorNumber >=0 && motorNumber < NUM_MOTORS) {
      int spaceIndex = cmd.indexOf(' ');
      if (spaceIndex != -1) {
        long newTarget = cmd.substring(spaceIndex+1).toInt();
        if (!calibration_mode) {
          if (newTarget < min_limit[motorNumber]) newTarget = min_limit[motorNumber];
          if (newTarget > max_limit[motorNumber]) newTarget = max_limit[motorNumber];
        }
        target_points[motorNumber] = (int)newTarget;
      }
    }
  }
  else if (cmd.equalsIgnoreCase("calibration start")) {
    calibration_mode = true;
    Serial.println("CALIBRATION_STARTED");
  }
  else if (cmd.equalsIgnoreCase("calibration stop")) {
    calibration_mode = false;
    Serial.println("CALIBRATION_STOPPED");
    writeFullLog();
  }
  else if (cmd.startsWith("calibration set_min")) {
    int sp = cmd.lastIndexOf(' ');
    if (sp>0) {
      int motorIndex = cmd.substring(sp+1).toInt() - 1;
      if (motorIndex >=0 && motorIndex < NUM_MOTORS) {
        min_limit[motorIndex] = actual_reducer_points[motorIndex];
        Serial.print("MIN_SET:");
        Serial.print(motorIndex+1);
        Serial.print(":");
        Serial.println(min_limit[motorIndex]);
      }
    }
  }
  else if (cmd.startsWith("calibration set_max")) {
    int sp = cmd.lastIndexOf(' ');
    if (sp>0) {
      int motorIndex = cmd.substring(sp+1).toInt() - 1;
      if (motorIndex >=0 && motorIndex < NUM_MOTORS) {
        max_limit[motorIndex] = actual_reducer_points[motorIndex];
        Serial.print("MAX_SET:");
        Serial.print(motorIndex+1);
        Serial.print(":");
        Serial.println(max_limit[motorIndex]);
      }
    }
  }
  else if (cmd.equalsIgnoreCase("GET_POS")) {
    emitStatusLines();
  }
  else if (cmd.equalsIgnoreCase("STOP")) {
    for (int i=0;i<NUM_MOTORS;i++){
      steppers[i]->setSpeed(0);
      motor_moving[i] = false;
    }
    Serial.println("EMERGENCY_STOP");
  }

  serialBuffer = "";
  newData = false;
}

bool anyMotorMoving() {
  for (int i=0;i<NUM_MOTORS;i++) if (motor_moving[i]) return true;
  return false;
}

// ============= Main Setup/Loop =============
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  encoder.begin();

  // Initialize steppers
  for (int i = 0; i < NUM_MOTORS; i++) {
    steppers[i]->setMaxSpeed(Max_Speed_StepsPerSec);
    steppers[i]->setSpeed(0);
  }

  // Initialize SD card
  if(!SD.begin(5)){
    Serial.println("SD_CARD_FAILED");
    is_card_here = false;
  } else {
    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
      Serial.println("NO_SD_CARD");
      is_card_here = false;
    } else {
      is_card_here = true;
      Serial.println("SD card OK");
    }
  }

  // Load configuration
  if (is_card_here) {
    loadFromLog();
  }

  initializeEncodersFromSavedPositions();

  // Start WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/cmd", handleCommand);
  server.on("/data", handleData);
  
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("=== SYSTEM READY ===");
  Serial.println("Connect to WiFi: " + String(ssid) + " Password: " + String(password));
  Serial.println("Open browser to: http://" + WiFi.softAPIP().toString());
}

void loop() {
  // Handle web clients
  server.handleClient();
  
  // Handle serial commands
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      newData = true;
      parseSerialData();
    } else {
      serialBuffer += c;
    }
  }

  unsigned long now = millis();

  // Read encoders
  if (now - lastEncoderRead >= 10) {
    for (int i = 0; i < NUM_MOTORS; i++) {
      PCA9548A(i);
      actual_encoder_points[i] = encoder.rawAngle();

      delta = actual_encoder_points[i] - previous_encoder_points[i];
      if (delta > 2048) delta -= 4096;
      else if (delta < -2048) delta += 4096;
      if (abs(delta) < 2) delta = 0;

      actual_reducer_points[i] += delta;

      if (!calibration_mode) {
        if (actual_reducer_points[i] > max_limit[i]) actual_reducer_points[i] = max_limit[i];
        if (actual_reducer_points[i] < min_limit[i]) actual_reducer_points[i] = min_limit[i];
      }
      previous_encoder_points[i] = actual_encoder_points[i];
    }
    lastEncoderRead = now;
  }

  // Control motors
  if (now - lastControlUpdate >= 20) {
    for (int i = 0; i < NUM_MOTORS; i++) {
      int error = target_points[i] - actual_reducer_points[i];
      int abs_error = abs(error);

      bool at_min_limit = (!calibration_mode) && (actual_reducer_points[i] <= min_limit[i] && error < 0);
      bool at_max_limit = (!calibration_mode) && (actual_reducer_points[i] >= max_limit[i] && error > 0);

      if (at_min_limit || at_max_limit) {
        steppers[i]->setSpeed(0);
        motor_moving[i] = false;
        motor_direction[i] = 0;
        continue;
      }

      if (motor_moving[i]) {
        if (abs_error <= range) {
          steppers[i]->setSpeed(0);
          motor_moving[i] = false;
          motor_direction[i] = 0;
          request_log_write = true;
        }
      } else {
        if (abs_error > hysteresis_range) {
          int new_direction = (error > 0) ? 1 : -1;
          float currentSpeed = (new_direction > 0) ? speedStepsPerSec : -speedStepsPerSec;
          steppers[i]->setSpeed(currentSpeed);
          motor_moving[i] = true;
          motor_direction[i] = new_direction;
        } else {
          steppers[i]->setSpeed(0);
          motor_moving[i] = false;
        }
      }
    }
    lastControlUpdate = now;
  }

  // Save log if needed
  bool anyMovingNow = anyMotorMoving();
  for (int i=0;i<NUM_MOTORS;i++){
    if (previous_motor_moving[i] && !motor_moving[i]) {
      request_log_write = true;
    }
    previous_motor_moving[i] = motor_moving[i];
  }

  if (request_log_write && !anyMovingNow) {
    writeFullLog();
    request_log_write = false;
  }

  // Run steppers
  for (int i = 0; i < NUM_MOTORS; i++) {
    steppers[i]->runSpeed();
  }

  // Debug output
  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 1000) {
    emitStatusLines(); // Send status to serial
    lastDebug = now;
  }
}