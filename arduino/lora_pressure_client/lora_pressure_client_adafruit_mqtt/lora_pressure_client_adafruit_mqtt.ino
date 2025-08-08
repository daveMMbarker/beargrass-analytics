// press button to return from deep sleep. I think this happens anyway without doing this
#define HELTEC_POWER_BUTTON  // must be before "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>
#include <WiFi.h>
#include <Preferences.h>

#include "WiFiClientSecure.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoJson.h>

/************************* Radio Setup *********************************/
// Frequency in MHz
#define FREQUENCY 866.3  // Europe
#define BANDWIDTH 250.0
#define SPREADING_FACTOR 9
#define TRANSMIT_POWER 0

/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  8883

Preferences prefs; // NVS storage

String aio_username;
String aio_key;
String wifi;
String wifi_pwd;

// io.adafruit.com root CA
const char* adafruitio_root_ca = \
"-----BEGIN CERTIFICATE-----\n"
    "MIIEjTCCA3WgAwIBAgIQDQd4KhM/xvmlcpbhMf/ReTANBgkqhkiG9w0BAQsFADBh\n"
      "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
      "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
      "MjAeFw0xNzExMDIxMjIzMzdaFw0yNzExMDIxMjIzMzdaMGAxCzAJBgNVBAYTAlVT\n"
      "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
      "b20xHzAdBgNVBAMTFkdlb1RydXN0IFRMUyBSU0EgQ0EgRzEwggEiMA0GCSqGSIb3\n"
      "DQEBAQUAA4IBDwAwggEKAoIBAQC+F+jsvikKy/65LWEx/TMkCDIuWegh1Ngwvm4Q\n"
      "yISgP7oU5d79eoySG3vOhC3w/3jEMuipoH1fBtp7m0tTpsYbAhch4XA7rfuD6whU\n"
      "gajeErLVxoiWMPkC/DnUvbgi74BJmdBiuGHQSd7LwsuXpTEGG9fYXcbTVN5SATYq\n"
      "DfbexbYxTMwVJWoVb6lrBEgM3gBBqiiAiy800xu1Nq07JdCIQkBsNpFtZbIZhsDS\n"
      "fzlGWP4wEmBQ3O67c+ZXkFr2DcrXBEtHam80Gp2SNhou2U5U7UesDL/xgLK6/0d7\n"
      "6TnEVMSUVJkZ8VeZr+IUIlvoLrtjLbqugb0T3OYXW+CQU0kBAgMBAAGjggFAMIIB\n"
      "PDAdBgNVHQ4EFgQUlE/UXYvkpOKmgP792PkA76O+AlcwHwYDVR0jBBgwFoAUTiJU\n"
      "IBiV5uNu5g/6+rkS7QYXjzkwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQGCCsG\n"
      "AQUFBwMBBggrBgEFBQcDAjASBgNVHRMBAf8ECDAGAQH/AgEAMDQGCCsGAQUFBwEB\n"
      "BCgwJjAkBggrBgEFBQcwAYYYaHR0cDovL29jc3AuZGlnaWNlcnQuY29tMEIGA1Ud\n"
      "HwQ7MDkwN6A1oDOGMWh0dHA6Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEds\n"
      "b2JhbFJvb3RHMi5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUHAgEW\n"
      "HGh0dHBzOi8vd3d3LmRpZ2ljZXJ0LmNvbS9DUFMwDQYJKoZIhvcNAQELBQADggEB\n"
      "AIIcBDqC6cWpyGUSXAjjAcYwsK4iiGF7KweG97i1RJz1kwZhRoo6orU1JtBYnjzB\n"
      "c4+/sXmnHJk3mlPyL1xuIAt9sMeC7+vreRIF5wFBC0MCN5sbHwhNN1JzKbifNeP5\n"
      "ozpZdQFmkCo+neBiKR6HqIA+LMTMCMMuv2khGGuPHmtDze4GmEGZtYLyF8EQpa5Y\n"
      "jPuV6k2Cr/N3XxFpT3hRpt/3usU/Zb9wfKPtWpoznZ4/44c1p9rzFcZYrWkj3A+7\n"
      "TNBJE0GmP2fhXhP1D/XVfIW/h0yCJGEiV9Glm/uGOa3DXHlmbAcxSyCRraG+ZBkA\n"
      "7h4SeM6Y8l/7MBRpPCz6l8Y=\n"
"-----END CERTIFICATE-----\n";

/****************************** MQTT & Feeds *********************************/
WiFiClientSecure client;
Adafruit_MQTT_Client *mqtt; // We'll create this dynamically after loading creds
Adafruit_MQTT_Publish *beargrass;

JsonDocument doc;
const int MAX_WORDS = 10;
String words[MAX_WORDS]; 
String rxdata;
volatile bool rxFlag = false;

/****************************** Setup Functions *********************************/
void setupDisplay(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, HIGH);
  delay(1);
  digitalWrite(RST_OLED, LOW);
  delay(1);
  digitalWrite(RST_OLED, HIGH);
  delay(1);
  display.init();
  display.flipScreenVertically();
  display.display();
}

void setupWiFi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(wifi);
  WiFi.begin(wifi, wifi_pwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupRadio() {
  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rx);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

/****************************** Preferences *********************************/
void loadAIOCredentials() {
  prefs.begin("aio", false); // namespace "aio"
  aio_username = prefs.getString("user", "");
  aio_key      = prefs.getString("key", "");
  prefs.end();

  if (aio_username == "" || aio_key == "") {
    // No stored creds — set defaults here or request via Serial
    aio_username = "<aio username>";
    aio_key = "<aio key";
    
    prefs.begin("aio", false);
    prefs.putString("user", aio_username);
    prefs.putString("key", aio_key);
    prefs.end();
    Serial.println("Stored default AIO credentials in NVS");
  } else {
    Serial.println("Loaded AIO credentials from NVS");
  }
  prefs.begin("wifi", false); // namespace "wifi"
  wifi     = prefs.getString("wifi", "");
  wifi_pwd = prefs.getString("wifi_pwd", "");
  prefs.end();
  if (wifi == "" || wifi_pwd == "") {
    wifi = "<ssid>";
    wifi_pwd = "<wifi pwd>";

    prefs.begin("wifi", false);
    prefs.putString("wifi", wifi);
    prefs.putString("wifi_pwd", wifi_pwd);
    prefs.end();
    Serial.println("Stored default WIFI credentials in NVS");
  } else {
    Serial.println("Loaded WIFI credentials from NVS");
  }
}

/****************************** Arduino Core *********************************/
void setup() {
  Serial.begin(115200);
  heltec_setup();
  setupDisplay();
  setupRadio();
  loadAIOCredentials();
  setupWiFi();
  client.setCACert(adafruitio_root_ca);

  mqtt = new Adafruit_MQTT_Client(&client, AIO_SERVER, AIO_SERVERPORT, aio_username.c_str(), aio_key.c_str());
  beargrass = new Adafruit_MQTT_Publish(mqtt, (aio_username + "/feeds/beargrass").c_str());
}

void loop() {
  heltec_loop();
  MQTT_connect();

  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      heltec_led(50);
      delay(100);
      heltec_led(0);

      splitString(rxdata.c_str(), " ");
      doc["id"] = words[1];
      doc["timestamp"] = words[0];
      doc["mA"] = words[4];
      doc["RSSI"] = String(radio.getRSSI(), 2);
      doc["SNR"] = String(radio.getSNR(), 2);

      String output;
      serializeJson(doc, output);

      if (!beargrass->publish(output.c_str())) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }
      Serial.println(output);
      display.clear();
      display.drawStringMaxWidth(0, 0, 128, output);
      display.display();
    }
  }
}

void rx() { rxFlag = true; }

int splitString(const char* str, char* delim) {
  int wordCount = 0;
  char* token = strtok(const_cast<char*>(str), delim);
  while (token != NULL && wordCount < MAX_WORDS) {
    words[wordCount++] = token;
    token = strtok(NULL, delim);
  }
  return wordCount;
}

void MQTT_connect() {
  int8_t ret;
  if (mqtt->connected()) return;

  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt->connect()) != 0) {
       Serial.println(mqtt->connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt->disconnect();
       delay(5000);
       if (--retries == 0) {
         Serial.println("Unable to connect to MQTT");
         return;
       }
  }
  Serial.println("MQTT Connected!");
}
