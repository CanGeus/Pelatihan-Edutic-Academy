#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <DHT.h>

const char *ssid = "SSID";
const char *password = "PASSWORD";

WebServer server(80);

// Setup DHT
DHT dht(14, DHT11);
float readDHTTemperature()
{
  // Sensor readings may also be up to 2 seconds
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  if (isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    return -1;
  }
  else
  {
    Serial.println(t);
    return t;
  }
}
float readDHTHumidity()
{
  // Sensor readings may also be up to 2 seconds
  float h = dht.readHumidity();
  if (isnan(h))
  {
    Serial.println("Failed to read from DHT sensor!");
    return -1;
  }
  else
  {
    Serial.println(h);
    return h;
  }
}

// Setup Relay
const int relayPin1 = 25;
const int relayPin2 = 26;
bool relayState1 = true;
bool relayState2 = true;

/* Login Page */
String login_page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Login</title>
    <style>
      body {
        background: #3498db;
        font-family: sans-serif;
        font-size: 14px;
        color: #777;
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
        margin: 0;
      }
      form {
        background: #fff;
        width: 100%;
        max-width: 300px;
        padding: 30px 20px; /* Added left/right padding to card */
        border-radius: 5px;
        box-shadow: 0px 0px 15px rgba(0,0,0,0.2);
        text-align: center;
      }
      h1 {
        margin-bottom: 20px;
      }
      input {
        width: 100%;
        height: 44px;
        border-radius: 4px;
        margin: 10px 0;
        font-size: 15px;
        background: #f1f1f1;
        border: none;
        box-sizing: border-box; /* Ensures padding stays inside width */
        padding: 0 10px; /* Optional: internal spacing inside input text */
      }
      .btn {
        background: #3498db;
        color: #fff;
        border: none;
        cursor: pointer;
      }
    </style>
  </head>
  <body>
    <form name="loginForm" onsubmit="return check(this)">
      <h1>ESP32 Login</h1>
      <input name="userid" placeholder="User ID">
      <input name="pwd" placeholder="Password" type="password">
      <input type="submit" class="btn" value="Login">
    </form>
    <script>
      function check(form) {
        if (form.userid.value === 'admin' && form.pwd.value === 'admin') {
          window.location.href = '/serverIndex';
          return false;
        } else {
          alert('Error: Invalid Username or Password');
          return false;
        }
      }
    </script>
  </body>
  </html>
)rawliteral";

/* Main OTA Update Page */
String main_page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Update</title>
    <script src="https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4"></script>
    <style>
      body {
        background: #3498db;
        font-family: sans-serif;
        font-size: 14px;
        color: #777;
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
        margin: 0;
      }
      form {
        background: #fff;
        max-width: 300px;
        width: 100%;
        padding: 30px;
        border-radius: 5px;
        text-align: center;
        box-shadow: 0px 0px 15px rgba(0,0,0,0.2);
      }
      #file-input, input {
        width: 100%;
        height: 44px;
        border-radius: 4px;
        margin: 10px auto;
        font-size: 15px;
      }
      input {
        background: #f1f1f1;
        border: 0;
        padding: 0 15px;
      }
      #file-input {
        padding: 0;
        border: 1px solid #ddd;
        line-height: 44px;
        text-align: left;
        display: block;
        cursor: pointer;
      }
      #bar, #prgbar {
        background-color: #f1f1f1;
        border-radius: 10px;
      }
      #bar {
        background-color: #3498db;
        width: 0%;
        height: 10px;
      }
      .btn {
        background: #3498db;
        color: #fff;
        cursor: pointer;
      }
    </style>
  </head>
  <body>
    <form method="POST" action="#" enctype="multipart/form-data" id="upload_form">
      <input type="file" name="update" id="file" onchange="sub(this)" style="display:none">
      <label id="file-input" for="file">   Choose file...</label>
      <input type="submit" class="btn" value="Update">
      <br><br>
      <div id="prg"></div>
      <br>
      <div id="prgbar"><div id="bar"></div></div>
      <div class="flex gap-4 mt-3">
        <a href='/sensor' class='bg-green-700 text-white font-bold px-2 py-3 rounded-xl w-1/2 text-center'>Sensor</a>
        <a href='/relay' class='bg-red-700 text-white font-bold px-2 py-3 rounded-xl w-1/2 text-center'>Relay</a>
      </div>
    </form>
    

    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js"></script>
    <script>
      function sub(obj){
        var fileName = obj.value.split('\\\\');
        document.getElementById('file-input').innerHTML = '   ' + fileName[fileName.length - 1];
      }

      $('form').submit(function(e){
        e.preventDefault();
        var form = $('#upload_form')[0];
        var data = new FormData(form);

        $.ajax({
          url: '/update',
          type: 'POST',
          data: data,
          contentType: false,
          processData: false,
          xhr: function() {
            var xhr = new window.XMLHttpRequest();
            xhr.upload.addEventListener('progress', function(evt) {
              if (evt.lengthComputable) {
                var per = evt.loaded / evt.total;
                $('#prg').html('progress: ' + Math.round(per * 100) + '%');
                $('#bar').css('width', Math.round(per * 100) + '%');
              }
            }, false);
            return xhr;
          },
          success: function(d, s) {
            console.log('success!');
          },
          error: function (a, b, c) {
            console.log('error');
          }
        });
      });
    </script>
  </body>
  </html>
)rawliteral";

void sensorDht()
{
  /* Sensor Page */
  String sensor_page = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8">
      <meta http-equiv="refresh" content="2">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>ESP32 DHT11</title>
      <script src="https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4"></script>
      <style>
        body {
          background: #3498db;
          font-family: sans-serif;
          font-size: 14px;
          color: #777;
          display: flex;
          justify-content: center;
          align-items: center;
          height: 100vh;
          margin: 0;
        }
      </style>
    </head>

    <body>
      <div class='bg-white py-5 px-10 rounded-xl'>
        <h1 class='text-2xl font-bold capitalize text-center text-black mb-3'>Monitoring <br> Suhu dan kelembapan</h1>
        <div class='flex gap-4'>
          <div class='bg-blue-400 rounded-xl p-4 w-1/2 text-center'>
            <h1 class='text-xl text-black capitalize font-bold'>Temp</h1>
            <h1 class='text-xl text-black capitalize font-bold'>%temp% <span> C</span></h1>
          </div>
          <div class='bg-yellow-400 rounded-xl p-4 w-1/2 text-center'>
            <h1 class='text-xl text-black capitalize font-bold'>Hum</h1>
            <h1 class='text-xl text-black capitalize font-bold'>%hum% <span> %</span></h1>
          </div>
        </div>
        <div class="flex gap-4 mt-3">
          <a href='/sensor' class='bg-green-700 text-white font-bold px-2 py-3 rounded-xl w-1/2 text-center'>Sensor</a>
          <a href='/relay' class='bg-red-700 text-white font-bold px-2 py-3 rounded-xl w-1/2 text-center'>Relay</a>
        </div>
      </div>
    </body>

    </html>
  )rawliteral";

  sensor_page.replace("%temp%", String(readDHTTemperature(), 1));
  sensor_page.replace("%hum%", String(readDHTHumidity(), 1));

  server.send(200, "text/html", sensor_page);
}

// fungsi untuk menampilkan halaman status relay (tidak toggle saat dibuka)
void relayPage()
{
  String relay_page = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>ESP32 Relay</title>
      <script src="https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4"></script>
      <style>
        body {
          background: #3498db;
          font-family: sans-serif;
          font-size: 14px;
          color: #777;
          display: flex;
          justify-content: center;
          align-items: center;
          height: 100vh;
          margin: 0;
        }
      </style>
    </head>

    <body>
      <div class='bg-white py-5 px-10 rounded-xl'>
        <h1 class='text-2xl font-bold capitalize text-center text-black mb-3'>Monitoring Relay</h1>
        <div class='flex gap-4'>
          <div class='bg-blue-400 rounded-xl p-4 w-1/2 text-center'>
            <h1 class='text-xl text-black capitalize font-bold'>Relay 1</h1>
            <h1 class='text-xl text-black capitalize font-bold'>%relay1%</h1>
            <p><a href="/relay1">Toggle Relay 1</a></p>
          </div>
          <div class='bg-yellow-400 rounded-xl p-4 w-1/2 text-center'>
            <h1 class='text-xl text-black capitalize font-bold'>Relay 2</h1>
            <h1 class='text-xl text-black capitalize font-bold'>%relay2%</h1>
            <p><a href="/relay2">Toggle Relay 2</a></p>
          </div>
        </div>
        <div class="flex gap-4 mt-3">
          <a href='/sensor' class='bg-green-700 text-white font-bold px-2 py-3 rounded-xl w-1/2 text-center'>Sensor</a>
          <a href='/relay' class='bg-red-700 text-white font-bold px-2 py-3 rounded-xl w-1/2 text-center'>Relay</a>
        </div>
      </div>
    </body>

    </html>
  )rawliteral";

  relay_page.replace("%relay1%", relayState1 ? "On" : "Off");
  relay_page.replace("%relay2%", relayState2 ? "On" : "Off");

  server.send(200, "text/html", relay_page);
}

// endpoint untuk toggle relay 1
void toggleRelay1()
{
  relayState1 = !relayState1;
  digitalWrite(relayPin1, relayState1 ? HIGH : LOW);
  // redirect kembali ke halaman status
  server.sendHeader("Location", "/relay");
  server.send(303, "text/plain", "");
}

// endpoint untuk toggle relay 2
void toggleRelay2()
{
  relayState2 = !relayState2;
  digitalWrite(relayPin2, relayState2 ? HIGH : LOW);
  server.sendHeader("Location", "/relay");
  server.send(303, "text/plain", "");
}

/* Setup function */
void setup(void)
{
  Serial.begin(115200);
  dht.begin();

  // Initialize Wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // Initialize Relay
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  digitalWrite(relayPin1, relayState1 ? HIGH : LOW);
  digitalWrite(relayPin2, relayState2 ? HIGH : LOW);

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // memulai DNS server , untuk memberi domain pada esp32
  if (!MDNS.begin("esp32"))
  { // alamat web --->  http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }

  Serial.println("mDNS responder started");

  // handle login page
  server.on("/", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", login_page); });

  // handle main page
  server.on("/serverIndex", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", main_page); });

  // handle sensor page
  server.on("/sensor", sensorDht);

  // handle relay page
  server.on("/relay", HTTP_GET, relayPage);
  server.on("/relay1", HTTP_GET, toggleRelay1);
  server.on("/relay2", HTTP_GET, toggleRelay2);

  // handle untuk upload program
  server.on("/update", HTTP_POST, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); }, []()
            {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } });

  // memulai server
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void)
{
  server.handleClient();
  delay(2);
}