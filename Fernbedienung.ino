/*
  Basic ESP8266 MQTT example

  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP8266 board/library.

  It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

  It will reconnect to the server if the connection is lost using a blocking
  reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
  achieve the same result without blocking the main loop.

  To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

#define BULITIN_LED 2            // Led in NodeMCU at pin GPIO16 (D0).
#define LED 4
#define TASTER_1 13
#define TASTER_2 12
#define POWER 14
#define DEBOUNCE 100
#define TOPIC "Fernbediehnung2"
#define POWER_TIME 30000

// Must be changed
const char* ssid = "SID";
const char* password = "PASSWORD";
const char* mqtt_server = "192.168.10.24";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long t;
unsigned long powerTimer;

enum {
  wait,
  wait_T1,
  wait_T2,
  T1_pressed,
  T2_pressed,
  none_pressed
} stat;

void setup_wifi() {

  delay(100);
  // We start by connecting to a WiFi network
  randomSeed(micros());
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
}

void connect() {
  Serial.print("Attempting MQTT connection to ");
  Serial.print(mqtt_server);
  Serial.print(" .. ");
  // Create a random client ID
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
    shutdown();
  }
}

void setup() {
  stat = wait;
  powerTimer = millis() + POWER_TIME;
  pinMode(POWER, OUTPUT);
  digitalWrite(POWER, HIGH);
  pinMode(BULITIN_LED, INPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(LED, HIGH);
  pinMode(LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(BULITIN_LED, HIGH);
  pinMode(TASTER_1, INPUT);
  pinMode(TASTER_2, INPUT);
  delay(100);
  Serial.begin(115200);
  client.setServer(mqtt_server, 1883);
}

void doStateMashine() {
  switch (stat) {
    case wait_T1:
      if (t < millis()) {
        t = millis() + 250;
        if (digitalRead(LED) == HIGH) {
          digitalWrite(LED, LOW);
          digitalWrite(BULITIN_LED, LOW);
        } else {
          digitalWrite(LED, HIGH);
          digitalWrite(BULITIN_LED, HIGH);
          Serial.print(".");
        }
      }
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected();
        connect();
        button1pressed();
        stat = T1_pressed;
        powerTimer = millis() + POWER_TIME;
      }
      break;

    case wait_T2:
      if (t < millis()) {
        t = millis() + 250;
        if (digitalRead(LED) == HIGH) {
          digitalWrite(LED, LOW);
          digitalWrite(BULITIN_LED, LOW);
        } else {
          digitalWrite(LED, HIGH);
          digitalWrite(BULITIN_LED, HIGH);
          Serial.print(".");
        }
      }
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected();
        connect();
        button2pressed();
        stat = T2_pressed;
        powerTimer = millis() + POWER_TIME;
      }
      break;

    case wait:
      if (digitalRead(TASTER_1) == HIGH) {
        setup_wifi();
        stat = wait_T1;
      } else if (digitalRead(TASTER_2) == HIGH) {
        setup_wifi();
        stat = wait_T2;
      }
      break;

    case T1_pressed:
      if (digitalRead(TASTER_1) == LOW && t < millis()) {
        t = millis() + DEBOUNCE;
        stat = none_pressed;
      }
      break;

    case T2_pressed:
      if (digitalRead(TASTER_2) == LOW && t < millis()) {
        t = millis() + DEBOUNCE;
        stat = none_pressed;
      }
      break;

    case none_pressed:
      if (digitalRead(TASTER_1) == HIGH && t < millis()) {
        powerTimer = millis() + POWER_TIME;
        t = millis() + DEBOUNCE;
        button1pressed();
        stat = T1_pressed;
      } else if (digitalRead(TASTER_2) == HIGH) {
        powerTimer = millis() + POWER_TIME;
        t = millis() + DEBOUNCE;
        button2pressed();
        stat = T2_pressed;
      }
      break;
  }
}

void loop() {
  doStateMashine();
  client.loop();
  if (powerTimer < millis()) {
    shutdown();
  }
}

void button1pressed() {
  Serial.println("Button 1 pressed");
  client.publish(TOPIC, "pressed 1");
}
void button2pressed() {
  Serial.println("Button 2 pressed");
  client.publish(TOPIC, "pressed 2");
}

void wifiConnected(void) {
  digitalWrite(BULITIN_LED, LOW);
  digitalWrite(LED, LOW);
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void shutdown(void) {
  client.publish(TOPIC, "off");
  client.disconnect();
  WiFi.disconnect();
  Serial.println("Turning off");
  Serial.flush();
  digitalWrite(POWER, LOW);
  for (;;) {
    ESP.wdtFeed();
  }
}
