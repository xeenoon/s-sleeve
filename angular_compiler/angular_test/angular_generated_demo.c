#include <stdlib.h>
#include <stdio.h>
#include "angular_http_service.h"
#include "angular_mapPercentToAngle.h"
#include "angular_ngOnInit.h"
#include "angular_updateKnee.h"
#include "helpers/include/net/http_server.h"
#include "helpers/include/net/http_service.h"

static const char *g_index_html =
  "<!doctype html>\n"
  "<html lang=\"en\">\n"
  "<head>\n"
  "  <meta charset=\"utf-8\">\n"
  "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
  "  <title>Knee Position Monitor</title>\n"
  "  <link rel=\"stylesheet\" href=\"/styles.css\">\n"
  "</head>\n"
  "<body>\n"
  "  <main>\n"
  "    <div class=\"layout\">\n"
  "      <div class=\"meter\">\n"
  "        <p>Knee Position</p>\n"
  "        <svg viewBox=\"0 0 280 320\" aria-label=\"Knee joint animation\">\n"
  "          <line class=\"guide\" x1=\"140\" y1=\"64\" x2=\"140\" y2=\"174\"></line>\n"
  "          <g id=\"leg\">\n"
  "            <line class=\"limb\" x1=\"140\" y1=\"64\" x2=\"140\" y2=\"174\"></line>\n"
  "            <line class=\"limb\" id=\"lower-leg\" x1=\"140\" y1=\"174\" x2=\"140.0\" y2=\"274.0\"></line>\n"
  "            <circle class=\"joint\" cx=\"140\" cy=\"64\" r=\"14\"></circle>\n"
  "            <circle class=\"joint\" cx=\"140\" cy=\"174\" r=\"16\"></circle>\n"
  "            <circle class=\"joint\" id=\"ankle\" cx=\"140.0\" cy=\"274.0\" r=\"14\"></circle>\n"
  "          </g>\n"
  "        </svg>\n"
  "        <div class=\"joint-labels\">\n"
  "          <span>Bent</span>\n"
  "          <span>Straight</span>\n"
  "        </div>\n"
  "      </div>\n"
  "      <div>\n"
  "        <p>Potentiometer Reading</p>\n"
  "        <h1 id=\"reading\">3500</h1>\n"
  "        <div class=\"subtext\" id=\"angle-text\">67% straight</div>\n"
  "      </div>\n"
  "    </div>\n"
  "  </main>\n"
  "  <script src=\"/app.js\"></script>\n"
  "</body>\n"
  "</html>\n";

static const char *g_styles_css =
  ":root {\n"
  "  color-scheme: light;\n"
  "  --bg: #f6efe5;\n"
  "  --text: #241a14;\n"
  "  --accent: #d66b2d;\n"
  "  --panel: rgba(255, 250, 244, 0.88);\n"
  "}\n"
  "* { box-sizing: border-box; }\n"
  "body { margin: 0; min-height: 100vh; display: grid; place-items: center; background: radial-gradient(circle at top, #ffd7bb 0%, transparent 35%), linear-gradient(160deg, var(--bg), #f0dcc5); color: var(--text); font-family: Arial, sans-serif; }\n"
  "main { width: min(92vw, 800px); padding: 3rem 2rem; text-align: center; background: var(--panel); border: 2px solid rgba(36, 26, 20, 0.08); border-radius: 28px; box-shadow: 0 24px 60px rgba(0, 0, 0, 0.12); }\n"
  "p { margin: 0 0 1rem; font-size: 1rem; letter-spacing: 0.08em; text-transform: uppercase; color: #715548; }\n"
  "h1 { margin: 0.2rem 0 0; font-size: clamp(3.2rem, 12vw, 6rem); line-height: 1; color: var(--accent); }\n"
  ".layout { display: grid; gap: 2rem; align-items: center; justify-items: center; }\n"
  ".meter { width: min(100%, 420px); padding: 1rem; border-radius: 24px; background: rgba(255, 255, 255, 0.55); }\n"
  ".subtext { margin-top: 0.5rem; font-size: 0.95rem; color: #715548; }\n"
  ".joint-labels { display: flex; justify-content: space-between; gap: 1rem; margin-top: 0.8rem; font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.06em; color: #715548; }\n"
  "svg { width: min(100%, 360px); height: auto; overflow: visible; }\n"
  ".limb { fill: none; stroke: var(--accent); stroke-linecap: round; stroke-width: 24; }\n"
  ".joint { fill: var(--text); }\n"
  ".guide { fill: none; stroke: rgba(36, 26, 20, 0.12); stroke-width: 12; stroke-linecap: round; stroke-dasharray: 10 14; }\n";

static const char *g_app_js =
  "(function () {\n"
  "  const readingEl = document.getElementById('reading');\n"
  "  const angleTextEl = document.getElementById('angle-text');\n"
  "  const lowerLegEl = document.getElementById('lower-leg');\n"
  "  const ankleEl = document.getElementById('ankle');\n"
  "  function applyState(state) {\n"
  "    readingEl.textContent = String(state.reading);\n"
  "    angleTextEl.textContent = state.hasSignal ? String(state.percentStraight) + '% straight' : 'Signal error';\n"
  "    lowerLegEl.setAttribute('x2', state.lowerLegX2);\n"
  "    lowerLegEl.setAttribute('y2', state.lowerLegY2);\n"
  "    ankleEl.setAttribute('cx', state.ankleX);\n"
  "    ankleEl.setAttribute('cy', state.ankleY);\n"
  "  }\n"
  "  function refresh() {\n"
  "    fetch('/state')\n"
  "      .then(function (response) { return response.json(); })\n"
  "      .then(applyState)\n"
  "      .catch(function () { angleTextEl.textContent = 'Signal error'; });\n"
  "  }\n"
  "  refresh();\n"
  "  window.setInterval(refresh, 1000);\n"
  "}());\n";

int main(int argc, char **argv) {
  ng_runtime_t runtime;
  angular_http_service_t service;
  unsigned short port = 18080;
  int max_requests = 0;

  if (argc >= 2) {
    port = (unsigned short)atoi(argv[1]);
  }
  if (argc >= 3) {
    max_requests = atoi(argv[2]);
  }

  angular_ngOnInit_call(&runtime);
  ng_runtime_set_int(&runtime, "reading", 3500);
  ng_runtime_set_int(&runtime, "percentStraight", 67);
  ng_runtime_set_double(&runtime, "angleDeg", angular_mapPercentToAngle_call(&runtime, 67.0));
  ng_runtime_set_bool(&runtime, "hasSignal", 1);
  angular_updateKnee_call(&runtime, 67.0);
  printf("[demo] starting port=%u max_requests=%d reading=%d\n", port, max_requests, ng_runtime_get_int(&runtime, "reading", 0));
  fflush(stdout);
  angular_http_service_init(&service, &runtime, g_index_html, g_styles_css, g_app_js);
  return ng_http_server_serve(port, ng_http_service_handle, &service.service, max_requests);
}
