#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "HX711.h"
#include "soc/rtc.h"
#include "BluetoothSerial.h"
#include "esp_adc_cal.h"
#include <Preferences.h>
extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}
#include <AsyncMqttClient.h>

// Pines para el sensor de peso HX711
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;

// Replace the next variables with your SSID/Password combination
#define WIFI_SSID "NETONE_CABELLO"
#define WIFI_PASSWORD "ncgenius1991"

// Add your MQTT Broker IP address, example:
#define MQTT_HOST IPAddress(192, 168, 0, 113)
#define MQTT_PORT 1833

// Distancia y peso MQTT Topics
#define MQTT_PUB_TEMP "distance"
#define MQTT_PUB_HUM  "weight"

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

HX711 scale;
Preferences preferences;

// Pines para el sensor ultrasonico
const int trigPin = 5;
const int echoPin = 18;

//Velocidad del sonido en cm/uS
#define SOUND_SPEED 0.034

long duration;
float distanceCm;


void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200); // Starts the serial communication
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  preferences.begin("NPC-Balanzas", false); 
  double calibracion = preferences.getDouble("calibracion",14.83);
  scale.set_scale(14.83);
  scale.tare(); // reset the scale to 0
  preferences.end();

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  connectToWifi();
  
}

void loop() {

  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = 10 seconds) 
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
  previousMillis = currentMillis;

  double oneReading = (scale.get_units(5)/1000);

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;

  // Prints the distance in the Serial Monitor
  Serial.print("Distancia (cm): ");
  Serial.println(distanceCm);

  Serial.print("Peso (Kg): ");
  Serial.println(oneReading);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  // Check if any reads failed and exit early (to try again).
  if (isnan(distanceCm) || isnan(oneReading)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
    
  // Publish an MQTT message on topic esp32/dht/temperature
  uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(distanceCm).c_str());                            
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_TEMP, packetIdPub1);
  Serial.printf("Message: %.2f \n", distanceCm);

  // Publish an MQTT message on topic esp32/dht/humidity
  uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(oneReading).c_str());                            
  Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_HUM, packetIdPub2);
  Serial.printf("Message: %.2f \n", oneReading);
  }
  delay(1000);
}