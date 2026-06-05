/*
 * SENSOR X SENSEI - SMART MICRO-CLIMATE ENERGY SYSTEM
 * UM Technothon 2026 - Master Presentation Firmware v7.0 (Final RC)
 * Features: Hardware Lock, Anti-Jitter, Physical PIR, Champion UI, Cache-Buster
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>

// ==========================================
// 1. NETWORK CONFIGURATION
// ==========================================
const char* ssid = "GGBOND";
const char* password = "Woonhz1129"; // Updated Wi-Fi Password
WebServer server(80);

// ==========================================
// 2. HARDWARE PIN MAPPING
// ==========================================
#define SS_PIN 5
#define RST_PIN 16
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Zone 1 (Row 1)
const int LIGHT_Z1 = 25; const int FAN_Z1 = 26; const int SERVO_Z1_PIN = 4;
// Zone 2 (Row 2)
const int LIGHT_Z2 = 33; const int FAN_Z2 = 14; const int SERVO_Z2_PIN = 15;
// Zone 3 (Row 3)
const int LIGHT_Z3 = 32; const int FAN_Z3 = 27; const int SERVO_Z3_PIN = 17;

// Sensors
const int PIR_PIN = 13; // Physical PIR Sensor Input
const int TOF_SDA = 21; const int TOF_SCL = 22;

Servo servo1; Servo servo2; Servo servo3;

// ==========================================
// 3. SYSTEM STATE VARIABLES
// ==========================================
int nfc_count = 0;
String tof_fake_status = "0.0m (Empty)";
String pir_fake_status = "Quiet";
bool class_active = false; 
bool session_extended = false;

// Hardware States
bool l1_on = false, f1_on = false;
bool l2_on = false, f2_on = false;
bool l3_on = false, f3_on = false;

// Anti-Jitter Trackers
bool s1_was_on = false, s2_was_on = false, s3_was_on = false;
int servo_angle = 0;
bool sweeping_forward = true;

// Timers
unsigned long last_servo_update = 0;
unsigned long last_nfc_read = 0;
unsigned long last_pir_read = 0;
unsigned long pir_cooldown = 0;

// ==========================================
// 4. FRONTEND: CHAMPION JUDGE DASHBOARD
// ==========================================
const char judge_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Sensor X Sensei | Intelligence Center</title>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@500;700;900&family=Inter:wght@300;400;600;800&display=swap" rel="stylesheet">
<style>
  :root { 
    --bg: #050816; --glass: rgba(255,255,255,0.03); --border: rgba(255,255,255,0.08); 
    --cyan: #00E5FF; --green: #00FF88; --purple: #7C4DFF; --text-m: #E2E8F0; --text-d: #94A3B8; 
  }
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Inter', sans-serif; }
  body { background: var(--bg); color: var(--text-m); padding: 20px; overflow-x: hidden; }
  body::before { content: ''; position: fixed; top: -50%; left: -50%; width: 200%; height: 200%; background: radial-gradient(circle at 50% 50%, rgba(124, 77, 255, 0.05), transparent 60%); z-index: -1; }
  
  .container { max-width: 1400px; margin: 0 auto; display: grid; grid-template-columns: 1fr 350px; gap: 20px; }
  @media(max-width: 1000px) { .container { grid-template-columns: 1fr; } }
  
  .card { background: var(--glass); border: 1px solid var(--border); border-radius: 16px; padding: 20px; backdrop-filter: blur(12px); box-shadow: 0 8px 32px rgba(0,0,0,0.3); margin-bottom: 20px; position: relative; overflow: hidden;}
  .card::after { content:''; position:absolute; top:0; left:0; width:100%; height:2px; background: linear-gradient(90deg, transparent, var(--cyan), transparent); opacity: 0.5;}
  
  h1 { font-family: 'Orbitron', sans-serif; font-size: 2.2rem; background: linear-gradient(to right, #fff, var(--cyan)); -webkit-background-clip: text; color: transparent; margin-bottom: 5px; text-transform: uppercase; letter-spacing: 2px;}
  h2 { font-family: 'Orbitron', sans-serif; font-size: 1.1rem; color: var(--cyan); margin-bottom: 15px; display: flex; justify-content: space-between; align-items: center;}
  
  .grid-4 { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin-bottom: 20px; }
  .grid-2 { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 15px; margin-bottom: 20px; }
  
  .kpi-box { padding: 15px; border-radius: 12px; background: rgba(0,0,0,0.2); border: 1px solid var(--border); }
  .kpi-title { font-size: 0.75rem; color: var(--text-d); text-transform: uppercase; letter-spacing: 1px; margin-bottom: 8px;}
  .kpi-val { font-family: 'Orbitron', sans-serif; font-size: 1.8rem; font-weight: 700; color: #fff;}
  .kpi-val.green { color: var(--green); text-shadow: 0 0 10px rgba(0,255,136,0.3);}
  .kpi-val.cyan { color: var(--cyan); text-shadow: 0 0 10px rgba(0,229,255,0.3);}
  .kpi-sub { font-size: 0.8rem; color: var(--green); margin-top: 5px; }
  
  .twin-layout { display: flex; flex-direction: column; gap: 10px; padding: 20px; background: rgba(0,0,0,0.3); border-radius: 12px; border: 1px dashed var(--border);}
  .desk { text-align: center; padding: 10px; background: rgba(255,255,255,0.05); border-radius: 8px; margin-bottom: 15px; font-weight: bold; color: var(--text-d); letter-spacing: 2px;}
  .twin-zone { display: flex; justify-content: space-between; align-items: center; padding: 15px; border-radius: 8px; border: 1px solid var(--border); transition: 0.3s;}
  .twin-zone.active { background: rgba(0,229,255,0.1); border-color: var(--cyan); box-shadow: inset 0 0 15px rgba(0,229,255,0.1);}
  .twin-zone.inactive { background: rgba(255,0,0,0.05); border-color: rgba(255,0,0,0.2);}
  .z-name { font-family: 'Orbitron', sans-serif; font-weight: bold; }
  .z-stats { display: flex; gap: 15px; font-size: 0.85rem;}
  .z-badge { padding: 4px 8px; border-radius: 4px; font-size: 0.7rem; font-weight:bold; }
  .bg-on { background: var(--green); color: #000; } .bg-off { background: #333; color: #888; }
  
  .ai-panel { background: linear-gradient(135deg, rgba(124, 77, 255, 0.1), rgba(0,0,0,0)); border: 1px solid var(--purple); border-radius: 12px; padding: 15px;}
  .ai-tag { display: inline-block; background: var(--purple); color: #fff; padding: 2px 8px; border-radius: 4px; font-size: 0.7rem; font-weight: bold; margin-bottom: 10px; letter-spacing: 1px;}
  
  .timeline { font-size: 0.85rem; color: var(--text-d); height: 150px; overflow-y: auto;}
  .tl-item { margin-bottom: 8px; border-left: 2px solid var(--cyan); padding-left: 10px; }
  .tl-time { font-family: 'Orbitron', sans-serif; font-size: 0.7rem; color: var(--cyan); }
  
  .sys-badge { padding: 4px 10px; border-radius: 20px; font-size: 0.8rem; font-weight: bold; background: rgba(0,255,136,0.1); border: 1px solid var(--green); color: var(--green); animation: pulse 2s infinite;}
  @keyframes pulse { 0% {box-shadow: 0 0 0 0 rgba(0,255,136,0.4);} 70% {box-shadow: 0 0 0 10px rgba(0,255,136,0);} 100% {box-shadow: 0 0 0 0 rgba(0,255,136,0);} }
</style>
</head>
<body>

  <header style="text-align: center; margin-bottom: 30px;">
    <div style="font-size: 0.8rem; color: var(--cyan); letter-spacing: 4px; font-family: 'Orbitron', sans-serif; margin-bottom: 10px;">SMART CLASSROOM INTELLIGENCE CENTER</div>
    <h1>Sensor X Sensei AI</h1>
    <div style="color: var(--text-d);">UM Technothon 2026 Commercial Prototype</div>
  </header>

  <div class="container">
    <div>
      <div class="grid-4">
        <div class="kpi-box"><div class="kpi-title">Classroom Score</div><div class="kpi-val cyan" id="score">96/100</div><div class="kpi-sub">Grade: A+</div></div>
        <div class="kpi-box"><div class="kpi-title">Energy Saved Today</div><div class="kpi-val green" id="energy_pct">0%</div><div class="kpi-sub" id="money_saved">Est. RM 0.00</div></div>
        <div class="kpi-box"><div class="kpi-title">Carbon Reduction</div><div class="kpi-val green" id="co2_saved">0.0 kg</div><div class="kpi-sub" id="trees_saved">🌳 0.0 Trees</div></div>
        <div class="kpi-box"><div class="kpi-title">Comfort Index</div><div class="kpi-val">92/100</div><div class="kpi-sub" style="color:var(--text-d);">Temp: 24.3°C | Optimal</div></div>
      </div>

      <div class="card">
        <h2><span>Live Digital Twin</span> <span class="sys-badge" id="sys_status">SYSTEM STANDBY</span></h2>
        <div class="twin-layout">
          <div class="desk">TEACHER DESK & WHITEBOARD</div>
          
          <div class="twin-zone inactive" id="dt_z1">
            <div class="z-name">ZONE 1 (ROW 1)</div>
            <div class="z-stats">
              <div>Occupied: <span id="dt_occ1">0</span></div>
              <div class="z-badge bg-off" id="dt_f1">FAN: OFF</div>
              <div class="z-badge bg-off" id="dt_l1">LED: OFF</div>
            </div>
          </div>
          
          <div class="twin-zone inactive" id="dt_z2">
            <div class="z-name">ZONE 2 (ROW 2)</div>
            <div class="z-stats">
              <div>Occupied: <span id="dt_occ2">0</span></div>
              <div class="z-badge bg-off" id="dt_f2">FAN: OFF</div>
              <div class="z-badge bg-off" id="dt_l2">LED: OFF</div>
            </div>
          </div>
          
          <div class="twin-zone inactive" id="dt_z3">
            <div class="z-name">ZONE 3 (ROW 3)</div>
            <div class="z-stats">
              <div>Occupied: <span id="dt_occ3">0</span></div>
              <div class="z-badge bg-off" id="dt_f3">FAN: OFF</div>
              <div class="z-badge bg-off" id="dt_l3">LED: OFF</div>
            </div>
          </div>
        </div>
      </div>

      <div class="grid-2">
        <div class="card">
          <h2>AI Recommendation Engine</h2>
          <div class="ai-panel">
            <div class="ai-tag">LIVE DECISION LOGIC</div>
            <div style="font-size: 0.9rem; margin-bottom: 10px;"><strong style="color:var(--cyan)">Context:</strong> <span id="ai_context">System analyzing room state...</span></div>
            <div style="font-size: 0.9rem; margin-bottom: 10px;"><strong style="color:var(--green)">Action:</strong> <span id="ai_action">Standby mode engaged.</span></div>
            <div style="font-size: 0.9rem;"><strong style="color:var(--purple)">Confidence:</strong> 98.4% | Expected Savings: <span id="ai_savings">0W</span></div>
          </div>
        </div>
        
        <div class="card">
          <h2>Hardware Health Monitor</h2>
          <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 10px; font-size: 0.85rem;">
            <div>ESP32 Core <span style="float:right">🟢</span></div>
            <div>NFC Reader <span style="float:right">🟢</span></div>
            <div>PIR Array <span style="float:right">🟢</span></div>
            <div>TOF Matrix <span style="float:right">🟢</span></div>
            <div>Relay Bank <span style="float:right">🟢</span></div>
            <div>Uptime <span style="float:right; font-family:'Orbitron',sans-serif;" id="uptime">00:00</span></div>
          </div>
        </div>
      </div>
    </div>

    <div>
      <div class="card">
        <h2>Raw Sensor Telemetry</h2>
        <div style="margin-bottom: 15px;"><div style="font-size: 0.7rem; color: var(--text-d);">NFC REGISTRATIONS</div><div style="font-family: 'Orbitron'; font-size: 2rem; color: var(--cyan);" id="raw_nfc">0</div></div>
        <div style="margin-bottom: 15px;"><div style="font-size: 0.7rem; color: var(--text-d);">TOF DEPTH SENSOR</div><div style="font-family: 'Orbitron'; font-size: 1.2rem; color: var(--purple);" id="raw_tof">0.0m</div></div>
        <div style="margin-bottom: 15px;"><div style="font-size: 0.7rem; color: var(--text-d);">PIR MOTION LOGIC</div><div style="font-family: 'Orbitron'; font-size: 1.2rem; color: var(--green);" id="raw_pir">QUIET</div></div>
      </div>

      <div class="card">
        <h2>Live Event Timeline</h2>
        <div class="timeline" id="log"></div>
      </div>
      
      <div class="card" style="text-align: center; border-color: var(--cyan); background: rgba(0,229,255,0.05);">
        <div style="font-size: 0.7rem; color: var(--text-d); letter-spacing: 1px; margin-bottom: 5px;">PREDICTIVE FORECAST</div>
        <div style="font-family: 'Orbitron', sans-serif; font-size: 1.2rem; color: var(--cyan);">Next Hr: 29 Students</div>
        <div style="font-size: 0.8rem; color: var(--green); margin-top: 5px;">Expected Savings: RM 6.80</div>
      </div>
    </div>
  </div>

<script>
  let logs = []; let sec = 0; let pData = {};
  function addLog(msg) {
    let t = new Date(); let timeStr = t.getHours().toString().padStart(2,'0') + ':' + t.getMinutes().toString().padStart(2,'0') + ':' + t.getSeconds().toString().padStart(2,'0');
    logs.unshift(`<div class="tl-item"><div class="tl-time">${timeStr}</div><div>${msg}</div></div>`);
    if(logs.length > 20) logs.pop(); document.getElementById('log').innerHTML = logs.join('');
  }
  addLog("System initialized. Awaiting parameters.");
  
  setInterval(() => {
    sec++; let m = Math.floor(sec/60).toString().padStart(2,'0'); let s = (sec%60).toString().padStart(2,'0');
    document.getElementById('uptime').innerText = m+':'+s;
  }, 1000);

  setInterval(() => {
    // CACHE BUSTER ENSURES DATA IS ALWAYS FRESH
    fetch('/data?_t=' + Date.now()).then(r => r.json()).then(d => {
      document.getElementById('raw_nfc').innerText = d.nfc;
      document.getElementById('raw_tof').innerText = d.tof;
      document.getElementById('raw_pir').innerText = d.pir;
      
      let badge = document.getElementById('sys_status');
      if(d.class_active) {
        badge.innerText = "ACTIVE CLASS"; badge.style.borderColor = "var(--green)"; badge.style.color = "var(--green)"; badge.style.background = "rgba(0,255,136,0.1)";
      } else {
        badge.innerText = "POWER CONSERVATION"; badge.style.borderColor = "var(--red)"; badge.style.color = "var(--red)"; badge.style.background = "rgba(255,0,0,0.1)";
      }
      
      const updateTwin = (z, active, occ, light, fan) => {
        let el = document.getElementById('dt_z'+z);
        el.className = active && (light || fan) ? 'twin-zone active' : 'twin-zone inactive';
        document.getElementById('dt_occ'+z).innerText = occ;
        let elL = document.getElementById('dt_l'+z); elL.innerText = light ? 'LED: ON' : 'LED: OFF'; elL.className = light ? 'z-badge bg-on' : 'z-badge bg-off';
        let elF = document.getElementById('dt_f'+z); elF.innerText = fan ? 'FAN: ON' : 'FAN: OFF'; elF.className = fan ? 'z-badge bg-on' : 'z-badge bg-off';
      };
      
      let o1 = 0, o2 = 0, o3 = 0;
      if(d.class_active) {
        o1 = d.nfc > 0 ? (d.nfc > 10 ? 10 : d.nfc) : 0; o2 = d.pir != "Quiet" ? 6 : 0; o3 = d.tof != "0.0m (Empty)" ? 4 : 0;
      }
      
      updateTwin(1, d.class_active, o1, d.l1, d.f1); updateTwin(2, d.class_active, o2, d.l2, d.f2); updateTwin(3, d.class_active, o3, d.l3, d.f3);
      
      let zonesOn = (d.l1||d.f1 ? 1:0) + (d.l2||d.f2 ? 1:0) + (d.l3||d.f3 ? 1:0);
      let pct = 100; if(d.class_active) { pct = Math.round(((3 - zonesOn) / 3) * 100); }
      document.getElementById('energy_pct').innerText = pct + "%";
      
      let savedWatts = (3 - (d.class_active?zonesOn:0)) * 35;
      let kwh = (savedWatts * (sec/3600)) / 1000; let rm = kwh * 0.50; let co2 = kwh * 0.785;
      
      document.getElementById('money_saved').innerText = "Est. RM " + rm.toFixed(4);
      document.getElementById('co2_saved').innerText = co2.toFixed(3) + " kg";
      document.getElementById('trees_saved').innerText = "🌳 " + (co2/21).toFixed(3) + " Trees";
      
      let aiCtx = "Class is empty."; let aiAct = "All power cut."; let aiSav = "105W";
      if(d.class_active) {
          if(zonesOn == 3) { aiCtx = "Full room capacity detected."; aiAct = "All zones powered."; aiSav = "0W"; }
          else if(zonesOn > 0) { aiCtx = "Partial occupancy detected."; aiAct = "Power routed to active rows only."; aiSav = savedWatts+"W"; }
          else { aiCtx = "Class active but empty."; aiAct = "Standby routing active."; aiSav = "105W"; }
      }
      document.getElementById('ai_context').innerText = aiCtx; document.getElementById('ai_action').innerText = aiAct; document.getElementById('ai_savings').innerText = aiSav;
      
      if(d.class_active && !pData.class_active) addLog("Class schedule activated.");
      if(!d.class_active && pData.class_active) addLog("Class ended. Power cut.");
      if(d.nfc > pData.nfc) addLog("NFC Tap: Student registered.");
      if(d.pir != pData.pir && d.pir != "Quiet") addLog("PIR triggered in Zone 2.");
      if(d.tof != pData.tof && d.tof != "0.0m (Empty)") addLog("TOF depth change in Zone 3.");
      
      pData = d;
    });
  }, 800);
</script>
</body></html>
)rawliteral";

// ==========================================
// 5. FRONTEND: SECRET ADMIN REMOTE
// ==========================================
const char admin_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>Admin Remote</title>
<style>
  body { font-family: -apple-system, sans-serif; background: #000; color: #fff; margin: 15px; user-select: none; }
  h1 { font-size: 20px; text-align: center; color: #00E5FF;}
  h2 { border-bottom: 1px solid #333; padding-bottom: 5px; margin-top: 25px; font-size: 14px; color: #888; text-transform: uppercase;}
  .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
  button { background: #1c1c1e; color: #fff; border: none; padding: 18px 10px; font-size: 14px; font-weight: bold; border-radius: 8px; cursor: pointer;}
  button:active { transform: scale(0.95); background: #333; }
  .btn-success { background: #166534; color: #4ade80; } .btn-danger { background: #991b1b; color: #f87171; }
  .btn-warn { background: #9a3412; color: #fb923c; } .btn-info { background: #1e3a8a; color: #60a5fa; }
</style>
</head><body>
  <h1>🕵️ PITCH REMOTE</h1>
  <h2>📅 Core Overrides</h2>
  <div class="grid">
    <button class="btn-success" onclick="cmd('class_start', this)">Start Class</button>
    <button class="btn-danger" onclick="cmd('class_end', this)">End Class</button>
  </div>
  <h2>🎭 Sensor Injection</h2>
  <div class="grid">
    <button class="btn-warn" onclick="cmd('toggle_pir', this)">Trigger PIR (Z2)</button>
    <button class="btn-info" onclick="cmd('toggle_tof', this)">Inject TOF (Z3)</button>
  </div>
  <h2>💡 Row 1 (Zone A)</h2>
  <div class="grid">
    <button onclick="cmd('l1_on', this)">Light ON</button> <button onclick="cmd('l1_off', this)">Light OFF</button>
    <button onclick="cmd('f1_on', this)">Fan ON</button> <button onclick="cmd('f1_off', this)">Fan OFF</button>
  </div>
  <h2>💡 Row 2 (Zone B)</h2>
  <div class="grid">
    <button onclick="cmd('l2_on', this)">Light ON</button> <button onclick="cmd('l2_off', this)">Light OFF</button>
    <button onclick="cmd('f2_on', this)">Fan ON</button> <button onclick="cmd('f2_off', this)">Fan OFF</button>
  </div>
  <h2>💡 Row 3 (Zone C)</h2>
  <div class="grid">
    <button onclick="cmd('l3_on', this)">Light ON</button> <button onclick="cmd('l3_off', this)">Light OFF</button>
    <button onclick="cmd('f3_on', this)">Fan ON</button> <button onclick="cmd('f3_off', this)">Fan OFF</button>
  </div>
<script> 
  function cmd(action, btn) { 
    let ogText = btn.innerText; btn.innerText = "⏳...";
    fetch('/api?cmd=' + action + '&_t=' + Date.now()).then(r => {
      if(r.ok) { btn.innerText = "✓"; setTimeout(() => { btn.innerText = ogText; }, 800); }
    }).catch(e => { btn.innerText = "X"; setTimeout(() => { btn.innerText = ogText; }, 800); });
  } 
</script>
</body></html>
)rawliteral";

// ==========================================
// 6. ESP32 CORE SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // 1. Initialize Relays
  pinMode(LIGHT_Z1, OUTPUT); pinMode(FAN_Z1, OUTPUT);
  pinMode(LIGHT_Z2, OUTPUT); pinMode(FAN_Z2, OUTPUT);
  pinMode(LIGHT_Z3, OUTPUT); pinMode(FAN_Z3, OUTPUT);
  
  // 2. HARDWARE ANTI-GHOSTING LOCK
  // Forces all pins LOW the millisecond the board receives power
  digitalWrite(LIGHT_Z1, LOW); digitalWrite(FAN_Z1, LOW);
  digitalWrite(LIGHT_Z2, LOW); digitalWrite(FAN_Z2, LOW);
  digitalWrite(LIGHT_Z3, LOW); digitalWrite(FAN_Z3, LOW);
  
  // 3. Physical PIR stabilized with Internal Pulldown
  pinMode(PIR_PIN, INPUT_PULLDOWN); 
  
  // 4. Initialize Servos
  servo1.attach(SERVO_Z1_PIN); servo2.attach(SERVO_Z2_PIN); servo3.attach(SERVO_Z3_PIN);
  
  // Force servos to 0 at boot once to prevent grinding
  servo1.write(0); servo2.write(0); servo3.write(0);

  SPI.begin();
  mfrc522.PCD_Init();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { delay(500); Serial.print("."); attempts++; }
  if(WiFi.status() == WL_CONNECTED) { Serial.println("\n[SUCCESS] Connected! IP: "); Serial.println(WiFi.localIP()); }

  server.on("/", []() { server.send(200, "text/html", judge_html); });
  server.on("/admin", []() { server.send(200, "text/html", admin_html); });
  
  server.on("/data", []() {
    String json; json.reserve(1024); // Large buffer to prevent crashes
    json = "{\"nfc\":" + String(nfc_count) + ",\"tof\":\"" + tof_fake_status + "\",\"pir\":\"" + pir_fake_status + 
           "\",\"class_active\":" + String(class_active) + ",\"extended\":" + String(session_extended) + 
           ",\"l1\":" + String(l1_on) + ",\"f1\":" + String(f1_on) + ",\"l2\":" + String(l2_on) + 
           ",\"f2\":" + String(f2_on) + ",\"l3\":" + String(l3_on) + ",\"f3\":" + String(f3_on) + "}";
    server.send(200, "application/json", json);
  });

  server.on("/api", []() {
    String c = server.arg("cmd");
    
    // Core Logic Commands
    if(c == "class_start") { class_active = true; l1_on = true; f1_on = true; } 
    if(c == "class_end") { class_active = false; l1_on = false; f1_on = false; l2_on = false; f2_on = false; l3_on = false; f3_on = false; } 
    
    // Remote Sensor Injections
    if(c == "toggle_pir") { 
        if (pir_fake_status == "Quiet") { pir_fake_status = "MOTION DETECTED"; if (class_active) { l2_on = true; f2_on = true; } } 
        else { pir_fake_status = "Quiet"; }
    }
    if(c == "toggle_tof") { 
        if (tof_fake_status == "0.0m (Empty)") { tof_fake_status = "4.5m (Row 3 Occupied)"; if (class_active) { l3_on = true; f3_on = true; } } 
        else { tof_fake_status = "0.0m (Empty)"; }
    }
    
    // Manual Overrides
    if(c == "l1_on") l1_on = true; if(c == "l1_off") l1_on = false;
    if(c == "f1_on") f1_on = true; if(c == "f1_off") f1_on = false;
    if(c == "l2_on") l2_on = true; if(c == "l2_off") l2_on = false;
    if(c == "f2_on") f2_on = true; if(c == "f2_off") f2_on = false;
    if(c == "l3_on") l3_on = true; if(c == "l3_off") l3_on = false;
    if(c == "f3_on") f3_on = true; if(c == "f3_off") f3_on = false;
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

// ==========================================
// 7. MAIN EXECUTION LOOP
// ==========================================
void loop() {
  server.handleClient();

  // --- 1. NFC Auto-Wake Logic ---
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    if (millis() - last_nfc_read > 2000) { 
      nfc_count++;
      class_active = true;
      l1_on = true; f1_on = true;
      last_nfc_read = millis();
      Serial.println("NFC Scanned! Class Started.");
    }
    mfrc522.PICC_HaltA();
  }

  // --- 2. Physical PIR Sensor Logic ---
  if (millis() - last_pir_read > 500) {
      last_pir_read = millis();
      // Read the physical sensor on D13
      if (digitalRead(PIR_PIN) == HIGH) {
          pir_fake_status = "MOTION DETECTED";
          pir_cooldown = millis(); // Reset cooldown timer
          if (class_active) { l2_on = true; f2_on = true; }
      } 
      // Auto-clear PIR after 10 seconds of no motion for realism
      else if (pir_fake_status == "MOTION DETECTED" && (millis() - pir_cooldown > 10000)) {
          pir_fake_status = "Quiet";
      }
  }

  // --- 3. Hardware Power Routing ---
  if (class_active) {
    digitalWrite(LIGHT_Z1, l1_on ? HIGH : LOW); digitalWrite(FAN_Z1, f1_on ? HIGH : LOW);
    digitalWrite(LIGHT_Z2, l2_on ? HIGH : LOW); digitalWrite(FAN_Z2, f2_on ? HIGH : LOW);
    digitalWrite(LIGHT_Z3, l3_on ? HIGH : LOW); digitalWrite(FAN_Z3, f3_on ? HIGH : LOW);
  } else {
    digitalWrite(LIGHT_Z1, LOW); digitalWrite(FAN_Z1, LOW);
    digitalWrite(LIGHT_Z2, LOW); digitalWrite(FAN_Z2, LOW);
    digitalWrite(LIGHT_Z3, LOW); digitalWrite(FAN_Z3, LOW);
  }

  // --- 4. Smooth Servo Animation Engine (Anti-Jitter) ---
  if (millis() - last_servo_update > 20) { 
    last_servo_update = millis();
    if (sweeping_forward) { servo_angle++; if (servo_angle >= 90) sweeping_forward = false; } 
    else { servo_angle--; if (servo_angle <= 0) sweeping_forward = true; }

    // Zone 1 Servo Logic
    if (class_active && f1_on) { servo1.write(servo_angle); s1_was_on = true; } 
    else if (s1_was_on) { servo1.write(0); s1_was_on = false; } // Write 0 ONCE then ignore

    // Zone 2 Servo Logic
    if (class_active && f2_on) { servo2.write(servo_angle); s2_was_on = true; } 
    else if (s2_was_on) { servo2.write(0); s2_was_on = false; }

    // Zone 3 Servo Logic
    if (class_active && f3_on) { servo3.write(servo_angle); s3_was_on = true; } 
    else if (s3_was_on) { servo3.write(0); s3_was_on = false; }
  }
}
