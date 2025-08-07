
// press button to return from deep sleep. I think this happens anyway without doing this
#define HELTEC_POWER_BUTTON  // must be before "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Frequency in MHz. Keep the decimal point to designate float.
#define FREQUENCY 866.3  // for Europe
// #define FREQUENCY           905.2       // for US

// LoRa bandwidth. Keep the decimal point to designate float.
// Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
#define BANDWIDTH 250.0

// Number from 5 to 12. Higher means slower but higher "processor gain",
// meaning (in nutshell) longer range and more robust against interference.
#define SPREADING_FACTOR 9

// Transmit power in dBm. 0 dBm = 1 mW, enough for tabletop-testing. This value can be
// set anywhere between -9 dBm (0.125 mW) to 22 dBm (158 mW). Note that the maximum ERP
// (which is what your antenna maximally radiates) on the EU ISM band is 25 mW, and that
// transmissting without an antenna can damage your hardware.
#define TRANSMIT_POWER 0

const char* ssid = "ATTTtalCjA-Eero";
const char* password = "ts3wubt?6j%e";

const char* mqttServer = "broker.hivemq.com";
//const char* mqttServer = "test.mosquitto.com";
const char* mqttClient = "beargrass-creek";
const char* mqttTopic = "bgcrk/readings";

WiFiClient espClient;
PubSubClient client(espClient);
JsonDocument doc;

const int MAX_WORDS = 10; // Adjust as needed
String words[MAX_WORDS]; 

String rxdata;
volatile bool rxFlag = false;

void setupDisplay(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  // Send a reset
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
  display.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Connecting to ");
    Serial.println(mqttServer);
    // Attempt to connect
    if (client.connect(mqttClient)) {
      Serial.println("connected");
      Serial.println(mqttClient);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setupRadio() {
  RADIOLIB_OR_HALT(radio.begin());
  // Set the callback function for received packets
  radio.setDio1Action(rx);
  // Set radio parameters
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  // Start receiving
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}


void setup() {
  heltec_setup();
  setupDisplay();
  setupRadio();
  setupWiFi();
  client.setServer(mqttServer, 1883);
}

void loop() {
  heltec_loop();

  // connect to MQTT broker
  // if (!client.connected()) {
  //   reconnect();
  // }
  // client.loop();

  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      heltec_led(50);  // flash LED upon receiving data
      delay(100);
      heltec_led(0);

      Serial.printf("RSSI[%.2f],SNR[%.2f]\n", radio.getRSSI(), radio.getSNR());
      
      // populates the global words
      splitString(rxdata.c_str(), " ");
    
      doc[String("id")] = words[1];
      doc[String("timestamp")] = words[0];
      doc[String("temp")] = words[2];
      doc[String("bVolts")] = words[3];
      doc[String("mA")] = words[4];
      doc[String("RSSI")] = String(radio.getRSSI(), 2);
      doc[String("SNR")] = String(radio.getSNR(), 2);
      
      String output;
      serializeJson(doc, output);
      client.publish(mqttTopic, output.c_str());
      Serial.println(output);
      display.clear();
      display.drawStringMaxWidth(0, 0, 128, output);
      display.display();
    }
  }
}

// Can't do Serial or display things here, takes too much time for the interrupt
void rx() {
  rxFlag = true;
}

// populates the global "words" String array
int splitString(const char* str, char* delim) {
  int wordCount = 0;
  char* token = strtok(const_cast<char*>(str), delim); // strtok modifies the string, so we need to cast away const

  while (token != NULL && wordCount < MAX_WORDS) {
    words[wordCount++] = token;
    token = strtok(NULL, delim);
  }
  return wordCount;
}
