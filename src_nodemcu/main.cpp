#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

namespace {
constexpr uint8_t LED_PIN_5 = D5;
constexpr uint8_t LED_PIN_6 = D6;
constexpr uint8_t LED_PIN_7 = D7;
constexpr char API_TOKEN[] = "123";

ESP8266WebServer server(80);
WiFiManager wm;
bool led5On = false;
bool led6On = false;
bool led7On = false;

bool isAuthorized() {
    String authorization = server.header("Authorization");
    return authorization == String("Bearer ") + API_TOKEN;
}

void sendUnauthorized() {
    Serial.println("[AUTH] Akses ditolak: Token tidak valid atau kosong.");
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
}

void handleControlPage() {
    Serial.println("[HTTP] Mengirim halaman kendali (GET /control)");
    static const char page[] PROGMEM = R"HTML(
<!doctype html><html lang="id"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP8266 LED Control</title>
<style>
body{font-family:system-ui,sans-serif;max-width:32rem;margin:3rem auto;padding:0 1rem;background:#eef2f3;color:#172126}
main{background:#fff;padding:2rem;border-radius:12px;box-shadow:0 8px 24px #17212622}
input[type=password]{font:inherit;padding:.7rem;margin:.25rem 0;border:1px solid #b7c2c5;border-radius:6px;box-sizing:border-box;width:100%}
.control-group{margin:1.2rem 0;display:flex;flex-direction:column;gap:1rem}
.row{display:flex;justify-content:space-between;align-items:center;padding:.8rem;background:#f7f9fa;border-radius:8px;border:1px solid #e1e8ed}
span{font-weight:600}
button{font:inherit;padding:.6rem 1rem;border:0;border-radius:6px;cursor:pointer;color:#fff;font-weight:600;min-width:5rem}
.btn-on{background:#28a745}.btn-off{background:#dc3545}
#status{margin:1rem 0;font-weight:600}
</style></head><body><main><h1>ESP8266 LED Control</h1>
<label>API token<input id="token" type="password" autocomplete="off"></label>
<div class="control-group">
<div class="row"><span>LED Pin 5 (D5)</span><div><button class="btn-on" onclick="setLed(5,true)">Nyalakan</button><button class="btn-off" onclick="setLed(5,false)">Matikan</button></div></div>
<div class="row"><span>LED Pin 6 (D6)</span><div><button class="btn-on" onclick="setLed(6,true)">Nyalakan</button><button class="btn-off" onclick="setLed(6,false)">Matikan</button></div></div>
<div class="row"><span>LED Pin 7 (D7)</span><div><button class="btn-on" onclick="setLed(7,true)">Nyalakan</button><button class="btn-off" onclick="setLed(7,false)">Matikan</button></div></div>
</div>
<p id="status">Status belum dibaca.</p>
<script>
const token=document.querySelector('#token');
async function request(path,options={}){options.headers={...(options.headers||{}),Authorization:'Bearer '+token.value};const r=await fetch(path,options);if(!r.ok)throw Error(await r.text());return r.json()}
async function refresh(){try{const s=await request('/api/status');document.querySelector('#status').textContent=`Wi-Fi: ${s.ip} | LED 5: ${s.led5?'ON':'OFF'}, LED 6: ${s.led6?'ON':'OFF'}, LED 7: ${s.led7?'ON':'OFF'}`}catch(e){document.querySelector('#status').textContent='Token salah atau ESP8266 tidak dapat dihubungi.'}}
async function setLed(pin,value){try{const res=await request('/api/led',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({pin:pin,on:value})});refresh()}catch(e){alert('Perintah ditolak. Periksa token.');refresh()}}
token.addEventListener('change',refresh);
refresh();
</script></main></body></html>)HTML";
    server.send(200, "text/html; charset=utf-8", page);
}

void handleStatus() {
    Serial.println("[HTTP] Permintaan status diterima (GET /api/status)");
    if (!isAuthorized()) { sendUnauthorized(); return; }
    String json = String("{\"led5\":") + (led5On ? "true" : "false") +
                  ",\"led6\":" + (led6On ? "true" : "false") +
                  ",\"led7\":" + (led7On ? "true" : "false") +
                  ",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
    server.send(200, "application/json", json);
}

void handleLed() {
    Serial.println("[HTTP] Permintaan ubah LED diterima (POST /api/led)");
    if (!isAuthorized()) { sendUnauthorized(); return; }
    if (!server.hasArg("plain")) {
        Serial.println("[ERROR] Body JSON kosong.");
        server.send(400, "application/json", "{\"error\":\"JSON body required\"}");
        return;
    }

    String body = server.arg("plain");
    Serial.print("[DATA] Payload: ");
    Serial.println(body);

    int pin = 0;
    if (body.indexOf("\"pin\":5") >= 0) pin = 5;
    else if (body.indexOf("\"pin\":6") >= 0) pin = 6;
    else if (body.indexOf("\"pin\":7") >= 0) pin = 7;
    else {
        Serial.println("[ERROR] Pin tidak dikenali.");
        server.send(400, "application/json", "{\"error\":\"invalid pin\"}");
        return;
    }

    bool state = false;
    if (body.indexOf("\"on\":true") >= 0) state = true;
    else if (body.indexOf("\"on\":false") >= 0) state = false;
    else {
        Serial.println("[ERROR] Status 'on' harus boolean.");
        server.send(400, "application/json", "{\"error\":\"on must be boolean\"}");
        return;
    }

    if (pin == 5) {
        led5On = state;
        analogWrite(LED_PIN_5, led5On ? 500 : 0);
        Serial.printf("[LED] Pin 5 (D5) diatur ke: %s (PWM 500)\n", led5On ? "ON" : "OFF");
    } else if (pin == 6) {
        led6On = state;
        analogWrite(LED_PIN_6, led6On ? 500 : 0);
        Serial.printf("[LED] Pin 6 (D6) diatur ke: %s (PWM 500)\n", led6On ? "ON" : "OFF");
    } else if (pin == 7) {
        led7On = state;
        analogWrite(LED_PIN_7, led7On ? 500 : 0);
        Serial.printf("[LED] Pin 7 (D7) diatur ke: %s (PWM 500)\n", led7On ? "ON" : "OFF");
    }

    String json = String("{\"led5\":") + (led5On ? "true" : "false") +
                  ",\"led6\":" + (led6On ? "true" : "false") +
                  ",\"led7\":" + (led7On ? "true" : "false") + "}";
    server.send(200, "application/json", json);
}
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[SYSTEM] Memulai ESP8266...");

    pinMode(LED_PIN_5, OUTPUT);
    pinMode(LED_PIN_6, OUTPUT);
    pinMode(LED_PIN_7, OUTPUT);
    analogWrite(LED_PIN_5, 0);
    analogWrite(LED_PIN_6, 0);
    analogWrite(LED_PIN_7, 0);

    const char* headerKeys[] = {"Authorization"};
    server.collectHeaders(headerKeys, 1);

    server.on("/control", HTTP_GET, handleControlPage);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/led", HTTP_POST, handleLed);

    Serial.println("[WIFI] Menghubungkan atau membuka portal WiFiManager...");
    bool res = wm.autoConnect("ESP8266-Setup", "esp8266setup");

    if (res) {
        Serial.print("[WIFI] Terhubung! Halaman kontrol dapat diakses di: http://");
        Serial.print(WiFi.localIP());
        Serial.println("/control");
        server.begin();
    } else {
        Serial.println("[WIFI] Gagal terhubung ke WiFi, melakukan restart...");
        delay(3000);
        ESP.restart();
    }
}

void loop() {
    server.handleClient();
}
