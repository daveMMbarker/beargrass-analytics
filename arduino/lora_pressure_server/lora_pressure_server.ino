
#define HELTEC_POWER_BUTTON  // must be before "#include <heltec_unofficial.h>"

// haven't been able to figure out how to use the built-in OLED display and 
// the I2C realtime clock at the same time, so disable display for now
// The display wouldn't be used anyway since it would eat power
#define NO_DISPLAY
#include <heltec_unofficial.h>

// Frequency in MHz. Keep the decimal point to designate float.
// Check your own rules and regulations to see what is legal where you are.
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
// transmitting without an antenna can damage your hardware.
#define TRANSMIT_POWER 22

// for persisting the sequence counter to flash memory
#include <Preferences.h>

// RTC I2C sensor
#include "DS3232.h"
DS3231 rtc;

// the output of the current to voltage board (connected to pressure sensor)
#define PRESSURE_PIN 20

// #define LOCAL_MODE = 0;

long sequence = 0;
long counter = 0;
Preferences preferences;

// void displayReset(void) {
//   pinMode(Vext, OUTPUT);
//   digitalWrite(Vext, LOW);
//   // Send a reset
//   pinMode(RST_OLED, OUTPUT);
//   digitalWrite(RST_OLED, HIGH);
//   delay(1);
//   digitalWrite(RST_OLED, LOW);
//   delay(1);
//   digitalWrite(RST_OLED, HIGH);
//   delay(1);
//   display.init();
//   display.flipScreenVertically();
//   display.display();
// }

void setupRadio() {
  Serial.println("radio init");
  radio.begin();
  // Set radio parameters
  Serial.printf("Frequency: %.2f MHz\n", FREQUENCY);
  radio.setFrequency(FREQUENCY);
  Serial.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  radio.setBandwidth(BANDWIDTH);
  Serial.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  Serial.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  radio.setOutputPower(TRANSMIT_POWER);
}

void setup() {
   // initialize I2C for the RTC
  Wire.begin();
  if (rtc.begin() != DS3232_OK) {
    Serial.println("could not connect to RTC");
  }

  heltec_setup();

  #ifndef LOCAL_MODE
  setupRadio();
  #endif
  //displayReset();
  preferences.begin("sequences", false); 
  sequence = preferences.getLong("sequence", 0);
}

/**
  We really don't need a separate setup() function since it runs loop() once then sleeps 
  and runs everything again on wakeup
**/
void loop() {
  heltec_loop();
  // turn power on to external 5v pin. will use this to activate the relay to power the pressure sensor
  heltec_ve(true);
  // wait a second after turning power on 
  delay(1000);

  // read time from RTC
  rtc.read();
  String timestamp = String(rtc.month()) +"-" +String(rtc.day()) +"-" +String(rtc.year()) +"::" +String(rtc.hours()) +":" +String(rtc.minutes()) +":" +String(rtc.seconds());
  float t = heltec_temperature();
  // read battery voltage [doesn't seem to be working]
  // int vbat = heltec_battery_percent(heltec_vbat());
  int vbat = heltec_vbat();
  
  float sensorMa = 0.0;
  float maxCurrentMa = 20.0;
  // resolution of the ADC converter
  float ADCintervals = 4096.0;

  // mA from pressure sensor - should be in 4-20 ma range
  sensorMa = analogRead(PRESSURE_PIN) * (maxCurrentMa / ADCintervals); 

  sequence++;
  Serial.println(timestamp);
  Serial.printf("%d %.1f %d %.1f\n", sequence, t, vbat, sensorMa);

  heltec_led(50);  // flash LED 
  delay(100);
  heltec_led(0);

  #ifndef LOCAL_MODE

  radio.transmit(timestamp +" " + String(sequence) + " " +String(t) + " " +String(vbat) +" " +String(sensorMa) );
  if (_radiolib_status != RADIOLIB_ERR_NONE) {
    heltec_led(50);
    delay(20);
    heltec_led(0);
    heltec_led(50);
    delay(20);
    heltec_led(0);
    heltec_led(50);
    delay(20);
    heltec_led(0);
    Serial.printf("fail (%i)", _radiolib_status);
  }
  #endif

  preferences.putLong("sequence", sequence);
  preferences.end();

  // turn off external power; deactivate the voltage booster that drives the current to voltage converter
  heltec_ve(false);
  //delay(120000);
  #ifndef LOCAL_MODE
  heltec_deep_sleep(600);
  #endif
}
