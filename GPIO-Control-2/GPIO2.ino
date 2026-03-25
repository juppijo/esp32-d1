/*
 * ============================================================
 * ESP32-S3 GPIO & RGB Master (Kombinierte Version)
 * ============================================================
 * Hardware: ESP32-S3-WROOM-1 (N16R8)
 * Features: GPIO Control, PWM, Digital Read, NeoPixel RGB (Pin 48)
 * Inklusive: mDNS, CORS, GitHub-GUI Loader
 * ============================================================
*  Libraries : ESPAsyncWebServer, AsyncTCP, ESPmDNS
 *
 *  Endpunkte:
 *    GET /            → Shell-Seite (lädt GUI von GitHub)
 *    GET /set?pin=&state=   → Digital-Output (0 oder 1)
 *    GET /pwm?pin=&duty=    → PWM-Output  (0–255)
 *    GET /read?pin=         → Digital-Input lesen
 *    GET /status            → JSON aller Pin-Zustände
 *    GET /info              → JSON mit System-Infos
 *    GET /LED
 * ============================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>
#include "SecretLogin.h" // Muss ssid, password, GITHUB_URL und HOSTNAME enthalten

// ── NeoPixel Konfiguration (Onboard LED) ────────────────────
#define RGB_PIN 48 
Adafruit_NeoPixel pixels(1, RGB_PIN, NEO_GRB + NEO_KHZ800); 

// ── Webserver & GPIO ────────────────────────────────────────
AsyncWebServer server(80);

struct PinCfg {
  int  pin;
  bool isOutput;
  const char* label;
};

// Kombinierte Pin-Liste 
PinCfg PINS[] = {
  {  2, true,  "LED / Relais 1"  },
  {  4, true,  "Relais 2"        },
  {  5, true,  "Relais 3"        },
  { 12, true,  "Relais 4"        },
  { 13, true,  "PWM Kanal 1"     },
  { 14, true,  "PWM Kanal 2"     },
  { 36, false, "Sensor 1"        },
  { 37, false, "Sensor 2"        },
  { 38, false, "Sensor 3"        }
};
const int PIN_COUNT = sizeof(PINS) / sizeof(PINS[0]);

// ── Hilfsfunktionen ──────────────────────────────────────────

PinCfg* findPin(int pin) {
  for (int i = 0; i < PIN_COUNT; i++)
    if (PINS[i].pin == pin) return &PINS[i];
  return nullptr;
}
// CORS-Header zu jeder Antwort hinzufügen

void addCORS(AsyncWebServerResponse* r) {
  r->addHeader("Access-Control-Allow-Origin",  "*");
  r->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  r->addHeader("Access-Control-Allow-Headers", "Content-Type"); 
}

void sendOK(AsyncWebServerRequest* req, const String& body, const char* ct = "application/json") {
  AsyncWebServerResponse* r = req->beginResponse(200, ct, body);
  addCORS(r);
  req->send(r); 
}

void sendErr(AsyncWebServerRequest* req, int code, const String& msg) {
  AsyncWebServerResponse* r = req->beginResponse(code, "text/plain", msg);
  addCORS(r);
  req->send(r);
}

// ── Shell-HTML (Lädt das Interface von GitHub) ──────────────
// Wichtig: innerHTML führt <script>-Tags NICHT aus.
// Lösung: DOMParser + manuelles Script-Einfügen nach dem Laden.

const char SHELL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32-S3 Control Center</title>
  <style>
    body{background:#0f172a;color:#f8fafc;font-family:sans-serif;display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:100vh;margin:0}
    #loader{text-align:center;color:#38bdf8}
    .spin{width:48px;height:48px;border:4px solid #1e293b;border-top:4px solid #38bdf8;border-radius:50%;animation:s 1s linear infinite;margin:0 auto 16px}
    @keyframes s{to{transform:rotate(360deg)}}
    .fs-btn{position:fixed;top:8px;right:12px;padding:8px 12px;border:none;border-radius:8px;background:rgba(148,163,184,.25);color:#f8fafc;font-size:1.3rem;cursor:pointer;z-index:9999}
  </style>
</head>
<body>
  <button class="fs-btn" onclick="toggleFS()">⛶</button>
  <div id="root"><div id="loader"><div class="spin"></div>Lade Interface...</div></div>
  <script>
    function toggleFS(){ if(!document.fullscreenElement) document.documentElement.requestFullscreen(); else document.exitFullscreen(); }
    fetch('%GITHUB_URL%')
      .then(r => r.text())
      .then(html => {
        const doc = new DOMParser().parseFromString(html, 'text/html');
        document.getElementById('root').innerHTML = doc.body.innerHTML;
        doc.querySelectorAll('script').forEach(old => {
          const s = document.createElement('script');
          if(old.src) s.src = old.src; else s.textContent = old.textContent;
          document.body.appendChild(s);
        });
      });
  </script>
</body>
</html>
)rawliteral"; 

// ── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  
  // NeoPixel initialisieren 
  pixels.begin();
  pixels.setBrightness(30);
  pixels.show(); 

  delay(500);
  Serial.println("\n╔══════════════════════════╗");
  Serial.println(  "║  ESP32-S3 GPIO2 Master   ║");
  Serial.println(  "╚══════════════════════════╝");

  Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("PSRAM Size: %d MB\n", ESP.getPsramSize() / (1024 * 1024));

   // ── Pins initialisieren ──────────────────────────────────
  for (int i = 0; i < PIN_COUNT; i++) {
    if (PINS[i].isOutput) {
      pinMode(PINS[i].pin, OUTPUT);
      digitalWrite(PINS[i].pin, LOW);
      Serial.printf("  OUT  GPIO %-3d  %s\n", PINS[i].pin, PINS[i].label);
    } else {
      pinMode(PINS[i].pin, INPUT);
      Serial.printf("  IN   GPIO %-3d  %s\n", PINS[i].pin, PINS[i].label);
    }
  }

 // ── WLAN verbinden ───────────────────────────────────────
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf("\nVerbinde mit '%s'", ssid);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries++ < 40) {
    delay(500); Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n✗ WLAN-Verbindung fehlgeschlagen!");
    return;
  }
  Serial.println("\n✓ Verbunden!");
  Serial.println("  IP   : http://" + WiFi.localIP().toString());
  Serial.println("  RSSI : " + String(WiFi.RSSI()) + " dBm");

  // ── mDNS ─────────────────────────────────────────────────
  if (MDNS.begin(HOSTNAME)) 
    Serial.println("  mDNS : http://" + String(HOSTNAME) + ".local");

  // ── CORS Preflight (OPTIONS) ─────────────────────────────
  server.on("/*", HTTP_OPTIONS, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* r = req->beginResponse(204);
    addCORS(r);
    req->send(r);
  });

  // --- Webserver Routen ------------------------------------------------------------

  // ── GET / → Shell-Seite ──────────────────────────────────
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    String html = SHELL_HTML;
    html.replace("%GITHUB_URL%", GITHUB_URL);
    AsyncWebServerResponse* r =
      req->beginResponse(200, "text/html; charset=utf-8", html);
    addCORS(r);
    req->send(r);
  });

  // ── GET /set?pin=2&state=1 → Digital-Output ──────────────
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (!req->hasParam("pin") || !req->hasParam("state"))
      return sendErr(req, 400, "Fehlende Parameter: pin, state");

    int pin   = req->getParam("pin")->value().toInt();
    int state = req->getParam("state")->value().toInt();

    PinCfg* p = findPin(pin);
    if (!p)        return sendErr(req, 403, "Pin " + String(pin) + " nicht in Whitelist");
    if (!p->isOutput) return sendErr(req, 403, "Pin " + String(pin) + " ist als Input konfiguriert");

    digitalWrite(pin, state ? HIGH : LOW);
    Serial.printf("SET  GPIO %-3d → %d\n", pin, state);
    sendOK(req, "{\"pin\":" + String(pin) + ",\"state\":" + String(state) + "}", "application/json");
  });

  // ── GET /pwm?pin=13&duty=128 → PWM (0–255) ───────────────
  // Für ESP32-S3: ledcAttach + ledcWrite (Arduino Core ≥ 3.x)
  server.on("/pwm", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (!req->hasParam("pin") || !req->hasParam("duty"))
      return sendErr(req, 400, "Fehlende Parameter: pin, duty");

    int pin  = req->getParam("pin")->value().toInt();
    int duty = constrain(req->getParam("duty")->value().toInt(), 0, 255);

    PinCfg* p = findPin(pin);
    if (!p || !p->isOutput)
      return sendErr(req, 403, "Pin " + String(pin) + " nicht für PWM geeignet");

    // Arduino Core 3.x: ledcAttach(pin, freq, resolution)
    ledcAttach(pin, 5000, 8);   // 5 kHz, 8-Bit (0–255)
    ledcWrite(pin, duty);

    Serial.printf("PWM  GPIO %-3d → %d/255\n", pin, duty);
    sendOK(req, "{\"pin\":" + String(pin) + ",\"duty\":" + String(duty) + "}", "application/json");
  });

  // ── GET /read?pin=36 → Digital-Input ─────────────────────
  server.on("/read", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (!req->hasParam("pin"))
      return sendErr(req, 400, "Fehlender Parameter: pin");

    int pin = req->getParam("pin")->value().toInt();
    PinCfg* p = findPin(pin);
    if (!p)
      return sendErr(req, 403, "Pin " + String(pin) + " nicht in Whitelist");

    int val = digitalRead(pin);
    Serial.printf("READ GPIO %-3d → %d\n", pin, val);
    sendOK(req, "{\"pin\":" + String(pin) + ",\"value\":" + String(val) + "}", "application/json");
  });

  // ── GET /status → JSON aller Pin-Zustände ────────────────
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest* req) {
    String json = "{\"pins\":[";
    for (int i = 0; i < PIN_COUNT; i++) {
      if (i) json += ",";
      json += "{\"pin\":"    + String(PINS[i].pin)               + ","
              "\"label\":\""  + String(PINS[i].label)             + "\","
              "\"output\":"   + (PINS[i].isOutput ? "true" : "false") + ","
              "\"value\":"    + String(digitalRead(PINS[i].pin))  + "}";
    }
    json += "]}";
    sendOK(req, json, "application/json");
  });

  // ── GET /info → System-Informationen ─────────────────────
  server.on("/info", HTTP_GET, [](AsyncWebServerRequest* req) {
    String json = "{"
      "\"ip\":\""     + WiFi.localIP().toString()  + "\","
      "\"hostname\":\"" + String(HOSTNAME) + ".local\","
      "\"rssi\":"     + String(WiFi.RSSI())         + ","
      "\"uptime\":"   + String(millis() / 1000)      + ","
      "\"heap\":"     + String(ESP.getFreeHeap())    + ","
      "\"chip\":\"ESP32-S3\""
    "}";
    sendOK(req, json, "application/json");
  });

  //------------------------------------------------------------------------------------

  // RGB-LED Steuerung 
  server.on("/rgb", HTTP_GET, [](AsyncWebServerRequest *req){
    if (req->hasParam("color")) {
      String color = req->getParam("color")->value();
      if (color == "red")        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      else if (color == "green")  pixels.setPixelColor(0, pixels.Color(0, 255, 0));
      else if (color == "blue")   pixels.setPixelColor(0, pixels.Color(0, 0, 255));
      else if (color == "yellow") pixels.setPixelColor(0, pixels.Color(255, 255, 0));
      else if (color == "white")  pixels.setPixelColor(0, pixels.Color(255, 255, 255));
      else if (color == "magenta")  pixels.setPixelColor(0, pixels.Color(255, 255, 255));
      else if (color == "aqua")  pixels.setPixelColor(0, pixels.Color(0, 255, 255)); 
      else if (color == "blue")   pixels.setPixelColor(0, pixels.Color(0, 0, 255));
      else if (color == "orange")  pixels.setPixelColor(0, pixels.Color(255, 165, 0));
      else if (color == "off")    pixels.setPixelColor(0, pixels.Color(0, 0, 0));

      pixels.show();
      sendOK(req, "{\"rgb\":\"" + color + "\"}");
    }
  });

  server.begin();
  Serial.println("\n✓ Webserver läuft auf Port 80");
  Serial.println("══════════════════════════════***\n");
}

void loop() {
  // Hauptlogik läuft asynchron — loop bleibt leer
}
