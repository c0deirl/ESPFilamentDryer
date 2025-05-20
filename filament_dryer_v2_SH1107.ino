#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_SH110X.h> // For SH1107 Display
#include <ArduinoJson.h> // For JSON API

// Replace with your network credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Value to use for the display, Initialize as "Standby"
char* displayvalue = "Standby";

// Pin assignments
#define DHTPIN 4
#define DHTTYPE DHT22
#define HEATER_PIN 5
#define FAN_PIN 6

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Initialize SSD1306 display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_RESET -1
Adafruit_SH1107 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, 1000000, 1000000);
//setRotation(1); //Set the screen orientation

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

float setTemperature = 25.0; // Default set temperature
int duration = 0; // Default duration in minutes
unsigned long startTime = 0;
bool heating = false;
bool status = false;

void setup() {
  // Initialize serial port
  Serial.begin(115200);

  // Initialize DHT sensor
  dht.begin();

  // Initialize SSD1306 display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setRotation(1); // Set orientation of the screen if needed
  display.setTextSize(1); // Set the default text size

  // Initialize relay pins
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Start server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Filament Dryer Controller</title>
      <style>
        body { font-family: Arial, sans-serif; background:rgb(46, 46, 46); color: #222; margin: 0; padding: 0;}
        .container { max-width: 400px; margin: 40px auto; background:rgb(46, 46, 46); border-radius: 8px; box-shadow: 0 2px 8px #ccc; padding: 24px;}
        h1 { text-align: center; margin-bottom: 0.5em; color:rgb(235, 133, 37);}
        .reading { font-size: 1.4em; margin: 1em 0; color:rgb(230, 230, 230); text-align: center;}
        .label { font-weight: bold; margin-bottom: 8px; color:rgb(230, 230, 230); display: block;}
        .form-group { margin: 16px 0; text-align: center;}
        input[type="number"] { width: 80px; padding: 0.5em; margin-right: 8px;}
        .duration-btns button {
          background: #2563eb;
          color: #fff;
          border: none;
          border-radius: 4px;
          padding: 10px 22px;
          margin: 0 5px;
          font-size: 1em;
          cursor: pointer;
          transition: background 0.2s;
        }
        .duration-btns button.selected, .duration-btns button:hover {
          background:rgb(11, 38, 114);
        }
        .submit-btn {
          margin-top: 18px;
          background: #059669;
          color: #fff;
          border: none;
          border-radius: 4px;
          padding: 10px 28px;
          font-size: 1em;
          cursor: pointer;
          transition: background 0.2s;
        }
		a:link, a:visited {
			
			color: white;
			
			text-align: center;
			text-decoration: none;
			}

			a:hover, a:active {
			background-color: red;
			}
        .submit-btn:hover { background: #047857;}
        .status { margin-top: 18px; text-align: center;}
        .footer { margin-top: 32px; text-align: center; font-size: 0.95em; color: #888;}
      </style>
    </head>
    <body>
      <div class="container">
        <h1>Filament Dryer</h1>
        <div class="reading">
          <span class="label">Current Temperature:</span>
          <span id="tempVal">--</span> &deg;C
        </div>
        <div class="reading">
          <span class="label">Current Humidity:</span>
          <span id="humVal">--</span> %
        </div>
        <form id="settingsForm" method="POST" action="/set">
          <div class="form-group">
            <span class="label">Set Temperature (&deg;C):</span>
            <input type="number" id="temp" name="temp" step="0.1" required>
          </div>
          <div class="form-group">
            <span class="label">Duration:</span>
            <div class="duration-btns">
              <button type="button" data-min="120">2 hours</button>
              <button type="button" data-min="240">4 hours</button>
              <button type="button" data-min="360">6 hours</button>
            </div>
            <input type="hidden" id="duration" name="duration" value="120">
          </div>
          <div class="form-group">
            <button type="submit" class="submit-btn">Start</button>
          </div>
        </form>
        <div class="status" id="statusMsg"></div>
        <div class="footer">ESP Filament Dryer | Github - <a href="https://github.com/c0deirl" target="_blank" >c0deIRL </a> &copy; 2025</div>
      </div>
      <script>
        // Duration button selection
        const durationBtns = document.querySelectorAll('.duration-btns button');
        const durationInput = document.getElementById('duration');
        durationBtns.forEach(btn => {
          btn.addEventListener('click', function() {
            durationBtns.forEach(b => b.classList.remove('selected'));
            this.classList.add('selected');
            durationInput.value = this.getAttribute('data-min');
          });
        });
        // Default select first button
        durationBtns[0].classList.add('selected');

        // Live temperature/humidity update
        function updateReadings() {
          fetch('/api/reading')
            .then(r => r.json())
            .then(data => {
              document.getElementById('tempVal').textContent = data.temperature !== null ? data.temperature : '--';
              document.getElementById('humVal').textContent = data.humidity !== null ? data.humidity : '--';
            });
        }
        setInterval(updateReadings, 2000);
        updateReadings();

        // Optional: show a status message after submit
        const form = document.getElementById('settingsForm');
        form.addEventListener('submit', function(e) {
          e.preventDefault();
          const fd = new FormData(form);
          fetch('/set', {
            method: 'POST',
            body: fd
          }).then(r => r.text()).then(msg => {
            document.getElementById('statusMsg').innerHTML = msg;
          });
        });
      </script>
    </body>
    </html>
    )rawliteral";
    request->send(200, "text/html", html);
  });

  // API endpoint for live readings
  server.on("/api/reading", HTTP_GET, [](AsyncWebServerRequest *request){
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    // Handle sensor errors (NaN)
    String response;
    StaticJsonDocument<100> doc;
    doc["temperature"] = isnan(t) ? 0.0 : t;
    doc["humidity"] = isnan(h) ? 0.0 : h;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/set", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("temp", true) && request->hasParam("duration", true)) {
      setTemperature = request->getParam("temp", true)->value().toFloat();
      duration = request->getParam("duration", true)->value().toInt();
      startTime = millis();
      heating = true;
      digitalWrite(FAN_PIN, HIGH); // Turn on fan
      request->send(200, "text/html", "<span style='color:green;'>Settings updated &amp; started.</span>");
    } else {
      request->send(400, "text/html", "<span style='color:red;'>Invalid input.</span>");
    }
  });

  server.begin();
}

void loop() {
  if (heating) {
    float currentTemp = dht.readTemperature();

    // Set the Hysteresis to prevent very quickly turning on and off
    // This allows the temperature to go above or below the set point by X degrees before triggering the heater

    float hysteresis = 5;
    
    if (currentTemp < setTemperature - hysteresis) {
      digitalWrite(HEATER_PIN, HIGH); // Turn on heater
    } 
    else if (currentTemp > setTemperature + hysteresis)
    {
      digitalWrite(HEATER_PIN, LOW); // Turn off heater
    }

    if ((millis() - startTime) > (duration * 60000)) {
      heating = false;
      digitalWrite(HEATER_PIN, LOW); // Turn off heater
      digitalWrite(FAN_PIN, LOW); // Turn off fan
    }
  }

  if (heating=false) {
    displayvalue = "Standby";
 }
    else {
        displayvalue = "Drying";
    }
  
  // Update display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(dht.readTemperature());
  display.println(" C");
  display.print("Humidity: ");
  display.print(dht.readHumidity());
  display.println(" %");
  display.println(displayvalue);
  display.display();

  delay(1000);
}