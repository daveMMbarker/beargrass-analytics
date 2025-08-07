#include <Preferences.h>

#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "ESP32-Access-Point";
const char* password = "thisHasToBeManyducks";

// Function prototypes
String httpGETRequest(String serverName);
void connectToWiFiAndPrintDetails();
void displayConnectionStatus(String message);
void displayTemperature(String temperature);
void displayHumidity(String humidity);
void displayRequestCount(unsigned long count);
void updateRestartCount();
void initDisplay();

//Your IP address or domain name with URL path
const char* serverNameTemp = "http://192.168.4.1/temperature";
const char* serverNameHumi = "http://192.168.4.1/humidity";

#include <Wire.h>
#include "SSD1306Wire.h"
#include "pins_arduino.h"

SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED);

String temperature;
String humidity;

unsigned long previousMillis = 0;
const long interval = 15000;
unsigned long requestCounter = 0;
unsigned long restarts;
Preferences preferences;

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void)  //Vext default OFF
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup() {
  //preferences.begin("restart-info", false);
  //restarts = preferences.getLong("restarts", 0);
  Serial.begin(115200);

  initDisplay();
  connectToWiFiAndPrintDetails();
}

void loop() {
  unsigned long currentMillis = millis();

//   if (currentMillis - previousMillis >= interval) {
//     if (WiFi.status() == WL_CONNECTED) {
//       temperature = httpGETRequest(serverNameTemp);
//       humidity = httpGETRequest(serverNameHumi);

//       Serial.println("Temperature: " + temperature + " *C - Humidity: " + humidity);

//       displayTemperature(temperature);
//       displayHumidity(humidity);
//       displayRequestCount(requestCounter++);

//       previousMillis = currentMillis;
//     } else {
//       displayConnectionStatus("Connection lost\nRestarting...");
//       delay(10000);
//       updateRestartCount();
//       ESP.restart();
//     }
//   }
}

// Function definitions

// String httpGETRequest(String serverName) {
//   WiFiClient client;
//   HTTPClient http;

//   http.begin(client, serverName);
//   int httpResponseCode = http.GET();

//   String payload = "--";

//   if (httpResponseCode > 0) {
//     Serial.print("HTTP Response code: ");
//     Serial.println(httpResponseCode);
//     payload = http.getString();
//   } else {
//     Serial.print("Error code: ");
//     Serial.println(httpResponseCode);
//   }
//   http.end();
//   return payload;
// }

void connectToWiFiAndPrintDetails() {
  WiFi.begin(ssid, password);
  displayConnectionStatus("Connecting...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("restarts:" + String(restarts)); //Added String conversion

  displayConnectionStatus("Connected!\nRestarts: " + String(restarts)); //Added String conversion
  delay(5000);
}

void displayConnectionStatus(String message) {
  //display.clearDisplay();
  //display.setTextSize(1);
  //display.setTextColor(WHITE);
  //display.setCursor(0, 0);
  display.drawString(0, 0, message);
  //display.display();
}

void displayTemperature(String temperature) {
  display.drawString(0, 0, "T: ");
  display.drawString(0, 0, temperature);
  display.drawString(0, 0, " ");
  
  display.drawString(0, 0, "F");
}

void displayHumidity(String humidity) {
  //display.setTextSize(2);
  //display.setCursor(0, 25);
  display.drawString(0, 25, "H: ");
  display.drawString(0, 25, humidity);
  display.drawString(0, 25, " %");
}

void displayRequestCount(unsigned long count) {
  //display.setTextSize(2);
  //display.setCursor(0, 50);
  display.drawString(0, 50, "#: ");
  display.drawString(0, 50, String(count));
}

void updateRestartCount() {
    //preferences.putLong("restarts", ++restarts);
    //preferences.end();
}

void initDisplay() {
  // Send a reset
    // Send a reset
  VextON();
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, HIGH);
  delay(1);
  digitalWrite(RST_OLED, LOW);
  delay(1);
  digitalWrite(RST_OLED, HIGH);
  delay(1);

}
