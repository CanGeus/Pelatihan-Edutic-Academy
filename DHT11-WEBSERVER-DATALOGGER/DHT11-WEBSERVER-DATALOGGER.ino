#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

const char *ssid = "SSID";
const char *password = "PASSWORD";

#define DHT_PIN 14
#define DHT_TYPE DHT11

WebServer server(80);
DHT dht(DHT_PIN, DHT_TYPE);

// Setup NTP (WIB: GMT+7)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", 25200, 60000);

// Struktur log data
struct LogData
{
  float temperature;
  float humidity;
  String timestamp;
};

// Array penyimpanan data
const int logSize = 16;
LogData logData[logSize];
int logIndex = 0;

// Mendapatkan nama hari
String getDayOfWeek(int day)
{
  switch (day)
  {
  case 0:
    return "Minggu";
  case 1:
    return "Senin";
  case 2:
    return "Selasa";
  case 3:
    return "Rabu";
  case 4:
    return "Kamis";
  case 5:
    return "Jumat";
  case 6:
    return "Sabtu";
  default:
    return "";
  }
}

// Fungsi untuk membaca sensor
float readDHTTemperature()
{
  float t = dht.readTemperature();
  if (isnan(t))
  {
    Serial.println("Failed to read temperature!");
    return -1;
  }
  return t;
}

float readDHTHumidity()
{
  float h = dht.readHumidity();
  if (isnan(h))
  {
    Serial.println("Failed to read humidity!");
    return -1;
  }
  return h;
}

// Fungsi untuk waktu
String getFormattedTime()
{
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  struct tm *ti = localtime(&rawTime);
  String dayOfWeek = getDayOfWeek(ti->tm_wday);

  char datetime[30];
  strftime(datetime, sizeof(datetime), "%d/%m/%Y %H:%M:%S", ti);
  return dayOfWeek + ", " + String(datetime);
}

void handleData();
void handleDownload();

// ====================== SETUP ======================
void setup()
{
  Serial.begin(115200);
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());

  timeClient.begin();

  if (MDNS.begin("esp32"))
  {
    Serial.println("MDNS responder started");
  }

  // Routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/download", handleDownload);

  server.begin();
  Serial.println("HTTP server started");
}

// ====================== LOOP ======================
void loop()
{
  server.handleClient();

  float t = readDHTTemperature();
  float h = readDHTHumidity();

  logData[logIndex].temperature = t;
  logData[logIndex].humidity = h;
  logData[logIndex].timestamp = getFormattedTime();

  logIndex = (logIndex + 1) % logSize;

  delay(5000);
}

// ====================== HANDLERS ======================
void handleRoot()
{
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Dashboard</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;500;700&display=swap" rel="stylesheet">
    <style>
        * { font-family: "Roboto", sans-serif !important; }
        .fade { transition: all 0.3s ease-in-out; }
    </style>
</head>

<body class="bg-gray-200 min-h-screen flex flex-col items-center justify-start">
    <div class="bg-white min-h-[100svh] max-w-lg w-full text-center shadow-lg relative overflow-hidden">

        <nav class="fixed top-0 left-1/2 transform -translate-x-1/2 w-full max-w-lg px-6 py-3 flex justify-between items-center z-50 bg-white shadow-md">
            <div class="flex flex-col text-left">
                <div class="text-xl font-bold">Hello Admin 👋</div>
                <p class="text-sm text-gray-500">Welcome Back!</p>
            </div>
            <div class="relative h-full">
                <img width="30" height="30" src="https://img.icons8.com/ios/50/appointment-reminders--v1.png" alt="Notifikasi" />
                <div class="absolute top-0 right-0 bg-blue-600 rounded-full w-2.5 h-2.5 border-2 border-white"></div>
            </div>
        </nav>

        <main class="flex flex-col gap-5 px-8 pt-24 pb-10 min-h-[1280px] bg-gray-50">
            <div class="relative w-full h-[450px] rounded-3xl overflow-hidden">
                <div class="absolute inset-0 bg-cover bg-center"
                    style="background-image: url('https://housing.com/news/wp-content/uploads/2023/03/exterior-design-shutterstock_1932966368-1200x700-compressed.jpg');">
                </div>
                <div class="absolute bottom-0 left-0 right-0 bg-gradient-to-t from-blue-800 to-blue-500/70 p-4 text-white mx-5 mb-5 rounded-2xl">
                    <div class="flex justify-between items-center">
                        <div class="text-left">
                            <h2 class="text-lg font-semibold">Device 1</h2>
                            <p class="text-sm opacity-90">Realtime Update</p>
                        </div>
                        <div class="">
                            <a href="/download" class="py-2 px-4 bg-white text-blue-600 font-bold rounded-2xl">Unduh Data</a>
                        </div>
                    </div>
                    <hr class="my-4">
                    <div class="flex justify-around items-center">
                        <div class="flex gap-3">
                            <div class="p-3 bg-white rounded-full">
                                <img src="https://img.icons8.com/ios-filled/50/fa314a/temperature.png" class="w-6 h-6">
                            </div>
                            <div class="text-left">
                                <h3 class="text-md font-bold">Temperature</h3>
                                <p id="temperature" class="text-sm font-bold">-- °C</p>
                            </div>
                        </div>
                        <div class="flex gap-3">
                            <div class="p-3 bg-white rounded-full">
                                <img src="https://img.icons8.com/ios-filled/50/00add6/hygrometer.png" class="w-6 h-6">
                            </div>
                            <div class="text-left">
                                <h3 class="text-md font-bold">Humidity</h3>
                                <p id="humidity" class="text-sm font-bold">-- %</p>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <p class="text-xs mt-2 text-gray-500">Data diperbarui setiap 2 detik</p>
        </main>
    </div>

    <script>
        async function fetchData() {
            try {
                const res = await fetch('/data');
                if (!res.ok) throw new Error("Gagal mengambil data dari server");
                const data = await res.json();
                document.getElementById("temperature").textContent = (data.temperature ?? '--') + " °C";
                document.getElementById("humidity").textContent = (data.humidity ?? '--') + " %";
            } catch (error) {
                console.warn("Gagal mengambil data:", error);
            }
        }
        fetchData();
        setInterval(fetchData, 2000);
    </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// Endpoint JSON data
void handleData()
{
  float t = readDHTTemperature();
  float h = readDHTHumidity();
  String json = "{\"temperature\":" + String(t, 2) + ",\"humidity\":" + String(h, 2) + "}";
  server.send(200, "application/json", json);
}

// Endpoint download CSV
void handleDownload()
{
  String csvContent = "No,Temperature (°C),Humidity (%),Time\n";
  for (int i = 0; i < logSize; i++)
  {
    csvContent += String(i + 1) + "," + String(logData[i].temperature, 2) + "," +
                  String(logData[i].humidity, 2) + "," + logData[i].timestamp + "\n";
  }

  server.sendHeader("Content-Disposition", "attachment; filename=data.csv");
  server.send(200, "text/csv", csvContent);
}