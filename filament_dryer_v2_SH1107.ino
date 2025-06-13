#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_SH110X.h> // For SH1107 Display
#include <ArduinoJson.h> // For JSON API
#include <PubSubClient.h> // MQTT Library										 

// Replace with your network credentials
const char* ssid = "SSID HERE";
const char* password = "PASSWORD HERE";

// MQTT broker settings
const char* mqtt_server = "MQTT_BROKER_IP";    // e.g., "192.168.1.10" or "broker.hivemq.com"
const int mqtt_port = 1883;                    // default MQTT port
const char* mqtt_user = "MQTT_USER";           // set to "" if no auth
const char* mqtt_pass = "MQTT_PASS";           // set to "" if no auth
const char* mqtt_topic = "espfilamentdryer/sensor"; // topic for sensor data

WiFiClient espClient;
PubSubClient mqttClient(espClient);					   								   

// Value to use for the display, Initialize as "Standby"
char* displayvalue = "Standby";

// GPIO Pin assignments
#define DHTPIN 4
#define DHTTYPE DHT22
#define HEATER_PIN 16
#define FAN_PIN 18

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Initialize SH1107 display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_RESET -1
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, 1000000, 100000);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

float setTemperature = 20.0; // Default set temperature
int duration = 0; // Default duration in minutes

unsigned long startTime = 0; //Initial Start Time
//Set the status values to false on startup
bool heating = false;
bool status = false;

// Create the MQTT section

unsigned long lastMqttPublish = 0;
const unsigned long mqttPublishInterval = 5000; // publish interval in ms (e.g., every 5 seconds)

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi. IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt_user[0] == '\0') {
      if (mqttClient.connect("ESPFilamentDryer")) {
        Serial.println("connected (no auth)");
      } else {
        Serial.print("failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
      }
    } else {
      if (mqttClient.connect("ESPFilamentDryer", mqtt_user, mqtt_pass)) {
        Serial.println("connected (with auth)");
      } else {
        Serial.print("failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
      }
    }
  }
}

void publishSensorData(float temperature, float humidity) {
  StaticJsonDocument<128> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  char payload[128];
  serializeJson(doc, payload);
  mqttClient.publish(mqtt_topic, payload);
}						  

void setup() {
  // Initialize serial port for debug connenctivity
  Serial.begin(115200);

  // Initialize DHT sensor
  dht.begin();

  // Initialize SH1107 display
  display.begin(0x3c, true);
  display.clearDisplay(); // Clear the display on startup
  display.setRotation(1); // Set orientation of the screen if needed
  display.setTextSize(1); // Set the default text size

  // Initialize relay pins
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  // If using an active low SSR, set the heater pin to default to HIGH rather than low.
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);

 setup_wifi();

  // Start server and configure the HTML for the web interface

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
        .submit-btn:hover { background: #047857;}
        .status { margin-top: 18px; text-align: center;}
        .footer { margin-top: 32px; text-align: center; font-size: 0.95em; color: #888;}
        .stop-btn {
          margin-top: 10px;
          background: #dc2626;
          color: #fff;
          border: none;
          border-radius: 4px;
          padding: 10px 28px;
          font-size: 1em;
          cursor: pointer;
          transition: background 0.2s;
        }
        .stop-btn:hover { background: #7f1d1d;}
        a:link, a:visited {
          color: white;
          text-align: center;
          text-decoration: none;
        }
        a:hover, a:active {
          background-color: red;
        }
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
		<div class="reading">
          <span class="label">Time Remaining:</span>
          <span id="timeRemainVal">--</span>
          <span id="timeRemainUnit">min</span>
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
        <div class="form-group">
          <button type="button" id="stopBtn" class="stop-btn">Stop</button>
        </div>
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
				// Fetch and update time remaining
          fetch('/api/remaining')
            .then(r => r.json())
            .then(data => {
              let min = data.minutes;
              if (typeof min !== "undefined" && min > 0) {
                document.getElementById('timeRemainVal').textContent = min;
                document.getElementById('timeRemainUnit').textContent = "min";
              } else {
                document.getElementById('timeRemainVal').textContent = "--";
                document.getElementById('timeRemainUnit').textContent = "min";
              }
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

        // Stop button handler
        document.getElementById('stopBtn').addEventListener('click', function() {
          fetch('/stop', { method: 'POST' })
            .then(r => r.text())
            .then(msg => {
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

 // --- New: API endpoint for time remaining ---
  server.on("/api/remaining", HTTP_GET, [](AsyncWebServerRequest *request){
    unsigned long timeLeft = 0;
    if (heating && duration > 0) {
      unsigned long elapsed = (millis() - startTime) / 60000; // in minutes
      timeLeft = (duration > elapsed) ? (duration - elapsed) : 0;
    }
    String response;
    StaticJsonDocument<64> doc;
    doc["minutes"] = timeLeft;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  // Web request to turn the heater on and start the timer
		
  server.on("/set", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("temp", true) && request->hasParam("duration", true)) {
      setTemperature = request->getParam("temp", true)->value().toFloat();
      duration = request->getParam("duration", true)->value().toInt();
      startTime = millis();
      heating = true;
												   
      request->send(200, "text/html", "<span style='color:green;'>Settings updated &amp; started.</span>");
    } else {
      request->send(400, "text/html", "<span style='color:red;'>Invalid input.</span>");
    }
  }
  );
  // Stop endpoint to reset system
  server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
    heating = false;
    setTemperature = 20.0; // Reset to default
    duration = 0;
    startTime = 0;
    digitalWrite(HEATER_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
    displayvalue = "Standby";
    request->send(200, "text/html", "<span style='color:green;'>Stopped and reset.</span>");
    }
  );

  server.begin();
}

void loop() {
if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop();
  
  if (heating = true) {
    float currentTemp = dht.readTemperature();

    // Set the Hysteresis to prevent very quickly turning on and off
    // This allows the temperature to go above or below the set point by X degrees before triggering the heater to stop or start
    // This should only be needed if using a standard relay, instead of a solid state relay. Saves on wear on the contacts

    float hysteresis = 1;
    
    //if (currentTemp < setTemperature - hysteresis) {
      if (currentTemp < setTemperature) {
      digitalWrite(HEATER_PIN, LOW); // Turn on heater
      digitalWrite(FAN_PIN, HIGH); // Turn on fan
      Serial.println("Turn On...");
      displayvalue = "Heating";
    } 
    else if (currentTemp > (setTemperature)) //Subtracting the hysteresis allows the latent heat in the coil to continue heating the air after power is removed
    {
      digitalWrite(HEATER_PIN, HIGH); // Turn off heater
      digitalWrite(FAN_PIN, HIGH); // Keep Fan On
      Serial.println("Turn Off...");
      displayvalue = "Standby";
    }

    if ((millis() - startTime) > (duration * 60000)) {
      heating = false;
      digitalWrite(HEATER_PIN, HIGH); // Turn off heater
      digitalWrite(FAN_PIN, LOW); // Turn off fan
      Serial.println("Finished");
      displayvalue = "STOP";
    }
  }
  
  // Serial Print remaining Time
  Serial.println("Start Time = ");
  Serial.println(startTime);
  timeremaining = (millis() - startTime);
  Serial.println("Time Remaining = ");
  Serial.println(timeremaining);

 // MQTT sensor publish
  unsigned long now = millis();
  if (now - lastMqttPublish > mqttPublishInterval) {
    lastMqttPublish = now;
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (!isnan(temp) && !isnan(hum)) {
      publishSensorData(temp, hum);
    }
  }

  //Display Values
				   
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Temp: ");
 // display.print("   ");
  display.print(dht.readTemperature());
  display.println(" C");
 // display.println(" ");
  display.println("Humid: ");
  //display.print("   ");
  display.print(dht.readHumidity());
  display.print(" %");
  display.setTextSize(3);
  display.setCursor(0, 85);
  display.print(displayvalue);
  display.println();
  display.setTextSize(1);
  display.setCursor(0, 120);
  display.println(WiFi.localIP()); // Display the local IP, this is optional
  display.display();

  delay(1000);
}