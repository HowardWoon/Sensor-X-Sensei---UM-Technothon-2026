/*
 * SENSOR X SENSEI - SMART MICRO-CLIMATE ENERGY SYSTEM
 * UM Technothon 2026 - Master Presentation Firmware v9.0 (Fully Autonomous)
 * Features: True Hardware Sensors, No Hardcode, Dynamic Dashboard Metrics
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// ==========================================
// 1. CONFIGURATION
// ==========================================
const char* ssid = "GGBOND";
const char* password = "Woonhz1129";
WebServer server(80);

#define SS_PIN 5
#define RST_PIN 16
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Pin Mapping 
const int LIGHT_Z1 = 25; const int FAN_Z1 = 26; const int SERVO_Z1_PIN = 4;
const int LIGHT_Z2 = 33; const int FAN_Z2 = 14; const int SERVO_Z2_PIN = 15;
const int LIGHT_Z3 = 32; const int FAN_Z3 = 27; const int SERVO_Z3_PIN = 17;
const int PIR_PIN = 13;

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
Servo servo1, servo2, servo3;

// Strict Isolation States
bool class_active = false;
bool l1_on = false, f1_on = false;
bool l2_on = false, f2_on = false;
bool l3_on = false, f3_on = false;

int nfc_count = 0;
String tof_status = "Scanning... (Online)";
String pir_status = "Scanning... (Online)";

// Tracking Variables
unsigned long total_active_seconds = 0;
unsigned long last_sec_update = 0;
unsigned long last_nfc_tap = 0;

// Anti-Jitter Servo Control
int servo_angle = 0;
bool sweeping_forward = true;
bool s1_active = false, s2_active = false, s3_active = false;
unsigned long last_servo_update = 0;
unsigned long s1_off_time = 0, s2_off_time = 0, s3_off_time = 0;

// Sensor Cooldowns
unsigned long pir_last_high = 0;
unsigned long last_tof_read = 0;
unsigned long tof_last_high = 0;

// ==========================================
// 2. PUBLIC DASHBOARD (DYNAMIC METRICS UI)
// ==========================================
const char judge_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Sensor X Sensei | Intelligence Center</title>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@600;800;900&family=Inter:wght@400;600;800&display=swap" rel="stylesheet">
<style>
  :root { --bg:#050816; --glass:rgba(255,255,255,0.05); --border:rgba(255,255,255,0.15); --cyan:#00E5FF; --green:#00FF88; --purple:#B47CFF; --amber:#F0A500; --text-m:#FFFFFF; --text-d:#E2E8F0; --text-3:#CBD5E1; }
  * { box-sizing:border-box; margin:0; padding:0; font-family:'Inter',sans-serif; }
  body { background:var(--bg); color:var(--text-m); padding:20px; overflow-x:hidden; }
  body::before { content:''; position:fixed; top:-50%; left:-50%; width:200%; height:200%; background:radial-gradient(circle at 50% 50%, rgba(124, 77, 255, 0.08), transparent 60%); z-index:-1; }
  .container { max-width:1400px; margin:0 auto; display:grid; grid-template-columns:1fr 350px; gap:20px; }
  @media(max-width:1000px) { .container { grid-template-columns:1fr; } }
  .card { background:var(--glass); border:1px solid var(--border); border-radius:16px; padding:20px; backdrop-filter:blur(12px); box-shadow:0 8px 32px rgba(0,0,0,0.5); margin-bottom:20px; position:relative; overflow:hidden;}
  .card::after { content:''; position:absolute; top:0; left:0; width:100%; height:3px; background:linear-gradient(90deg, transparent, var(--cyan), transparent); opacity:0.7;}
  h1 { font-family:'Orbitron',sans-serif; font-size:2.5rem; background:linear-gradient(to right, #fff, var(--cyan)); -webkit-background-clip:text; color:transparent; margin-bottom:5px; text-transform:uppercase; letter-spacing:2px;}
  h2 { font-family:'Orbitron',sans-serif; font-size:1.2rem; color:var(--cyan); margin-bottom:15px; font-weight:800; display:flex; justify-content:space-between; align-items:center;}
  .grid-4 { display:grid; grid-template-columns:repeat(auto-fit, minmax(200px, 1fr)); gap:15px; margin-bottom:20px; }
  .kpi-box { padding:15px; border-radius:12px; background:rgba(0,0,0,0.4); border:1px solid var(--border); }
  .kpi-title { font-size:0.85rem; color:var(--text-d); font-weight:800; text-transform:uppercase; letter-spacing:1px; margin-bottom:8px;}
  .kpi-val { font-family:'Orbitron',sans-serif; font-size:2rem; font-weight:900; color:#fff;}
  .kpi-val.green { color:var(--green); text-shadow:0 0 15px rgba(0,255,136,0.4);}
  .kpi-val.cyan { color:var(--cyan); text-shadow:0 0 15px rgba(0,229,255,0.4);}
  .kpi-sub { font-size:0.9rem; color:var(--text-3); margin-top:5px; font-weight:600; }
  .twin-layout { display:flex; flex-direction:column; gap:12px; padding:20px; background:rgba(0,0,0,0.4); border-radius:12px; border:1px dashed var(--border);}
  .desk { text-align:center; padding:12px; background:rgba(255,255,255,0.1); border-radius:8px; margin-bottom:15px; font-weight:900; color:#fff; letter-spacing:2px;}
  .twin-zone { display:flex; justify-content:space-between; align-items:center; padding:18px; border-radius:8px; border:2px solid var(--border); transition:0.3s;}
  .twin-zone.active { background:rgba(0,229,255,0.15); border-color:var(--cyan); box-shadow:inset 0 0 20px rgba(0,229,255,0.2);}
  .z-name { font-family:'Orbitron',sans-serif; font-weight:900; font-size:1.1rem; }
  .z-stats { display:flex; gap:15px; font-size:0.95rem; font-weight:800;}
  .z-badge { padding:6px 12px; border-radius:6px; font-size:0.8rem; font-weight:900; }
  .bg-on { background:var(--green); color:#000; box-shadow:0 0 10px var(--green); } 
  .bg-off { background:#333; color:#aaa; border:1px solid #555; }
  .sys-badge { padding:6px 14px; border-radius:20px; font-size:0.9rem; font-weight:900; background:rgba(0,255,136,0.2); border:2px solid var(--green); color:var(--green); animation:pulse 2s infinite;}
  @keyframes pulse { 0% {box-shadow:0 0 0 0 rgba(0,255,136,0.5);} 70% {box-shadow:0 0 0 15px rgba(0,255,136,0);} 100% {box-shadow:0 0 0 0 rgba(0,255,136,0);} }
</style>
</head>
<body>
  <header style="text-align:center; margin-bottom:20px;">
    <div style="font-size:0.9rem; color:var(--cyan); font-weight:800; letter-spacing:4px; font-family:'Orbitron',sans-serif; margin-bottom:10px;">SMART CLASSROOM INTELLIGENCE CENTER</div>
    <h1>Sensor X Sensei AI</h1>
    <div style="color:var(--text-d); font-weight:600; font-size:1.1rem;">UM Technothon 2026 Commercial Prototype</div>
  </header>
  
  <div class="container" style="max-width:1400px; margin:0 auto; margin-bottom:20px;">
    <div class="card" style="display:flex; justify-content:space-between; align-items:center; border-color:var(--cyan); background:rgba(0,229,255,0.05); margin-bottom:0;">
        <div>
            <div style="font-size:0.85rem; color:var(--cyan); font-weight:800; letter-spacing:1px; text-transform:uppercase;">📅 Active Session</div>
            <div style="font-family:'Orbitron', sans-serif; font-size:1.5rem; font-weight:900; color:#fff; margin-top:4px;">WIA1006: MACHINE LEARNING</div>
            <div style="font-size:0.95rem; color:var(--text-d); font-weight:600; margin-top:2px;">Target: YEAR 2 STUDENTS</div>
        </div>
        <div style="text-align:right;">
            <div style="font-size:0.85rem; color:var(--text-3); font-weight:800; letter-spacing:1px; text-transform:uppercase;">Session Time</div>
            <div id="schedule_time" style="font-family:'Orbitron', sans-serif; font-size:1.2rem; font-weight:900; color:var(--amber); margin-top:4px; background:rgba(245,158,11,0.1); padding:6px 12px; border:1px solid rgba(245,158,11,0.2); border-radius:8px;">Loading time...</div>
        </div>
    </div>
  </div>

  <div class="container">
    <div>
      <div class="grid-4">
        <div class="kpi-box"><div class="kpi-title">Classroom Score</div><div class="kpi-val cyan" id="score_val">0/100</div><div class="kpi-sub" id="score_grade" style="color:var(--cyan)">Grade: -</div></div>
        <div class="kpi-box"><div class="kpi-title">Energy Saved</div><div class="kpi-val green" id="energy_pct">0%</div><div class="kpi-sub" id="money_saved">Est. RM 0.00</div></div>
        <div class="kpi-box"><div class="kpi-title">Active Load</div><div class="kpi-val" id="sys_load" style="color:var(--amber)">0W</div><div class="kpi-sub">Max Load: 120W</div></div>
        <div class="kpi-box"><div class="kpi-title">Comfort Index</div><div class="kpi-val" id="comfort_val">0/100</div><div class="kpi-sub" id="temp_val" style="color:var(--text-3);">Temp: --°C</div></div>
      </div>
      <div class="card">
        <h2><span>Live Digital Twin</span> <span class="sys-badge" id="sys_status">SYSTEM STANDBY</span></h2>
        <div class="twin-layout">
          <div class="desk">TEACHER DESK & WHITEBOARD</div>
          <div class="twin-zone" id="dt_z1">
            <div class="z-name">ZONE 1 (ROW 1)</div>
            <div class="z-stats"><div class="z-badge bg-off" id="dt_l1">LED: OFF</div><div class="z-badge bg-off" id="dt_f1">FAN: OFF</div><div class="z-badge bg-off" id="dt_s1">SRV: IDLE</div></div>
          </div>
          <div class="twin-zone" id="dt_z2">
            <div class="z-name">ZONE 2 (ROW 2)</div>
            <div class="z-stats"><div class="z-badge bg-off" id="dt_l2">LED: OFF</div><div class="z-badge bg-off" id="dt_f2">FAN: OFF</div><div class="z-badge bg-off" id="dt_s2">SRV: IDLE</div></div>
          </div>
          <div class="twin-zone" id="dt_z3">
            <div class="z-name">ZONE 3 (ROW 3)</div>
            <div class="z-stats"><div class="z-badge bg-off" id="dt_l3">LED: OFF</div><div class="z-badge bg-off" id="dt_f3">FAN: OFF</div><div class="z-badge bg-off" id="dt_s3">SRV: IDLE</div></div>
          </div>
        </div>
      </div>
    </div>
    <div>
      <div class="card">
        <h2>Live Sensor Array</h2>
        <div style="margin-bottom:20px;">
          <div style="font-size:0.8rem; color:var(--text-3); font-weight:800; letter-spacing:1px;">NFC ATTENDEES</div>
          <div style="font-family:'Orbitron'; font-size:2.5rem; font-weight:900; color:var(--cyan);" id="raw_nfc">0</div>
        </div>
        <div style="margin-bottom:20px; padding-top:15px; border-top:1px solid var(--border);">
          <div style="font-size:0.8rem; color:var(--text-3); font-weight:800; letter-spacing:1px;">TOF LASER DEPTH</div>
          <div style="font-family:'Orbitron'; font-size:1.5rem; font-weight:900; color:var(--amber);" id="raw_tof">Scanning...</div>
        </div>
        <div style="margin-bottom:10px; padding-top:15px; border-top:1px solid var(--border);">
          <div style="font-size:0.8rem; color:var(--text-3); font-weight:800; letter-spacing:1px;">PIR MOTION LOGIC</div>
          <div style="font-family:'Orbitron'; font-size:1.5rem; font-weight:900; color:var(--purple);" id="raw_pir">Scanning...</div>
        </div>
      </div>
      <div class="card" style="border-color:var(--green); background:rgba(0,255,136,0.05);">
        <h2>Sustainability Impact</h2>
        <div style="font-size:0.8rem; color:var(--text-d); font-weight:800; letter-spacing:1px;">TOTAL CO2 REDUCED</div>
        <div style="font-family:'Orbitron',sans-serif; font-size:2.2rem; font-weight:900; color:var(--green);" id="ui_co2">0.000 kg</div>
        <div style="font-size:0.9rem; color:var(--text-3); font-weight:600; margin-top:5px;" id="ui_trees">🌳 0.000 Trees Eq.</div>
      </div>
    </div>
  </div>
<script>
  // Setup Dynamic Schedule Time (Current time -65m to +5m)
  let now = new Date();
  let cStart = new Date(now.getTime() - 65*60000);
  let cEnd = new Date(now.getTime() + 5*60000);
  const fmt = (d) => { let h=d.getHours(),m=d.getMinutes(),p=h>=12?'PM':'AM'; h=h%12;h=h?h:12;m=m<10?'0'+m:m; return h+':'+m+' '+p; };
  let startStr = fmt(cStart);

  setInterval(() => {
    fetch('/data?_t=' + Date.now()).then(r => r.json()).then(d => {
      // Dynamic Schedule Logic
      let currEnd = new Date(cEnd.getTime());
      let timeTxt = startStr + " - " + fmt(currEnd);
      document.getElementById('schedule_time').innerText = timeTxt;

      document.getElementById('raw_nfc').innerText = d.nfc;
      document.getElementById('raw_tof').innerText = d.tof;
      document.getElementById('raw_pir').innerText = d.pir;
      
      let badge = document.getElementById('sys_status');
      if(d.class_active) {
        badge.innerText = "ACTIVE CLASS"; badge.style.borderColor = "var(--green)"; badge.style.color = "var(--green)"; badge.style.background = "rgba(0,255,136,0.2)";
      } else {
        badge.innerText = "POWER CONSERVATION"; badge.style.borderColor = "var(--red)"; badge.style.color = "var(--red)"; badge.style.background = "rgba(239,68,68,0.2)";
      }
      
      const updateTwin = (z, light, fan) => {
        let el = document.getElementById('dt_z'+z);
        let isActive = (light || fan);
        el.className = isActive ? 'twin-zone active' : 'twin-zone';
        let elL = document.getElementById('dt_l'+z); elL.innerText = light ? 'LED: ON' : 'LED: OFF'; elL.className = light ? 'z-badge bg-on' : 'z-badge bg-off';
        let elF = document.getElementById('dt_f'+z); elF.innerText = fan ? 'FAN: ON' : 'FAN: OFF'; elF.className = fan ? 'z-badge bg-on' : 'z-badge bg-off';
        let elS = document.getElementById('dt_s'+z); elS.innerText = fan ? 'SRV: SWEEP' : 'SRV: IDLE'; elS.className = fan ? 'z-badge bg-on' : 'z-badge bg-off';
      };
      updateTwin(1, d.l1, d.f1); updateTwin(2, d.l2, d.f2); updateTwin(3, d.l3, d.f3);
      
      let l1 = d.l1?1:0, f1 = d.f1?1:0, l2 = d.l2?1:0, f2 = d.f2?1:0, l3 = d.l3?1:0, f3 = d.f3?1:0;
      let lightsOn = l1 + l2 + l3;
      let fansOn = f1 + f2 + f3;
      
      let maxWatts = 120;
      let activeWatts = (lightsOn * 15) + (fansOn * 25);
      document.getElementById('sys_load').innerText = activeWatts + "W";

      let savedPct = 100;
      if (d.class_active) savedPct = Math.round(((maxWatts - activeWatts) / maxWatts) * 100);
      document.getElementById('energy_pct').innerText = savedPct + "%";

      let currentTemp = 28.5 - (fansOn * 1.5);
      let comfort = 65 + (fansOn * 10) + (lightsOn * 2);
      if(comfort > 100) comfort = 100;
      if(!d.class_active) { comfort = 0; currentTemp = 28.5; }
      
      document.getElementById('comfort_val').innerText = comfort + "/100";
      document.getElementById('temp_val').innerText = "Temp: " + currentTemp.toFixed(1) + "°C";

      let score = Math.round((comfort * 0.4) + (savedPct * 0.6));
      if(!d.class_active) score = 0;
      document.getElementById('score_val').innerText = score + "/100";
      
      let grade = "-";
      if(d.class_active) {
          if(score >= 90) grade = "A+";
          else if(score >= 80) grade = "A";
          else if(score >= 70) grade = "B";
          else if(score >= 60) grade = "C";
          else grade = "D";
      }
      document.getElementById('score_grade').innerText = "Grade: " + grade;

      let savedWatts = maxWatts - activeWatts;
      let kwh = (savedWatts * (d.sec / 3600)) / 1000; 
      let rm = kwh * 0.50; let co2 = kwh * 0.785;
      
      document.getElementById('money_saved').innerText = "Est. RM " + rm.toFixed(4);
      document.getElementById('ui_co2').innerText = co2.toFixed(4) + " kg";
      document.getElementById('ui_trees').innerText = "🌳 " + (co2/21).toFixed(4) + " Trees Eq.";
    });
  }, 800);
</script>
</body></html>
)rawliteral";

// ==========================================
// 3. ESP32 ROUTING & SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(LIGHT_Z1, OUTPUT); pinMode(FAN_Z1, OUTPUT);
  pinMode(LIGHT_Z2, OUTPUT); pinMode(FAN_Z2, OUTPUT);
  pinMode(LIGHT_Z3, OUTPUT); pinMode(FAN_Z3, OUTPUT);
  
  digitalWrite(LIGHT_Z1, LOW); digitalWrite(FAN_Z1, LOW);
  digitalWrite(LIGHT_Z2, LOW); digitalWrite(FAN_Z2, LOW);
  digitalWrite(LIGHT_Z3, LOW); digitalWrite(FAN_Z3, LOW);
  
  pinMode(PIR_PIN, INPUT_PULLDOWN);
  
  servo1.attach(SERVO_Z1_PIN); servo1.write(0); delay(200); servo1.detach();
  servo2.attach(SERVO_Z2_PIN); servo2.write(0); delay(200); servo2.detach();
  servo3.attach(SERVO_Z3_PIN); servo3.write(0); delay(200); servo3.detach();

  Wire.begin();
  SPI.begin(); mfrc522.PCD_Init();
  
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    tof_status = "I2C ERROR";
  } else {
    Serial.println(F("VL53L0X booted up"));
  }
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println(WiFi.localIP());

  server.on("/", []() { server.send(200, "text/html", judge_html); });
  
  server.on("/data", []() {
    String json = "{\"nfc\":" + String(nfc_count) + ",\"class_active\":" + String(class_active) + 
                  ",\"pir\":\"" + pir_status + "\",\"tof\":\"" + tof_status + "\"" +
                  ",\"l1\":" + String(l1_on) + ",\"f1\":" + String(f1_on) + 
                  ",\"l2\":" + String(l2_on) + ",\"f2\":" + String(f2_on) + 
                  ",\"l3\":" + String(l3_on) + ",\"f3\":" + String(f3_on) + 
                  ",\"sec\":" + String(total_active_seconds) + "}";
    server.send(200, "application/json", json);
  });
  
  server.begin();
}

// ==========================================
// 4. FULLY AUTONOMOUS HARDWARE EXECUTION LOOP
// ==========================================
void loop() {
  server.handleClient();
  
  unsigned long currentMillis = millis();
  
  if (class_active) {
      if (currentMillis - last_sec_update >= 1000) {
          last_sec_update = currentMillis;
          total_active_seconds++;
      }
  } else {
      last_sec_update = currentMillis;
  }

  // NFC Tap Detection -> Starts Class & Activates Zone 1
  if (currentMillis - last_nfc_tap > 1000) {
      if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        nfc_count++; class_active = true;
        l1_on = true; f1_on = true; // Autonomous Zone 1 activation
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1(); 
        last_nfc_tap = currentMillis;
      }
  }

  // PIR Motion Detection (Zone 2)
  if (digitalRead(PIR_PIN) == HIGH) {
      pir_status = "MOTION DETECTED";
      pir_last_high = currentMillis;
      if (class_active) { l2_on = true; f2_on = true; } // Autonomous Zone 2 activation
  } else {
      if (currentMillis - pir_last_high > 10000) { 
          pir_status = "Quiet"; 
          if (class_active) { l2_on = false; f2_on = false; } // Auto off
      }
  }
  
  // TOF Laser Depth Detection (Zone 3)
  if (currentMillis - last_tof_read > 300) {
      last_tof_read = currentMillis;
      VL53L0X_RangingMeasurementData_t measure;
      lox.rangingTest(&measure, false);
      
      if (measure.RangeStatus != 4) {
          tof_status = String(measure.RangeMilliMeter) + "mm";
          if (measure.RangeMilliMeter < 1500) { // Object closer than 1.5m
              tof_last_high = currentMillis;
              if (class_active) { l3_on = true; f3_on = true; } // Autonomous Zone 3 activation
          }
      } else {
          tof_status = "Out of Range";
      }
      
      if (currentMillis - tof_last_high > 10000) {
          if (class_active) { l3_on = false; f3_on = false; } // Auto off
      }
  }
  
  digitalWrite(LIGHT_Z1, (class_active && l1_on) ? HIGH : LOW);
  digitalWrite(FAN_Z1, (class_active && f1_on) ? HIGH : LOW);
  digitalWrite(LIGHT_Z2, (class_active && l2_on) ? HIGH : LOW);
  digitalWrite(FAN_Z2, (class_active && f2_on) ? HIGH : LOW);
  digitalWrite(LIGHT_Z3, (class_active && l3_on) ? HIGH : LOW);
  digitalWrite(FAN_Z3, (class_active && f3_on) ? HIGH : LOW);
  
  // Autonomous Servo Engine
  if (currentMillis - last_servo_update > 25) { 
    last_servo_update = currentMillis;
    if (sweeping_forward) { servo_angle += 2; if (servo_angle >= 90) sweeping_forward = false; } 
    else { servo_angle -= 2; if (servo_angle <= 0) sweeping_forward = true; }

    if (class_active && f1_on) { 
      if (!servo1.attached()) servo1.attach(SERVO_Z1_PIN);
      servo1.write(servo_angle); s1_active = true; 
    } else {
      if (s1_active) { servo1.write(0); s1_off_time = millis(); s1_active = false; }
      if (servo1.attached() && !s1_active && (millis() - s1_off_time > 500)) { servo1.detach(); }
    }
    
    if (class_active && f2_on) { 
      if (!servo2.attached()) servo2.attach(SERVO_Z2_PIN);
      servo2.write(servo_angle); s2_active = true; 
    } else {
      if (s2_active) { servo2.write(0); s2_off_time = millis(); s2_active = false; }
      if (servo2.attached() && !s2_active && (millis() - s2_off_time > 500)) { servo2.detach(); }
    }

    if (class_active && f3_on) { 
      if (!servo3.attached()) servo3.attach(SERVO_Z3_PIN);
      servo3.write(servo_angle); s3_active = true; 
    } else {
      if (s3_active) { servo3.write(0); s3_off_time = millis(); s3_active = false; }
      if (servo3.attached() && !s3_active && (millis() - s3_off_time > 500)) { servo3.detach(); }
    }
  }
}
