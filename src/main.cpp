#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// On a standard ESP32, GPIO23 is not ADC-capable.
// Move the potentiometer signal wire to an ADC pin like 34, 35, 32, or 33.
constexpr int POT_PIN = 34;

const char *AP_SSID = "ESP32-Pot-Reader";

WebServer server(80);

const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Potentiometer</title>
  <style>
    :root {
      color-scheme: light;
      --bg: #f6efe5;
      --text: #241a14;
      --accent: #d66b2d;
      --accent-soft: #f2b488;
      --panel: rgba(255, 250, 244, 0.88);
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      min-height: 100vh;
      display: grid;
      place-items: center;
      background:
        radial-gradient(circle at top, #ffd7bb 0%, transparent 35%),
        linear-gradient(160deg, var(--bg), #f0dcc5);
      color: var(--text);
      font-family: Arial, sans-serif;
    }

    main {
      width: min(92vw, 800px);
      padding: 3rem 2rem;
      text-align: center;
      background: var(--panel);
      border: 2px solid rgba(36, 26, 20, 0.08);
      border-radius: 28px;
      box-shadow: 0 24px 60px rgba(0, 0, 0, 0.12);
      backdrop-filter: blur(10px);
    }

    p {
      margin: 0 0 1rem;
      font-size: 1rem;
      letter-spacing: 0.08em;
      text-transform: uppercase;
      color: #715548;
    }

    h1 {
      margin: 0.2rem 0 0;
      font-size: clamp(3.2rem, 12vw, 6rem);
      line-height: 1;
      color: var(--accent);
    }

    .layout {
      display: grid;
      gap: 2rem;
      align-items: center;
      justify-items: center;
    }

    .meter {
      width: min(100%, 420px);
      padding: 1rem;
      border-radius: 24px;
      background: rgba(255, 255, 255, 0.55);
    }

    .subtext {
      margin-top: 0.5rem;
      font-size: 0.95rem;
      color: #715548;
    }

    .joint-labels {
      display: flex;
      justify-content: space-between;
      gap: 1rem;
      margin-top: 0.8rem;
      font-size: 0.85rem;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      color: #715548;
    }

    svg {
      width: min(100%, 360px);
      height: auto;
      overflow: visible;
    }

    .limb {
      fill: none;
      stroke: var(--accent);
      stroke-linecap: round;
      stroke-width: 24;
    }

    .joint {
      fill: var(--text);
    }

    .guide {
      fill: none;
      stroke: rgba(36, 26, 20, 0.12);
      stroke-width: 12;
      stroke-linecap: round;
      stroke-dasharray: 10 14;
    }
  </style>
</head>
<body>
  <main>
    <div class="layout">
      <div class="meter">
        <p>Knee Position</p>
        <svg viewBox="0 0 280 320" aria-label="Knee joint animation">
          <line class="guide" x1="140" y1="64" x2="140" y2="174"></line>
          <g id="leg">
            <line class="limb" x1="140" y1="64" x2="140" y2="174"></line>
            <line class="limb" id="lower-leg" x1="140" y1="174" x2="140" y2="274"></line>
            <circle class="joint" cx="140" cy="64" r="14"></circle>
            <circle class="joint" cx="140" cy="174" r="16"></circle>
            <circle class="joint" id="ankle" cx="140" cy="274" r="14"></circle>
          </g>
        </svg>
        <div class="joint-labels">
          <span>Bent</span>
          <span>Straight</span>
        </div>
      </div>

      <div>
        <p>Potentiometer Reading</p>
        <h1 id="reading">0</h1>
        <div class="subtext" id="angle-text">0% straight</div>
      </div>
    </div>
  </main>

  <script>
    const MIN_READING = 3100;
    const MAX_READING = 3700;
    const BENT_ANGLE = 78;
    const STRAIGHT_ANGLE = 0;

    function clamp(value, min, max) {
      return Math.min(max, Math.max(min, value));
    }

    function mapReadingToAngle(reading) {
      const normalized = clamp((reading - MIN_READING) / (MAX_READING - MIN_READING), 0, 1);
      const angle = BENT_ANGLE + (STRAIGHT_ANGLE - BENT_ANGLE) * normalized;
      return { normalized, angle };
    }

    function updateKnee(reading) {
      const lowerLeg = document.getElementById("lower-leg");
      const ankle = document.getElementById("ankle");
      const angleText = document.getElementById("angle-text");
      const { normalized, angle } = mapReadingToAngle(reading);
      const radians = angle * Math.PI / 180;
      const kneeX = 140;
      const kneeY = 174;
      const shinLength = 100;
      const ankleX = kneeX + Math.sin(radians) * shinLength;
      const ankleY = kneeY + Math.cos(radians) * shinLength;

      lowerLeg.setAttribute("x1", kneeX);
      lowerLeg.setAttribute("y1", kneeY);
      lowerLeg.setAttribute("x2", ankleX.toFixed(1));
      lowerLeg.setAttribute("y2", ankleY.toFixed(1));
      ankle.setAttribute("cx", ankleX.toFixed(1));
      ankle.setAttribute("cy", ankleY.toFixed(1));
      angleText.textContent = `${Math.round(normalized * 100)}% straight`;
    }

    async function refreshReading() {
      try {
        const response = await fetch("/value");
        const value = Number(await response.text());
        document.getElementById("reading").textContent = value;
        updateKnee(value);
      } catch (error) {
        document.getElementById("reading").textContent = "ERR";
        document.getElementById("angle-text").textContent = "Signal error";
      }
    }

    updateKnee(MIN_READING);
    refreshReading();
    setInterval(refreshReading, 500);
  </script>
</body>
</html>
)HTML";

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleValue() {
  server.send(200, "text/plain", String(analogRead(POT_PIN)));
}

void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12);
  pinMode(POT_PIN, INPUT);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);

  server.on("/", handleRoot);
  server.on("/value", handleValue);
  server.begin();

  Serial.println();
  Serial.println("ESP32 potentiometer web server started.");
  Serial.print("Connect to Wi-Fi: ");
  Serial.println(AP_SSID);
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}
