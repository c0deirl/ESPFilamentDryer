#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Encoder.h>
#include "esp32-hal-ledc.h" // Add this line for PWM functions

// WiFi credentials
const char* ssid = "SurgeFX";
const char* password = "password";

// Pin definitions
#define MOTOR_PIN1 26  // H-Bridge input 1
#define MOTOR_PIN2 27  // H-Bridge input 2
#define MOTOR_EN 14    // H-Bridge enable pin
#define ENCODER_A 32   // Rotary encoder pin A
#define ENCODER_B 33   // Rotary encoder pin B

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C

// PWM configuration
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// Initialize objects
WebServer server(80);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ESP32Encoder encoder;

// Global variables
int speedValue = 0;
bool updateDisplay = true;

// HTML webpage
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>SurgeFX Foam Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { text-align: center; font-family: Arial; }
    .slider { width: 300px; }
  </style>
</head>
<body>
  <h2>SurgeFX Foam Control</h2>
  <h1><img src"/data/Social_Sharing_Logo.webp"></h1>
  <input type="range" min="0" max="255" value="0" class="slider" id="speedSlider">
  <p>Speed: <span id="speedValue">0</span></p>
  <script>
    var slider = document.getElementById("speedSlider");
    var output = document.getElementById("speedValue");
    slider.oninput = function() {
      output.innerHTML = this.value;
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/speed?value=" + this.value, true);
      xhr.send();
    }
    setInterval(function() {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          slider.value = this.responseText;
          output.innerHTML = this.responseText;
        }
      };
      xhr.open("GET", "/getSpeed", true);
      xhr.send();
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
  
  // Initialize encoder
  encoder.attachHalfQuad(ENCODER_A, ENCODER_B);
  encoder.setCount(0);
  
  // Initialize motor pins
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
//  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
// ledcAttachPin(MOTOR_EN, PWM_CHANNEL);
  
  // Connect to WiFi
//  WiFi.begin(ssid, password);
 // while (WiFi.status() != WL_CONNECTED) {
 //   delay(500);
 //   Serial.print(".");
//  }
    //Start the Wifi Access Point
  WiFi.softAP(ssid, password);

  // Display IP on OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("IP:");
  display.println(WiFi.softAPIP());
  display.display();
  
  // Setup web server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });
  
  server.on("/speed", HTTP_GET, []() {
    if (server.hasArg("value")) {
      speedValue = server.arg("value").toInt();
      updateMotor();
      updateDisplay = true;
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/getSpeed", HTTP_GET, []() {
    server.send(200, "text/plain", String(speedValue));
  });
  
  server.begin();
}

//void updateMotor() {
//  if (speedValue > 0) {
//    digitalWrite(MOTOR_PIN1, HIGH);
//    digitalWrite(MOTOR_PIN2, LOW);
//    ledcWrite(PWM_CHANNEL, speedValue);
//  } else {
//    digitalWrite(MOTOR_PIN1, LOW);
//    digitalWrite(MOTOR_PIN2, LOW);
//    ledcWrite(PWM_CHANNEL, 0);
//  }
//}

// Alternative PWM method
void updateMotor() {
  if (speedValue > 0) {
    digitalWrite(MOTOR_PIN1, HIGH);
    digitalWrite(MOTOR_PIN2, LOW);
    analogWrite(MOTOR_EN, speedValue);  // Using analogWrite instead of ledcWrite
  } else {
    digitalWrite(MOTOR_PIN1, LOW);
    digitalWrite(MOTOR_PIN2, LOW);
    analogWrite(MOTOR_EN, 0);
  }
}

void updateOLED() {
  display.clearDisplay();
  
  // Display IP address
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("IP:");
  display.println(WiFi.softAPIP());
  
  // Display speed value
  display.setCursor(0, 32);
  display.print("Speed: ");
  display.println(speedValue);
  
  // Draw a progress bar
  int barWidth = map(speedValue, 0, 255, 0, SCREEN_WIDTH - 4);
  display.drawRect(0, 50, SCREEN_WIDTH - 2, 10, SSD1306_WHITE);
  display.fillRect(2, 52, barWidth, 6, SSD1306_WHITE);
  
  display.display();
}

void loop() {
  server.handleClient();
  
  // Handle encoder
  static int lastCount = 0;
  int count = encoder.getCount();
  
  if (count != lastCount) {
    speedValue = constrain(count, 0, 255);
    encoder.setCount(speedValue);
    lastCount = speedValue;
    updateMotor();
    updateDisplay = true;
  }
  
  // Update OLED display
  if (updateDisplay) {
    updateOLED();
    updateDisplay = false;
  }
  
  delay(10);
}
