#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char* ssid PROGMEM = "***************"; // replace with your WiFi SSID
const char* password PROGMEM = "***********"; // replace with your WiFi password
const char* host PROGMEM = "192.168.0.0"; // IP address of your MQTT broker (Jeedom)

// Debug Mode
// Allows for various messages to be sent on the serial port of the ESP for debugging.
// #define DEBUG 1

// setup states
#define STATE_INITIAL 0
#define STATE_IN_MSG 1
byte state = STATE_INITIAL;

char value[20];
char topic[] = "T211/12345678";
long lastReconnectAttempt = 0;

WiFiClient T211;
PubSubClient mqttClient(T211);

// Returns the next byte from the meter
// Blocks until there is something to return
char get_byte() {
  int a = -1;
  if (Serial.available() == 0) {
    delay(500);
    if (Serial.available() == 0) {
      // The serial buffer is empty. All messages have been processed.
      // We pause and then we request the data from the meter through the GPIO2 port.
      delay(2500);
      digitalWrite(2, HIGH);
    }
  }
  
  // We’re waiting for the data to arrive at the serial port.
  // The yield() function allows the ESP watchdog to be timed to prevent restarts.
  while ((a = Serial.read()) == -1) {
    yield();
  };
  digitalWrite(2, LOW);
  return a;
}

boolean reconnect() {
  digitalWrite(0, LOW);
  if (WiFi.status() != WL_CONNECTED) {
    #ifdef DEBUG
    Serial.println("Reconnecting to WiFi...");
    #endif
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    //Alternatively, you can restart your board
    //ESP.restart();
    #ifdef DEBUG
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.RSSI());
    #endif
  }
  while (WiFi.status() != WL_CONNECTED) {
    #ifdef DEBUG
    Serial.print('.');
    #endif
    digitalWrite(0, !digitalRead(0));
    delay(1000);
  }
  digitalWrite(0, LOW);
  // Setup the WILL message on connect
  mqttClient.connect("T211 Bridge");
  #ifdef DEBUG
  Serial.print("MQTT Status :");
  Serial.println(mqttClient.state());
  Serial.print("Wifi Status :");
  Serial.println(WiFi.status());
  #endif
  digitalWrite(0, HIGH);
  return mqttClient.connected();
}

void setup() {
  
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  // The ESP is started with this GPIO set to +5V.
  // Set it to 0 and wait for the meter to send its message before activating the serial port of the ESP.
  pinMode(2, OUTPUT); // Data Request from T211
  digitalWrite(2, LOW);
  delay(1000);
  Serial.setRxBufferSize(1024);
  #ifdef DEBUG
  Serial.begin(115200);
  #else
  Serial.begin(115200); //, SERIAL_8N2, SERIAL_RX_ONLY); 
  #endif
  #ifdef DEBUG
  Serial.println("Connection to wifi network");
  #endif


  // WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(0, !digitalRead(0));
    delay(500);
  }
  digitalWrite(0, HIGH);
  #ifdef DEBUG
  Serial.println("Connection to MQTT Broker");
  #endif
  mqttClient.setServer(host, 1883);


}

void loop() {

  state = STATE_INITIAL;
  while (state == STATE_INITIAL) {
    #ifdef DEBUG
    Serial.println("Wait input ...");
    #endif
    // Scan for CRLF1-0
    while (get_byte() != 13) {}
    if (get_byte() == 10 && get_byte() == '1' && get_byte() == '-' && get_byte() == '0') {
      state = STATE_IN_MSG;
    }
    mqttClient.loop();
  }
  #ifdef DEBUG
  Serial.println("Input detected");
  #endif
  get_byte(); // :
  int i = 5;
  topic[i] = get_byte();
  while (topic[i] != '(') { // ( Start Value
    i=i+1;
    topic[i] = get_byte();
  }
  topic[i] = 0;
  i = 0;
  value[0] = get_byte();
  while (value[i] != ')' && i < 19) { // ) End Value
    i=i+1;
    value[i] = get_byte();
  }
  value[i] = 0;
  if (i == 19) value[18] = '*'; //Payload is too long. addition of an wildcard to indicate that the payload has been truncated.
  #ifdef DEBUG
  Serial.print("Publish topic ");
  Serial.print(topic);
  Serial.print(" with payload ");
  Serial.println(value);
  #endif
  if (!mqttClient.connected()) {
    #ifdef DEBUG
    Serial.println("Connection to MQTT Broker lost. Reconnection...");
    #endif
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    mqttClient.loop();
    digitalWrite(0, LOW);
    mqttClient.publish(topic,(const uint8_t*) value,i,0);
    digitalWrite(0, HIGH);
  }
}
