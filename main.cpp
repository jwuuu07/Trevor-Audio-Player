

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <ESP32Servo.h>



const char* ssid = "troller";
const char* password = "12345678";
WebServer server(80);

HardwareSerial mySerial(1);
DFRobotDFPlayerMini myDFPlayer;

#define TRIG_PIN D6
#define ECHO_PIN D7

#define SERVO_PIN D4
Servo myServo;

int volumeLevel = 15;
bool isPaused = false;
float tripDistance = 160.0;
bool tripEnabled = true;
bool audioTriggered = false;       
float currentDistance = 0;
unsigned long lastDistanceCheck = 0;
const int DISTANCE_CHECK_INTERVAL = 100;
int servoPosition = 90;
int tripTrack = 1;               

enum ServoState { IDLE, MOVE_TO_180, HOLD_180, MOVE_TO_0, HOLD_0, RETURN };
ServoState servoState = IDLE;
unsigned long servoStateTimestamp = 0;
const unsigned long SERVO_HOLD_MS = 700;


const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Troller Control</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 20px;
}
.container {
  background: rgba(255, 255, 255, 0.95);
  border-radius: 20px;
  padding: 40px;
  max-width: 500px;
  width: 100%;
  box-shadow: 0 20px 60px rgba(0,0,0,0.3);
}
h1 {
  color: #667eea;
  text-align: center;
  margin-bottom: 30px;
  font-size: 28px;
}
.control-group {
  margin-bottom: 25px;
}
.control-group label {
  display: block;
  color: #333;
  font-weight: 600;
  margin-bottom: 10px;
  font-size: 14px;
}
.input-row {
  display: flex;
  gap: 10px;
}
input[type="number"] {
  flex: 1;
  padding: 12px;
  border: 2px solid #ddd;
  border-radius: 10px;
  font-size: 16px;
  transition: border-color 0.3s;
}
input[type="number"]:focus {
  outline: none;
  border-color: #667eea;
}
input[type="range"] {
  width: 100%;
  height: 8px;
  border-radius: 5px;
  background: #ddd;
  outline: none;
  -webkit-appearance: none;
}
input[type="range"]::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: #667eea;
  cursor: pointer;
  transition: background 0.3s;
}
input[type="range"]::-webkit-slider-thumb:hover {
  background: #764ba2;
}
input[type="range"]::-moz-range-thumb {
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: #667eea;
  cursor: pointer;
  border: none;
}
.volume-display {
  text-align: center;
  color: #667eea;
  font-size: 24px;
  font-weight: bold;
  margin-top: 10px;
}
.distance-display {
  text-align: center;
  color: #667eea;
  font-size: 20px;
  font-weight: bold;
  margin-top: 10px;
  padding: 15px;
  background: #f0f0f0;
  border-radius: 10px;
}
button {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border: none;
  padding: 14px 20px;
  border-radius: 10px;
  font-size: 15px;
  font-weight: 600;
  cursor: pointer;
  transition: transform 0.2s, box-shadow 0.2s;
  width: 100%;
}
button:hover {
  transform: translateY(-2px);
  box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
}
button:active {
  transform: translateY(0);
}
.btn-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  margin-top: 10px;
}
.btn-success {
  background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
}
.btn-secondary {
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
}
.playback-controls {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 10px;
  margin-top: 20px;
}
.playback-controls button {
  padding: 18px 10px;
  font-size: 16px;
}
.status {
  text-align: center;
  margin-top: 20px;
  padding: 12px;
  background: #f0f0f0;
  border-radius: 10px;
  color: #333;
  font-size: 14px;
  min-height: 45px;
  display: flex;
  align-items: center;
  justify-content: center;
}
.toggle-switch {
  position: relative;
  display: inline-block;
  width: 60px;
  height: 34px;
}
.toggle-switch input {
  opacity: 0;
  width: 0;
  height: 0;
}
.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #ccc;
  transition: .4s;
  border-radius: 34px;
}
.slider:before {
  position: absolute;
  content: "";
  height: 26px;
  width: 26px;
  left: 4px;
  bottom: 4px;
  background-color: white;
  transition: .4s;
  border-radius: 50%;
}
input:checked + .slider {
  background-color: #667eea;
}
input:checked + .slider:before {
  transform: translateX(26px);
}
.toggle-container {
  display: flex;
  align-items: center;
  gap: 15px;
  justify-content: center;
}
</style>
</head>
<body>
<div class="container">
  <h1>Troller Control</h1>
  
  <div class="control-group">
    <label>Distance Sensor</label>
    <div class="distance-display" id="distanceDisplay">-- cm</div>

    <div style="margin-top: 15px;">
      <label>Trip Distance (cm)</label>
      <div class="input-row">
        <input type="number" id="tripDist" min="1" max="400" value="160" placeholder="Distance">
        <button onclick="setTripDistance()" class="btn-success">Set</button>
      </div>
    </div>

    <!-- NEW: Trip Song control -->
    <div style="margin-top: 15px;">
      <label>Trip Song (Track #)</label>
      <div class="input-row">
        <input type="number" id="tripTrack" min="1" max="999" value="1" placeholder="Track for trip">
        <button onclick="setTripTrack()" class="btn-success">Set Trip Song</button>
      </div>
    </div>

    <div class="toggle-container" style="margin-top: 15px;">
      <label style="margin: 0;">Enable Trip</label>
      <label class="toggle-switch">
        <input type="checkbox" id="tripToggle" onchange="toggleTrip()" checked>
        <span class="slider"></span>
      </label>
    </div>
  </div>
  
  <div class="control-group">
    <label>Servo Control</label>
    <input type="range" id="servoSlider" min="0" max="180" value="90" oninput="updateServo()">
    <div class="volume-display" id="servoDisplay">90 deg</div>
  </div>
  
  <div class="control-group">
    <label>Track Number</label>
    <div class="input-row">
      <input type="number" id="trackNum" min="1" max="999" value="1" placeholder="Enter track">
      <button onclick="playTrack()" class="btn-success">Play</button>
    </div>
  </div>
  
  <div class="control-group">
    <label>Volume Control</label>
    <input type="range" id="volumeSlider" min="0" max="30" value="15" oninput="updateVolume()">
    <div class="volume-display" id="volumeDisplay">15</div>
  </div>
  
  <div class="playback-controls">
    <button onclick="send('prev')" title="Previous">Prev</button>
    <button onclick="send('pause')" title="Pause/Resume">Pause</button>
    <button onclick="send('stop')" title="Stop">Stop</button>
    <button onclick="send('next')" title="Next">Next</button>
  </div>
  
  <div class="btn-grid">
    <button onclick="send('repeat')" class="btn-secondary">Repeat</button>
    <button onclick="resetTrip()" class="btn-secondary">Reset Trip</button>
  </div>
  
  <div class="status" id="status">Ready</div>
</div>

<script>
function showStatus(msg) {
  document.getElementById('status').textContent = msg;
  setTimeout(() => {
    document.getElementById('status').textContent = 'Ready';
  }, 2000);
}

function updateDistance() {
  fetch('/distance')
    .then(r => r.json())
    .then(data => {
      if (data.distance > 0) {
        document.getElementById('distanceDisplay').textContent = data.distance.toFixed(1) + ' cm';
      } else {
        document.getElementById('distanceDisplay').textContent = 'Out of range';
      }
    });
}

function setTripDistance() {
  const dist = document.getElementById('tripDist').value;
  fetch('/tripdist?value=' + dist)
    .then(r => r.text())
    .then(t => showStatus('Trip distance set to ' + dist + ' cm'));
}

// NEW: set trip track (song)
function setTripTrack() {
  const t = document.getElementById('tripTrack').value;
  if (t < 1 || t > 999) {
    showStatus('Trip song must be 1–999');
    return;
  }
  fetch('/triptrack?value=' + t)
    .then(r => r.text())
    .then(() => showStatus('Trip song set to track ' + t));
}

function toggleTrip() {
  const enabled = document.getElementById('tripToggle').checked;
  fetch('/tripToggle?enabled=' + (enabled ? '1' : '0'))
    .then(r => r.text())
    .then(t => showStatus('Trip ' + (enabled ? 'enabled' : 'disabled')));
}

function resetTrip() {
  fetch('/resetTrip')
    .then(r => r.text())
    .then(t => showStatus('Trip reset'));
}

function updateServo() {
  const pos = document.getElementById('servoSlider').value;
  document.getElementById('servoDisplay').textContent = pos + ' deg';
  fetch('/servo?pos=' + pos)
    .then(r => r.text())
    .then(t => showStatus('Servo: ' + pos + ' deg'));
}

function playTrack() {
  const track = document.getElementById('trackNum').value;
  if (track < 1 || track > 999) {
    showStatus('Track must be between 1-999');
    return;
  }
  fetch('/play?track=' + track)
    .then(r => r.text())
    .then(t => showStatus('Playing track ' + track));
}

function updateVolume() {
  const vol = document.getElementById('volumeSlider').value;
  document.getElementById('volumeDisplay').textContent = vol;
  fetch('/volume?level=' + vol)
    .then(r => r.text())
    .then(t => showStatus('Volume: ' + vol));
}

function send(cmd) {
  const cmdNames = {
    'pause': 'Pause/Resume',
    'stop': 'Stopped',
    'next': 'Next track',
    'prev': 'Previous track',
    'repeat': 'Repeat mode'
  };
  fetch('/' + cmd)
    .then(r => r.text())
    .then(t => showStatus(cmdNames[cmd] || 'Command sent'));
}

document.addEventListener('DOMContentLoaded', function() {
  document.getElementById('trackNum').addEventListener('keypress', function(e) {
    if (e.key === 'Enter') playTrack();
  });
  setInterval(updateDistance, 300);
  updateDistance();
});
</script>
</body>
</html>
)rawliteral";


float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  float distance = (duration * 0.0343f) / 2.0f;
  if (distance > 400) return -1;

  return distance;
}

void checkDistanceTrigger() {
  if (!tripEnabled) return;

  if (currentDistance > 0 && currentDistance < tripDistance && !audioTriggered && servoState == IDLE) {
    myDFPlayer.play(tripTrack); 
    audioTriggered = true;
    Serial.printf("Distance trigger! Playing track %d. Distance: %.1f cm\n", tripTrack, currentDistance);

    myServo.write(180);
    servoState = MOVE_TO_180;
    servoStateTimestamp = millis();
    Serial.println("Servo -> 180°");
  }
}

void updateServoSequence() {
  unsigned long elapsed = millis() - servoStateTimestamp;

  switch (servoState) {
    case MOVE_TO_180:
      if (elapsed >= 500) { 
        servoState = HOLD_180;
        servoStateTimestamp = millis();
        Serial.println("Holding at 180°");
      }
      break;

    case HOLD_180:
      if (elapsed >= SERVO_HOLD_MS) {
        myServo.write(0);
        servoState = MOVE_TO_0;
        servoStateTimestamp = millis();
        Serial.println("Servo -> 0°");
      }
      break;

    case MOVE_TO_0:
      if (elapsed >= 500) {  
        servoState = HOLD_0;
        servoStateTimestamp = millis();
        Serial.println("Holding at 0°");
      }
      break;

    case HOLD_0:
      if (elapsed >= SERVO_HOLD_MS) {
        myServo.write(servoPosition);
        servoState = IDLE;
        Serial.printf("Servo returned to %d°\n", servoPosition);
      }
      break;

    case IDLE:
    case RETURN:
    default:
      break;
  }
}

void handleRoot() { 
  server.send(200, "text/html", htmlPage); 
}

void handleDistance() {
  String json = "{\"distance\":" + String(currentDistance, 1) + "}";
  server.send(200, "application/json", json);
}

void handleTripDistance() {
  if (server.hasArg("value")) {
    tripDistance = server.arg("value").toFloat();
    Serial.printf("Trip distance set to: %.1f cm\n", tripDistance);
    server.send(200, "text/plain", "OK");
    return;
  }
  server.send(400, "text/plain", "Invalid value");
}

void handleTripTrack() {
  if (server.hasArg("value")) {
    int t = server.arg("value").toInt();
    if (t >= 1 && t <= 999) {
      tripTrack = t;
      Serial.printf("Trip track set to: %d\n", tripTrack);
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid trip track (1-999)");
}

void handleTripToggle() {
  if (server.hasArg("enabled")) {
    tripEnabled = (server.arg("enabled") == "1");
    if (!tripEnabled) {
      audioTriggered = false;
    }
    Serial.printf("Trip %s\n", tripEnabled ? "enabled" : "disabled");
    server.send(200, "text/plain", "OK");
    return;
  }
  server.send(400, "text/plain", "Invalid value");
}

void handleResetTrip() {
  audioTriggered = false;
  Serial.println("Trip reset");
  server.send(200, "text/plain", "OK");
}

void handleServo() {
  if (server.hasArg("pos")) {
    servoPosition = server.arg("pos").toInt();
    if (servoPosition >= 0 && servoPosition <= 180) {
      if (servoState == IDLE) {  
        myServo.write(servoPosition);
      }
      Serial.printf("Servo position set to: %d\n", servoPosition);
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid position");
}

void handlePlay() {
  if (server.hasArg("track")) {
    int track = server.arg("track").toInt();
    if (track >= 1 && track <= 999) {
      myDFPlayer.play(track);
      Serial.printf("Playing track: %d\n", track);
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid track");
}

void handleVolume() {
  if (server.hasArg("level")) {
    int vol = server.arg("level").toInt();
    if (vol >= 0 && vol <= 30) {
      volumeLevel = vol;
      myDFPlayer.volume(volumeLevel);
      Serial.printf("Volume set to: %d\n", volumeLevel);
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid volume");
}

void handleCommand(String cmd) {
  if (cmd == "pause") {
    isPaused = !isPaused;
    if (isPaused) myDFPlayer.pause(); 
    else myDFPlayer.start();
  } else if (cmd == "stop") {
    myDFPlayer.stop();
  } else if (cmd == "next") {
    myDFPlayer.next();
  } else if (cmd == "prev") {
    myDFPlayer.previous();
  } else if (cmd == "repeat") {
    myDFPlayer.loop(1);
  }
  Serial.printf("Command: %s\n", cmd.c_str());
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  
  myServo.attach(SERVO_PIN);
  myServo.write(servoPosition);
  
  mySerial.begin(9600, SERIAL_8N1, D1, D0);
  delay(500);
  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("DFPlayer init failed. Check wiring and SD card.");
    while (true) delay(1000);
  }
  myDFPlayer.volume(volumeLevel);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  Serial.println("DFPlayer ready.");

  WiFi.softAP(ssid, password);
  Serial.print("Access Point: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/distance", handleDistance);
  server.on("/tripdist", handleTripDistance);
  server.on("/triptrack", handleTripTrack);   
  server.on("/tripToggle", handleTripToggle);
  server.on("/resetTrip", handleResetTrip);
  server.on("/servo", handleServo);
  server.on("/play", handlePlay);
  server.on("/volume", handleVolume);
  server.onNotFound([]() { server.send(404, "text/plain", "Not Found"); });

  const char* cmds[] = {"pause","stop","next","prev","repeat"};
  for (auto c : cmds) {
    server.on(("/" + String(c)).c_str(), [c]() { handleCommand(c); });
  }

  server.begin();
  Serial.println("HTTP server started.");
  Serial.println("Open the AP IP shown above in your browser (e.g., http://192.168.4.1)");
}

void loop() {
  server.handleClient();

  if (millis() - lastDistanceCheck >= DISTANCE_CHECK_INTERVAL) {
    currentDistance = getDistance();
    lastDistanceCheck = millis();

    if (currentDistance > 0)
      Serial.printf("Distance: %.1f cm\n", currentDistance);
    else
      Serial.println("Distance: out of range");

    checkDistanceTrigger();
  }

  updateServoSequence();
}
