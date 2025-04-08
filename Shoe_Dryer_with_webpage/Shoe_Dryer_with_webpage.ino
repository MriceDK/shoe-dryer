#include <DHT.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// WiFi credentials
const char* ssid = "wlmlvin";
const char* password = "mlvin@8730";

#define DHTTYPE DHT22
#define FANPIN1 18
#define FANPIN2 19
#define BUTTON_PIN 27
#define BUTTON_LIGHT_PIN 23

DHT dht1(25, DHTTYPE);
DHT dht2(26, DHTTYPE);
DHT dht3(33, DHTTYPE);

int buttonState = LOW;
int lastButtonState = LOW;
int debounceStartTime = 0;
unsigned long prevMillis;
unsigned long fanTimer = 300000;
bool fansRunning = false;
float humidity1 = 0;
float humidity2 = 0;
float humidity3 = 0;
String fanStateLeft = "off";
String fanStateRight = "off";
float humidityThreshold = 0; // Global variable to store the threshold
bool isProcessing = false;
AsyncWebServer server(80);

void setup() {
  Serial.begin(9600);
  pinMode(FANPIN1, OUTPUT);
  pinMode(FANPIN2, OUTPUT);
  pinMode(BUTTON_LIGHT_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  dht1.begin();
  dht2.begin();
  dht3.begin();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    const char index_html[] PROGMEM = R"rawliteral(
      <!DOCTYPE html>
      <html lang="en">
      <head>
        <meta charset="UTF-8">
        <title>Shoe Dryer</title>
        <style>
          /* General Styles */
          body {
              font-family: Arial, sans-serif;
              margin: 0;
              padding: 0;
              background-color: #f9f9f9;
              color: #333;
          }

          h1 {
              text-align: center;
              background-color: #007BFF;
              color: white;
              padding: 1rem;
              margin: 0;
              border-bottom: 0.25rem solid #0056b3;
              max-width: 100%;
          }

          h2 {
              color: #0056b3;
              font-size: 1.5rem;
              font-weight: bold;
              margin-top: 2rem;
              text-align: center;
          }

          /* Form Styles */
          form {
              max-width: 25rem;
              margin: 0.5rem auto 2rem;
              padding: 1.5rem;
              background: white;
              border-radius: 1rem;
              box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
          }

          label {
              display: block;
              margin-bottom: 0.5rem;
              font-weight: bold;
              margin-top: 1rem;
          }

          input[type="number"] {
              width: 100%;
              padding: 0.5rem;
              margin-bottom: 1rem;
              border: 0.1rem solid #ddd;
              border-radius: 0.5rem;
              font-size: 1rem;
          }

          button, input[type="button"] {
              display: inline-block;
              background-color: #007BFF;
              color: white;
              border: none;
              padding: 0.7rem 1.5rem;
              font-size: 1rem;
              border-radius: 0.5rem;
              cursor: pointer;
              transition: background-color 0.3s ease;

          }

          button:hover, input[type="button"]:hover{
              background-color: #0056b3;
          }

          footer {
              background-color: #007BFF;
              display: block;
              padding: 1rem;
              margin-top: 1rem;
              max-width: 100%;
          }

          /* Paragraph Styles */
          p {
              text-align: center;
              margin: 1rem 0;
          }

          span {
              font-weight: bold;
              color: #007BFF;
          }

          footer p {
              color: #FFF;
          }

          /* Humidity and Fan Control Section */

          #fan-control-container {
              display: flex;
              flex-flow: column wrap;
          }

          /* Humidity Meters Section */
          #humidity-meters, #fan-control-container {
              display: flex;
              justify-content: space-around; /* Space between the boxes */
              align-items: center; /* Center items vertically */
              margin: 0.5rem auto 2rem;
              padding: 1rem;
              max-width: 25rem;
              background: white;
              border-radius: 10px;
              box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
          }

          /* Individual Humidity Box */
          .humidity-box {
              flex: 1; /* Make all boxes the same width */
              margin: 0 0.5rem; /* Spacing between boxes */
              padding: 1rem;
              text-align: center;
              background-color: #f9f9f9; /* Use variable or fallback */
              border-radius: 8px;
              box-shadow: 0 2px 6px rgba(0, 0, 0, 0.1);
          }

          .humidity-box p {
              margin: 0.5rem 0;
              font-weight: bold;
          }

          .humidity-box span {
              display: block;
              font-size: 1.2rem;
              color: #007BFF;
          }



          /* Button Centering */
          form button {
              display: block;
              width: 100%;
              margin-top: 1rem;
          }

          #fan-control-container button {
              align-self: center;
              max-width: 10rem;
          }

          /* Updated Styles for New Elements */
          #current-humidity-threshold {
              color: #28a745;
          }

          /* Responsive Design */
          @media (max-width: 25rem) {
              body {
                  font-size: 0.9rem;
                  padding: 0 1rem;
              }

              h1 {
                  font-size: 1.5rem;
                  padding: 0.8rem;
              }

              form {
                  max-width: 90%;
                  padding: 1rem;
              }

              button {
                  font-size: 0.9rem;
              }

              section {
                  max-width: 90%;
              }
          }

        </style>
      </head>
      <body>

      <h1>Shoe Dryer webinterface</h1>
        <!--Version 1.1 - 15/03/2025 15:35 -->
        <h2>Fan Settings</h2>
        <form>
          <label for="fan-timer">Set fan timer (Minutes) : </label>
          <input type="number" id="fan-timer" name="fanTimerControl" value="0">
          <input type="button" id="set-timer" value="Set Timer">

          <label for="humidity-threshold">Set humidity difference required : </label>
          <input type="number" id="humidity-threshold" name="humidityThresholdControl" value="0">
          <input type="button" id="set-humidity" value="Set difference required">

          <p>Fans will run for <span id="time-left">0</span> Minutes</p>
        </form>

        <h2>Humidity Meters</h2>
        <p id="current-humidity-threshold-text">Current humidity difference required <span id="current-humidity-threshold">0%</span></p>
        <div id="humidity-meters">
          <div class="humidity-box">
            <p>Left : <span id="humidity-left">- %</span></p>
          </div>
          <div class="humidity-box">
            <p>Control : <span id="humidity-control">- %</span></p>
          </div>
          <div class="humidity-box">
            <p>Right : <span id="humidity-right">- %</span></p>
          </div>
        </div>


        <h2>Fan Control</h2>
        <div id="fan-control-container">
          <p>Fan status LEFT : <span id="fan-status-left">off</span></p>
          <p>Fan status RIGHT : <span id="fan-status-right">off</span></p>

          <button id="toggle-fans">Toggle Fans</button>
        </div>
      <footer>
        <p>© 2025 MDK</p>
      </footer>
        <script>
          document.addEventListener('DOMContentLoaded', init)

          function init() {
              document.querySelector("#set-timer").addEventListener("click", updateTimer);
              document.querySelector("#set-humidity").addEventListener("click", updateHumidityTheshold);
              document.querySelector("#toggle-fans").addEventListener("click", toggleFans)

          }

          function updateTimer() {
              const $fanTimer = document.querySelector("#fan-timer");
              document.querySelector("#time-left").textContent = $fanTimer.value;
          }
          
          function updateHumidityTheshold() {
            const humidityThreshold = document.querySelector("#humidity-threshold").value;
            if (humidityThreshold >= 0 && humidityThreshold <=100) {
                document.querySelector("#current-humidity-threshold-text").innerHTML = `Current humidity threshold <span id="current-humidity-difference">${humidityThreshold}%</span>`;
                // Send the threshold value to the backend
                fetch('/setThreshold?threshold=' + humidityThreshold)
                  .then(response => response.text())
                  .then(data => {
                    console.log("Threshold set to: " + humidityThreshold);
                  });
            } else {
              document.querySelector("#current-humidity-threshold-text").innerHTML = `The humidity difference should be a value between <span id="current-humidity-difference">0 - 100 %</span>`;
            }
          }

          function toggleFans() {
            fetch('/toggle')
              .then(response => response.json())
              .then(data => {
                document.getElementById('fan-status-left').textContent = data.fanStateLeft;
                document.getElementById('fan-status-right').textContent = data.fanStateRight;
              });
          }

          function updateUI() {
            fetch('/data')
              .then(response => response.json())
              .then(data => {
                document.getElementById('humidity-left').textContent = data.humidity1 + "%";
                document.getElementById('humidity-control').textContent = data.humidity3 + "%";
                document.getElementById('humidity-right').textContent = data.humidity2 + "%";
                document.getElementById('fan-status-left').textContent = data.fanStateLeft;
                document.getElementById('fan-status-right').textContent = data.fanStateRight;
              });
          }

          setInterval(updateUI, 2000);
        </script>
      </body>
      </html>
      )rawliteral";
    request->send(200, "text/html", index_html);
  });

  server.on("/setTimer", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("timer")) {
      String timerValue = request->getParam("timer")->value();
      fanTimer = timerValue.toInt() * 60000;
    }
    request->redirect("/");
  });

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (isProcessing) {
      request->send(429, "text/plain", "Too many requests");
      return;
    }
    isProcessing = true;

    fansRunning = !fansRunning;
    digitalWrite(FANPIN1, fansRunning ? HIGH : LOW);
    digitalWrite(FANPIN2, fansRunning ? HIGH : LOW);
    digitalWrite(BUTTON_LIGHT_PIN, fansRunning ? HIGH : LOW);
    fanStateLeft = fansRunning ? "on" : "off";
    fanStateRight = fansRunning ? "on" : "off";
    if (fanStateLeft != "on" && fanStateLeft != "off") {
      fanStateLeft = "error";
    }
    if (fanStateRight != "on" && fanStateRight != "off") {
      fanStateRight = "error";
    }
    if (fansRunning) {
      prevMillis = millis(); // <-- ADD THIS LINE
    }
    StaticJsonDocument<200> doc;
    doc["fanStateLeft"] = fanStateLeft;
    doc["fanStateRight"] = fanStateRight;
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
    isProcessing = false;
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (isProcessing) {
      request->send(429, "text/plain", "Too many requests");
      return;
    }
    isProcessing = true;

    StaticJsonDocument<200> doc;
    doc["fanStateLeft"] = fanStateLeft;
    doc["fanStateRight"] = fanStateRight;
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
    isProcessing = false;
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (isProcessing) {
      request->send(429, "text/plain", "Too many requests");
      return;
    }
    isProcessing = true;

    StaticJsonDocument<300> doc;
    doc["humidity1"] = isnan(humidity1) ? -1 : humidity1; // Handle NaN values
    doc["humidity2"] = isnan(humidity2) ? -1 : humidity2;
    doc["humidity3"] = isnan(humidity3) ? -1 : humidity3;
    doc["fanStateLeft"] = fanStateLeft;
    doc["fanStateRight"] = fanStateRight;
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
    isProcessing = false;

  });


  server.on("/setThreshold", HTTP_GET, [&](AsyncWebServerRequest* request) {
    if (isProcessing) {
      request->send(429, "text/plain", "Too many requests");
      return;
    }
    isProcessing = true; 
    if (request->hasParam("threshold")) {
      String thresholdValue = request->getParam("threshold")->value();
      humidityThreshold = thresholdValue.toFloat(); // Store the threshold value
      Serial.println("Humidity threshold set to: " + thresholdValue);
      request->send(200, "text/plain", "Difference required set to " + thresholdValue);
    } else {
      request->send(400, "text/plain", "Difference parameter missing");
    }
    isProcessing = false;
  });

  server.begin();
}

void loop() {
  delay(25); // this speeds up the simulation
  buttonState = digitalRead(BUTTON_PIN);
  
  // Handle button press logic (toggle fans)
  if (buttonState != lastButtonState && millis() - debounceStartTime > 25) {
    debounceStartTime = millis(); // Reset debounce timer
    if (buttonState == LOW && (fanStateLeft == "on" || fanStateRight == "on")) {
      digitalWrite(FANPIN1, LOW);  // Turn fans off
      digitalWrite(FANPIN2, LOW);  
      digitalWrite(BUTTON_LIGHT_PIN, LOW);
      fansRunning = false;
      fanStateLeft = "off";
      fanStateRight = "off";
      Serial.println("Fans turned OFF via button");
    } else if (buttonState == LOW && (fanStateLeft == "off" && fanStateRight == "off")) {
      digitalWrite(FANPIN1, HIGH); // Turn fans on
      digitalWrite(FANPIN2, HIGH);  
      digitalWrite(BUTTON_LIGHT_PIN, HIGH);
      fansRunning = true;
      fanStateLeft = "on";
      fanStateRight = "on";
      Serial.println("Fans turned ON via button");
      prevMillis = millis(); // Reset timer start
    }
  }
  lastButtonState = buttonState;

  // Read humidity sensors
  humidity1 = isnan(dht1.readHumidity()) ? -1 : dht1.readHumidity();
  humidity2 = isnan(dht2.readHumidity()) ? -1 : dht2.readHumidity();
  humidity3 = isnan(dht3.readHumidity()) ? -1 : dht3.readHumidity();


  // If fanTimer is greater than 0, check if it's time to turn off the fans
  if (fanTimer > 0 && fansRunning && millis() - prevMillis > fanTimer) {
    if (!isnan(humidity1) && humidity1 <= humidity3 + humidityThreshold && fanStateLeft == "on") {
      digitalWrite(FANPIN1, LOW);
      fanStateLeft = "off";
      Serial.println("Fan 1 turned OFF due to humidity condition");
    }
    if (!isnan(humidity2) && humidity2 <= humidity3 + humidityThreshold && fanStateRight == "on") {
      digitalWrite(FANPIN2, LOW);
      fanStateRight = "off";
      Serial.println("Fan 2 turned OFF due to humidity condition");
    }
    // If both fans are off, stop the timer
    if (digitalRead(FANPIN1) == LOW && digitalRead(FANPIN2) == LOW) {
      fansRunning = false;
      digitalWrite(BUTTON_LIGHT_PIN, LOW);
      fanStateLeft = "off";
      fanStateRight = "off";
      Serial.println("Both fans turned OFF");
    }
  }

  // Handle case when fanTimer is 0 (indefinite run)
  if (fanTimer == 0 && !fansRunning) {
    // Fans will run indefinitely until manually turned off
    digitalWrite(FANPIN1, HIGH);
    digitalWrite(FANPIN2, HIGH);
    fansRunning = true;
    fanStateLeft = "on";
    fanStateRight = "on"; 
    digitalWrite(BUTTON_LIGHT_PIN, HIGH);
    }
}