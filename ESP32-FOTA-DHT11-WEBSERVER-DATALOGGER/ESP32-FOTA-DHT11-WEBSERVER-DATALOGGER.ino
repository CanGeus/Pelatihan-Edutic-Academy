#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Update.h>

const char *ssid = "webweb";
const char *password = "11112222";

#define DHT_PIN 14
#define DHT_TYPE DHT11
#define RELAY1 25
#define RELAY2 26

bool relayState1 = false, relayState2 = false;

WebServer server(80);
DHT dht(DHT_PIN, DHT_TYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", 25200, 60000);

// Cache pembacaan sensor
float lastTemp = 0, lastHum = 0;
unsigned long lastRead = 0;
const unsigned long sensorInterval = 1000;

// Log data
struct LogData {
  float temperature;
  float humidity;
  String timestamp;
};
const int logSize = 16;
LogData logs[logSize];
int logIndex = 0;

const char* days[] = {"Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"};

String getTimeString() {
  time_t now = timeClient.getEpochTime();
  struct tm *t = localtime(&now);
  char buf[30];
  strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", t);
  return String(days[t->tm_wday]) + ", " + buf;
}

float readTemp() {
  float t = dht.readTemperature();
  return isnan(t) ? lastTemp : t;
}
float readHum() {
  float h = dht.readHumidity();
  return isnan(h) ? lastHum : h;
}

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.println("\nTerhubung: " + WiFi.localIP().toString());

  timeClient.begin();
  if (MDNS.begin("esp32")) Serial.println("MDNS aktif");

  server.on("/", []() {
    static const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="id"><head><meta charset="UTF-8"/><meta name="viewport" content="width=device-width,initial-scale=1.0"/><title>Dashboard</title>
<script src="https://cdn.tailwindcss.com"></script>
<link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;500;700&display=swap" rel="stylesheet">
<style>*{font-family:'Roboto',sans-serif!important}.fade{transition:all .3s ease-in-out}</style></head>
<body class="bg-gray-200 min-h-screen flex flex-col items-center justify-start">
<div class="bg-white min-h-[100svh] max-w-lg w-full text-center shadow-lg relative overflow-hidden">
<nav class="fixed top-0 left-1/2 transform -translate-x-1/2 w-full max-w-lg px-6 py-3 flex justify-between items-center z-50 bg-white shadow-md">
<div class="flex flex-col text-left"><div class="text-xl font-bold">Hello Admin 👋</div><p class="text-sm text-gray-500">Welcome Back!</p></div>
<div class="relative h-full"><img width="30" height="30" src="https://img.icons8.com/ios/50/appointment-reminders--v1.png" alt="Notifikasi"/><div class="absolute top-0 right-0 bg-blue-600 rounded-full w-2.5 h-2.5 border-2 border-white"></div></div></nav>
<main class="flex flex-col gap-5 px-8 pt-24 pb-10 min-h-[1280px] bg-gray-50">
<div class="relative w-full h-[450px] rounded-3xl overflow-hidden">
<div class="absolute inset-0 bg-cover bg-center" style="background-image:url('https://housing.com/news/wp-content/uploads/2023/03/exterior-design-shutterstock_1932966368-1200x700-compressed.jpg');"></div>
<div class="absolute bottom-0 left-0 right-0 bg-gradient-to-t from-blue-800 to-blue-500/70 p-4 text-white mx-5 mb-5 rounded-2xl">
<div class="flex justify-between items-center">
<div class="text-left"><h2 class="text-lg font-semibold">Device 1</h2><p class="text-sm opacity-90">Realtime Update</p></div>
<div><a href="/download" class="py-2 px-4 bg-white text-blue-600 font-bold rounded-2xl">Unduh Data</a></div></div><hr class="my-4">
<div class="flex justify-around items-center">
<div class="flex gap-3"><div class="p-3 bg-white rounded-full"><img src="https://img.icons8.com/ios-filled/50/fa314a/temperature.png" class="w-6 h-6"></div><div class="text-left"><h3 class="text-md font-bold">Temperature</h3><p id="temperature" class="text-sm font-bold">-- °C</p></div></div>
<div class="flex gap-3"><div class="p-3 bg-white rounded-full"><img src="https://img.icons8.com/ios-filled/50/00add6/hygrometer.png" class="w-6 h-6"></div><div class="text-left"><h3 class="text-md font-bold">Humidity</h3><p id="humidity" class="text-sm font-bold">-- %</p></div></div></div></div></div>
<p class="text-xs mt-2 text-gray-500">Data diperbarui setiap 2 detik</p>
<div class="grid grid-cols-2 gap-3">
<div onclick="toggleRelay1()" id="relay1" class="relative w-full h-[150px] rounded-3xl overflow-hidden bg-gradient-to-t from-blue-800 to-blue-500/70 p-4 cursor-pointer">
<div class="absolute inset-0 bg-cover bg-center" style="background-image:url('https://hips.hearstapps.com/hmg-prod/images/cute-room-ideas-1677096334.png');"></div><div id="relay1-status" class="absolute bottom-0 left-0 right-0 bg-gradient-to-t from-black/70 to-black/0 h-1/6"></div></div>
<div onclick="toggleRelay2()" id="relay2" class="relative w-full h-[150px] rounded-3xl overflow-hidden bg-gradient-to-t from-blue-800 to-blue-500/70 p-4 cursor-pointer">
<div class="absolute inset-0 bg-cover bg-center" style="background-image:url('https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcTeDSczhiNEOE_qokb5z68Ph0PNcchuwYRynw&s');"></div><div id="relay2-status" class="absolute bottom-0 left-0 right-0 bg-gradient-to-t from-black/70 to-black/0 h-1/6"></div></div></div></main></div>
<script>
let relay1Triggered=false;
async function fetchData(){
 try{
  const r=await fetch('/data');const d=await r.json();
  document.getElementById('temperature').textContent=d.temperature+' °C';
  document.getElementById('humidity').textContent=d.humidity+' %';
  if(typeof d.temperature==='number'){
   if(d.temperature>30&&!relay1Triggered){toggleRelay1();relay1Triggered=true;}
   else if(d.temperature<=30&&relay1Triggered){relay1Triggered=false;}
  }
 }catch(e){console.warn(e);}
}
setInterval(fetchData,2000);fetchData();
async function toggleRelay1(){const r=await fetch('/relay1');const d=await r.json();updateRelayVisual('relay1-status',d.relay1);}
async function toggleRelay2(){const r=await fetch('/relay2');const d=await r.json();updateRelayVisual('relay2-status',d.relay2);}
function updateRelayVisual(id,state){const el=document.getElementById(id);el.classList.toggle('h-full',state);el.classList.toggle('h-1/6',!state);}
</script></body></html>)rawliteral";
    server.send(200, "text/html", html);
  });

  server.on("/data", []() {
    String json = "{\"temperature\":" + String(lastTemp, 1) + ",\"humidity\":" + String(lastHum, 1) + "}";
    server.send(200, "application/json", json);
  });

  server.on("/relay1", []() {
    relayState1 = !relayState1;
    digitalWrite(RELAY1, relayState1);
    String json = "{\"relay1\":" + String(relayState1 ? "true" : "false") +
                  ",\"relay2\":" + String(relayState2 ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  server.on("/relay2", []() {
    relayState2 = !relayState2;
    digitalWrite(RELAY2, relayState2);
    String json = "{\"relay1\":" + String(relayState1 ? "true" : "false") +
                  ",\"relay2\":" + String(relayState2 ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  server.on("/download", []() {
    String csv = "No,Temperature (°C),Humidity (%),Time\n";
    for (int i = 0; i < logSize; i++) {
      csv += String(i + 1) + "," + String(logs[i].temperature, 1) + "," +
             String(logs[i].humidity, 1) + "," + logs[i].timestamp + "\n";
    }
    server.sendHeader("Content-Disposition", "attachment; filename=data.csv");
    server.send(200, "text/csv", csv);
  });

  server.on("/firmware", []() {
    server.send(200, "text/html", F("<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>"));
  });

  // OTA
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) Update.begin(UPDATE_SIZE_UNKNOWN);
    else if (upload.status == UPLOAD_FILE_WRITE) Update.write(upload.buf, upload.currentSize);
    else if (upload.status == UPLOAD_FILE_END) Update.end(true);
  });

  server.begin();
  Serial.println("Server aktif");
}

// ====================== LOOP ======================
void loop() {
  server.handleClient();
  unsigned long now = millis();
  if (now - lastRead > sensorInterval) {
    lastRead = now;
    timeClient.update();
    lastTemp = readTemp();
    lastHum = readHum();
    logs[logIndex] = {lastTemp, lastHum, getTimeString()};
    logIndex = (logIndex + 1) % logSize;
  }
}
