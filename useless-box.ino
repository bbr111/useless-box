// Platform libraries.
#include <Arduino.h>  // To add IntelliSense for platform constants.
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#define EEPROM_SIZE 128

struct WifiConfig {
  char ssid[32];
  char password[64];
};

struct SettingsConfig {
  bool isSoundEnabled;
};

bool isSoundEnabled = true;

// My classes.
#include "speed-servo.h"
#include "status-led.h"
#include "proximity-sensor.h"

#include "config.h"  // To store configuration and secrets.
#include "beeperfx.h"

SpeedServo lidServo;
SpeedServo switchServo;
StatusLed led;
ProximitySensor sensor;

int lastSwitchState = 0;
long playCount = 0;
bool isLidOpen = false;
bool monitorSensor = false;
String lastServoAction = "—";
String currentRunMode = "—";

#define AP_SSID "JontesBox"
#define AP_PASSWORD "123456789"

String ssid = "";
String password = "";

// Webserver
ESP8266WebServer server(80);
DNSServer dnsServer;

bool isWifiConfigured = false;

void handleSetWifi();
void handleProximity();
void handleSetServo();
void handleResetServo();
void handleConfigPage();
void handleWifiStatus();
void handleStatus();
void run();
void runSlow();
void runWaitThenFast();
void runFast();
void runFastThenClap();
void runOpenCloseThenFast();
void runPeekThenFast();
void runFastWithDelay();
void runClap();
void runHalf();
void openLidSlow();
void openLidFast();
void closeLidSlow();
void closeLidFast();
void clapLid();
void flipSwitchSlow();
void flipSwitchFast();
void handlePlaySound();
void handleSetSound();

void saveWifiConfig(const WifiConfig& config) {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, config);
  EEPROM.commit();
  EEPROM.end();
}

bool loadWifiConfig(WifiConfig& config) {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, config);
  EEPROM.end();

  // simple Plausibilitätsprüfung: SSID nicht leer
  return strlen(config.ssid) > 0;
}

void saveSettingsConfig(const SettingsConfig& settings) {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(96, settings);  // hinter WifiConfig speichern
  EEPROM.commit();
  EEPROM.end();
}

bool loadSettingsConfig(SettingsConfig& settings) {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(96, settings);
  EEPROM.end();
  // Keine große Validierung nötig hier
  return true;
}

void setup() {
  initSerial();
  initServos();
  initLed();
  BeeperFX::begin(BEEPER_PIN);     // Beispiel: KY-006 am Pin D3
  BeeperFX::startup();     // Beim Start piepen
  Serial.println("Vor initSensor()");
  initSensor();
  Serial.println("Nach initSensor()");
  pinMode(PIN_SWITCH, INPUT);

  Serial.printf("Application version: %s\n", APP_VERSION);
  Serial.println("Setup completed.");

  // Versuche WLAN-Daten zu laden
  WifiConfig cfg;
  if (loadWifiConfig(cfg)) {
    ssid = String(cfg.ssid);
    password = String(cfg.password);
    isWifiConfigured = true;

    Serial.println("🔁 WLAN-Daten gefunden. Versuche Verbindung...");
    Serial.println("SSID: " + ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(1000);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Verbunden mit: " + ssid);
      Serial.print("IP-Adresse: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\n❌ Verbindung fehlgeschlagen. Starte AP-Modus...");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID, AP_PASSWORD);
      Serial.println("AP IP: ");
      Serial.println(WiFi.softAPIP());
    }
  } else {
    Serial.println("⚠️ Keine WLAN-Daten gefunden. Starte im AP-Modus...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.println("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }

  SettingsConfig settings;
  if (loadSettingsConfig(settings)) {
    isSoundEnabled = settings.isSoundEnabled;
  }

  // Start DNS server to redirect all requests to the server
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Define routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/setWifi", HTTP_POST, handleSetWifi);
  server.on("/proximity", HTTP_GET, handleProximity);
  server.on("/setServo", HTTP_GET, handleSetServo);
  server.on("/resetServo", HTTP_GET, handleResetServo);
  server.on("/config", HTTP_GET, handleConfigPage);  // ggf. handleRoot()
  server.on("/wifiStatus", HTTP_GET, handleWifiStatus);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/resetWifi", HTTP_POST, handleResetWifi);
  server.on("/playSound", HTTP_GET, handlePlaySound);
  server.on("/setSound", HTTP_GET, handleSetSound);

  // Start the server
  server.begin();
  Serial.println("Web Server Started");
}

void initSerial() {
  Serial.begin(115200);
  delay(3000);
  Serial.println();
  Serial.println("Initializing serial connection DONE.");
}

void initServos() {
  lidServo.attach(PIN_LID_SERVO);
  lidServo.moveNowTo(LID_START_POSITION);

  switchServo.attach(PIN_SWITCH_SERVO);
  switchServo.moveNowTo(SWITCH_START_POSITION);
}

void initLed() {
  led.setPin(LED_BUILTIN);
  led.turnOff();
}

void initSensor() {
  sensor.attach(PIN_SENSOR_SDA, PIN_SENSOR_SCL, SENSOR_TRIGGER_THRESHOLD);
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  // // If Wi-Fi is configured, try to connect
  // if (isWifiConfigured && ssid.length() > 0) {
  //   WiFi.begin(ssid, password);
  //   int attempts = 0;
  //   while (WiFi.status() != WL_CONNECTED && attempts < 30) {
  //     delay(1000);
  //     Serial.print(".");
  //     attempts++;
  //   }

  //   if (WiFi.status() == WL_CONNECTED) {
  //     Serial.println("Connected to Wi-Fi!");
  //     Serial.print("IP Address: ");
  //     Serial.println(WiFi.localIP());
  //   } else {
  //     Serial.println("Failed to connect to Wi-Fi. Reverting to AP mode.");
  //     WiFi.mode(WIFI_AP);
  //     WiFi.softAP(AP_SSID, AP_PASSWORD);
  //     Serial.println("Access Point Started Again");
  //   }
  // }

  int switchState = digitalRead(PIN_SWITCH);
  boolean isSwitchTurnedOn = (switchState != lastSwitchState) && (switchState == LOW);

  if (isSwitchTurnedOn) {
    led.turnOn();
    run();
    isLidOpen = false;
    led.turnOff();
  } else {
    // Check the proximity sensor.
    if (sensor.isInRange()) {
      if (!isLidOpen && monitorSensor) {
        openLidFast();
        isLidOpen = true;
      }
    } else {
      if (isLidOpen) {
        closeLidFast();
        isLidOpen = false;
      }
    }
  }

  lastSwitchState = switchState;

  // Wait 250 ms before next reading (required for the sensor).
  delay(250);
}

void handlePlaySound() {
  if (!server.hasArg("sound")) {
    server.send(400, "text/plain", "Fehlender Parameter");
    return;
  }

  String s = server.arg("sound");

  if (s == "startup") BeeperFX::startup();
  else if (s == "success") BeeperFX::success();
  else if (s == "error") BeeperFX::error();
  else if (s == "r2d2v1") BeeperFX::r2d2_v1();
  else if (s == "r2d2v2") BeeperFX::r2d2_v2();
  else if (s == "imperialshort") BeeperFX::imperialMarchShort();
  else if (s == "imperiallong") BeeperFX::imperialMarchLong();
  else if (s == "superMarioShort") BeeperFX::superMarioShort();
  else if (s == "nokiaTune") BeeperFX::nokiaTune();
  else if (s == "tetrisIntro") BeeperFX::tetrisIntro();
  else if (s == "starTrekBeep") BeeperFX::starTrekBeep();
  else if (s == "waveSweep") BeeperFX::waveSweep();
  else if (s == "powerUp") BeeperFX::powerUp();
  else if (s == "airwolfTheme") BeeperFX::airwolfTheme();
  else if (s == "aTeamTheme") BeeperFX::aTeamTheme();
  else {
    server.send(400, "text/plain", "Unbekannter Sound");
    return;
  }

  server.send(200, "text/plain", "Sound abgespielt: " + s);
}

void handleSetSound() {
  if (!server.hasArg("enabled")) {
    server.send(400, "text/plain", "Fehlender Parameter");
    return;
  }

  String enabled = server.arg("enabled");
  isSoundEnabled = (enabled == "1");

  // Direkt speichern
  SettingsConfig settings;
  settings.isSoundEnabled = isSoundEnabled;
  saveSettingsConfig(settings);

  server.send(200, "text/plain", "Sound " + String(isSoundEnabled ? "aktiviert" : "deaktiviert"));
}

void handleResetWifi() {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0); // alles auf 0 setzen
  }
  EEPROM.commit();
  EEPROM.end();

  isWifiConfigured = false;
  ssid = "";
  password = "";

  server.send(200, "text/html", "<h3>WLAN-Daten gelöscht!</h3><p>Starte neu...</p>");

  delay(2000);
  ESP.restart();
}

void handleRoot() {
  String checkedAttr = isSoundEnabled ? "checked" : "";
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>JontesBox</title>
      <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    </head>
    <body class="bg-light">
      <div class="container py-4">
        <h1 class="text-center mb-4">JontesBox Steuerung</h1>
        <div class="alert alert-info text-center mb-3" id="wifiDisplay">
          Lade WLAN-Status...
        </div>

        <div id="proximityDisplay" class="alert alert-success text-center mb-3">
          🟢 Alles frei
        </div>

        <div class="alert alert-secondary text-center mb-3" id="runStatus">
          Aktueller Modus: <strong><span id="currentMode">–</span></strong><br>
          Letzte Aktion: <span id="lastAction">–</span>
        </div>
        <div class="card mb-3">
          <div class="card-body">
            <h5 class="card-title">Proximity-Wert</h5>
            <p class="card-text fs-4 text-center">
              <span id="proximityValue">...</span>
            </p>
          </div>
        </div>

        <div class="card mb-3">
          <div class="card-body">
            <label for="lidSlider" class="form-label">Deckel-Servo</label>
            <input type="range" class="form-range" min="0" max="180" id="lidSlider" onchange="updateServo('lid', this.value)">
            <button class="btn btn-secondary mt-2 w-100" onclick="resetServo('lid')">Deckel zurücksetzen</button>
          </div>
        </div>

        <div class="card mb-3">
          <div class="card-body">
            <label for="switchSlider" class="form-label">Schalter-Servo</label>
            <input type="range" class="form-range" min="0" max="180" id="switchSlider" onchange="updateServo('switch', this.value)">
            <button class="btn btn-secondary mt-2 w-100" onclick="resetServo('switch')">Schalter zurücksetzen</button>
          </div>
        </div>

        <div class="card mb-3">
          <div class="card-body">
            <h5 class="card-title">Sound aktivieren</h5>
            <div class="form-check form-switch">
              <input class="form-check-input" type="checkbox" id="soundToggle" %CHECKED% onchange="toggleSound(this)">
              <label class="form-check-label" for="soundToggle">Ton</label>
            </div>
          </div>
        </div>

        <div class="card mb-3">
          <div class="card-body">
            <label for="soundSelect" class="form-label">Soundeffekte</label>
            <select class="form-select mb-2" id="soundSelect">
              <option value="startup">Startup</option>
              <option value="success">Erfolg</option>
              <option value="error">Fehler</option>
              <option value="r2d2v1">R2D2 V1</option>
              <option value="r2d2v2">R2D2 V2</option>
              <option value="imperialshort">Imperial March (kurz)</option>
              <option value="imperiallong">Imperial March (lang)</option>
              <option value="nokiaTune">Nokia</option>
              <option value="tetrisIntro">Tetris</option>
              <option value="starTrekBeep">StarTrek</option>
              <option value="waveSweep">Wave Sweep</option>
              <option value="powerUp">Power Up</option>
              <option value="airwolfTheme">AirWolf</option>
              <option value="aTeamTheme">A-Team</option>
            </select>
            <button class="btn btn-warning w-100" onclick="playSound()">▶️ Abspielen</button>
          </div>
        </div>

        <div class="d-grid">
          <button class="btn btn-primary" onclick="window.location.href='/config'">WLAN konfigurieren</button>
        </div>
      </div>

      <script>
        function updateProximity() {
          fetch('/status')
            .then(res => res.json())
            .then(data => {
              document.getElementById('proximityValue').innerText = data.proximity;
              document.getElementById('currentMode').innerText = data.currentMode;
              document.getElementById('lastAction').innerText = data.lastAction;

              const proximityState = document.getElementById('proximityDisplay');
              if (data.proximity > 100) {
                proximityState.innerText = "🟥 Objekt erkannt!";
                proximityState.className = "alert alert-danger text-center";
              } else {
                proximityState.innerText = "🟢 Alles frei";
                proximityState.className = "alert alert-success text-center";
              }
            });
        }

        function updateWifiStatus() {
          fetch('/wifiStatus')
            .then(res => res.json())
            .then(data => {
              const statusBox = document.getElementById('wifiDisplay');
              statusBox.innerHTML = `
                WLAN: <strong>${data.ssid}</strong><br>
                Modus: ${data.mode}<br>
                IP: ${data.ip}
              `;
            });
        }

        function updateServo(target, value) {
          fetch(`/setServo?target=${target}&value=${value}`);
        }

        function resetServo(target) {
          fetch(`/resetServo?target=${target}`)
            .then(() => {
              if (target === "lid") {
                document.getElementById("lidSlider").value = 90; // <- LID_START_POSITION
              } else if (target === "switch") {
                document.getElementById("switchSlider").value = 90; // <- SWITCH_START_POSITION
              }
            });
        }

        function playSound() {
          const sound = document.getElementById("soundSelect").value;
          fetch(`/playSound?sound=${sound}`);
        }

        function toggleSound(element) {
          const isEnabled = element.checked ? 1 : 0;
          fetch(`/setSound?enabled=${isEnabled}`);
        }

        setInterval(updateProximity, 1000);
        updateWifiStatus();
      </script>
    </body>
    </html>
  )rawliteral";

  html.replace("%CHECKED%", checkedAttr);
  server.send(200, "text/html", html);
}

void handleSetWifi() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    isWifiConfigured = true;

    WifiConfig cfg;
    ssid.toCharArray(cfg.ssid, sizeof(cfg.ssid));
    password.toCharArray(cfg.password, sizeof(cfg.password));
    saveWifiConfig(cfg);

    Serial.println("Neue WLAN-Daten gespeichert.");
    server.send(200, "text/html", "<meta charset='utf-8'><h3>Gespeichert. Starte Verbindung...</h3><a href='/'>Zurück</a>");
  } else {
    server.send(400, "text/html", "<meta charset='utf-8'><h3>Fehlende Eingaben!</h3><a href='/config'>Zurück</a>");
  }
}
void handleProximity() {
  uint8_t value = sensor.getProximity();
  server.send(200, "text/plain", String(value));
}

void handleSetServo() {
  if (!server.hasArg("target") || !server.hasArg("value")) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }

  String target = server.arg("target");
  int value = server.arg("value").toInt();
  Serial.println(value);
  if (target == "lid") {
    lidServo.moveNowTo(value);
  } else if (target == "switch") {
    switchServo.moveNowTo(value);
  }

  server.send(200, "text/plain", "OK");
}

void handleResetServo() {
  if (!server.hasArg("target")) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }

  String target = server.arg("target");

  if (target == "lid") {
    lidServo.moveNowTo(LID_START_POSITION);
  } else if (target == "switch") {
    switchServo.moveNowTo(SWITCH_START_POSITION);
  }

  server.send(200, "text/plain", "Reset OK");
}

void handleStatus() {
  String json = "{";
  json += "\"proximity\":" + String(sensor.getProximity()) + ",";
  json += "\"lastAction\":\"" + lastServoAction + "\",";
  json += "\"currentMode\":\"" + currentRunMode + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleConfigPage() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>WLAN konfigurieren</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <meta charset="utf-8">
      <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    </head>
    <body class="bg-light">
      <div class="container py-5">
        <h2 class="text-center mb-4">WLAN konfigurieren</h2>
        <form method="POST" action="/setWifi">
          <div class="mb-3">
            <label for="ssid" class="form-label">SSID</label>
            <input type="text" class="form-control" id="ssid" name="ssid" required>
          </div>
          <div class="mb-3">
            <label for="password" class="form-label">Passwort</label>
            <input type="password" class="form-control" id="password" name="password" required>
          </div>
          <button type="submit" class="btn btn-primary w-100">Speichern & Verbinden</button>
        </form>
        <div class="text-center mt-3">
          <a href="/" class="btn btn-secondary">Zurück</a>
        </div>
      </div>
      <form method="POST" action="/resetWifi">
        <button type="submit" class="btn btn-danger w-100 mt-3">WLAN-Daten zurücksetzen</button>
      </form>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleWifiStatus() {
  String ip = (WiFi.getMode() == WIFI_STA) ? WiFi.localIP().toString()
                                           : WiFi.softAPIP().toString();

  String json = "{";
  json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
  json += "\"ip\":\"" + ip + "\",";
  json += "\"mode\":\"" + String(WiFi.getMode() == WIFI_STA ? "STA" : "AP") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void resetWifiConfig() {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
  EEPROM.commit();
  EEPROM.end();
  server.send(200, "text/html", "<meta charset='utf-8'><h3>WLAN-Daten gelöscht</h3><a href='/'>Zurück</a>");
  delay(1000);
  ESP.restart(); // Neustart
}

void run() {
  Serial.println(playCount);
  switch (playCount % 10) {
    case 0:
    case 1:
      runSlow();
      currentRunMode = "runSlow";
      lastServoAction = "Deckel 1x geklatscht";
      break;
    case 2:
      runWaitThenFast();
      currentRunMode = "runWaitThenFast";
      lastServoAction = "Deckel 4× geklatscht";
      break;
    case 3:
      runFast();
      currentRunMode = "runFast";
      lastServoAction = "Deckel 4× geklatscht";
      break;
    case 4:
      runFastThenClap();
      monitorSensor = true;
      currentRunMode = "runFastThenClap";
      break;
    case 5:
      runOpenCloseThenFast();
      monitorSensor = false;
      currentRunMode = "runOpenCloseThenFast";
      break;
    case 6:
      runPeekThenFast();
      currentRunMode = "runPeekThenFast";
      break;
    case 7:
      runFastWithDelay();
      monitorSensor = true;
      currentRunMode = "runFastWithDelay";
      break;
    case 8:
      runClap();
      monitorSensor = false;
      currentRunMode = "runClap";
      break;
    case 9:
      runHalf();
      currentRunMode = "runHalf";
      break;
    default:
      break;
  }
  Serial.println(currentRunMode);
  playCount++;
}

void runSlow() {
  //BeeperFX::imperialMarchLong();
  openLidSlow();
  flipSwitchSlow();
  closeLidSlow();
  if (isSoundEnabled) {
    BeeperFX::tetrisIntro();
  }
}

void runWaitThenFast() {
  delay(5000);
  flipSwitchFast();
  if (isSoundEnabled) {
    BeeperFX::imperialMarchShort();
  }
}

void runFast() {
  flipSwitchFast();
}

void runFastThenClap() {
  flipSwitchFast();
  clapLid();
}

void runOpenCloseThenFast() {
  openLidSlow();
  delay(2000);
  closeLidSlow();
  delay(2000);
  flipSwitchFast();
}

void runPeekThenFast() {
  switchServo.moveSlowTo(SWITCH_HALF_POSITION);
  delay(3000);
  switchServo.moveFastTo(SWITCH_START_POSITION);
  delay(3000);
  flipSwitchFast();
}

void runFastWithDelay() {
  openLidSlow();
  delay(4000);
  flipSwitchFast();
  closeLidFast();
}

void runClap() {
  if (isSoundEnabled) {
    BeeperFX::r2d2_v2();
  }
  clapLid();
  clapLid();
  clapLid();
  clapLid();
  openLidFast();
  flipSwitchFast();
  closeLidFast();
}

void runHalf() {
  switchServo.moveSlowTo(SWITCH_HALF_POSITION);
  delay(3000);
  switchServo.moveFastTo(SWITCH_END_POSITION);
  switchServo.moveFastTo(SWITCH_START_POSITION);
}

void openLidSlow() {
  lidServo.moveSlowTo(LID_END_POSITION);
}

void openLidFast() {
  lidServo.moveFastTo(LID_END_POSITION);
}

void closeLidSlow() {
  lidServo.moveSlowTo(LID_START_POSITION);
}

void closeLidFast() {
  lidServo.moveFastTo(LID_START_POSITION);
}

void clapLid() {
  openLidFast();
  closeLidFast();
}

void flipSwitchSlow() {
  switchServo.moveSlowTo(SWITCH_END_POSITION);
  switchServo.moveSlowTo(SWITCH_START_POSITION);
}

void flipSwitchFast() {
  switchServo.moveFastTo(SWITCH_END_POSITION);
  switchServo.moveFastTo(SWITCH_START_POSITION);
}
