#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_SSD1306.h>

// Replace with your network credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Pin assignments
#define DHTPIN 4
#define DHTTYPE DHT22
#define HEATER_PIN 5
#define FAN_PIN 6

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Initialize SSD1306 display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

float setTemperature = 25.0; // Default set temperature
int duration = 0; // Default duration in minutes
unsigned long startTime = 0;
bool heating = false;

void setup() {
  // Initialize serial port
  Serial.begin(115200);

  // Initialize DHT sensor
  dht.begin();

  // Initialize SSD1306 display
  if (!display.begin(SSD1306_I2C_ADDRESS, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();

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
    String html = "<html><body>";
    html += "<h1>Temperature Controller</h1>";
    html += "<p>Current Temperature: " + String(dht.readTemperature()) + " &deg;C</p>";
    html += "<p>Current Humidity: " + String(dht.readHumidity()) + " %</p>";
    html += "<form action=\"/set\" method=\"POST\">";
    html += "<label for=\"temp\">Set Temperature (&deg;C):</label>";
    html += "<input type=\"number\" id=\"temp\" name=\"temp\" step=\"0.1\"><br>";
    html += "<label for=\"duration\">Duration (minutes):</label>";
    html += "<input type=\"number\" id=\"duration\" name=\"duration\"><br>";
    html += "<input type=\"submit\" value=\"Submit\">";
    html += "</form>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/set", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("temp", true) && request->hasParam("duration", true)) {
      setTemperature = request->getParam("temp", true)->value().toFloat();
      duration = request->getParam("duration", true)->value().toInt();
      startTime = millis();
      heating = true;
      digitalWrite(FAN_PIN, HIGH); // Turn on fan
      request->send(200, "text/html", "<p>Settings updated. <a href=\"/\">Go back</a></p>");
    } else {
      request->send(400, "text/html", "<p>Invalid input. <a href=\"/\">Go back</a></p>");
    }
  });

  server.begin();
}

void loop() {
  if (heating) {
    float currentTemp = dht.readTemperature();
    if (currentTemp < setTemperature) {
      digitalWrite(HEATER_PIN, HIGH); // Turn on heater
    } else {
      digitalWrite(HEATER_PIN, LOW); // Turn off heater
    }

    if ((millis() - startTime) > (duration * 60000)) {
      heating = false;
      digitalWrite(HEATER_PIN, LOW); // Turn off heater
      digitalWrite(FAN_PIN, LOW); // Turn off fan
    }
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
  display.display();

  delay(1000);
}
