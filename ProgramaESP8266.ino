#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "HX711.h" 
#include <PubSubClient.h>

// Replace the next variables with your SSID/Password combination
#define WIFI_SSID "NETONE_CABELLO"
#define WIFI_PASSWORD "ncgenius1991"

//#define WIFI_SSID "Lab. Telematica"
//#define WIFI_PASSWORD "l4bt3l3m4tic@"

// Add your MQTT Broker IP address, example:
#define MQTT_HOST "192.168.0.115"
#define MQTT_PORT 1883

//#define MQTT_HOST IPAddress(200, 126, 14, 223)
//#define MQTT_PORT 8238

// Temperature MQTT Topics
#define MQTT_PUB_DISTANCIA "tacho1/distancia"
#define MQTT_PUB_PESO "tacho1/peso"

//#define MQTT_PUB_DISTANCIA2 "tacho2/distancia"
//#define MQTT_PUB_PESO2 "tacho2/peso"

//#define MQTT_PUB_DISTANCIA3 "tacho3/distancia"
//#define MQTT_PUB_PESO3 "tacho3/peso"

const int trigPin = 15;
const int echoPin = 14;

const int LOADCELL_DOUT_PIN = 12;
const int LOADCELL_SCK_PIN = 13;
char buffer[5];
HX711 scale;

WiFiClient espClient;
PubSubClient client(espClient);
const char *topic = "tacho1/distancia";
const char *topic2 = "tacho1/peso";

//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034

long duration;
float distanceCm;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void callback(char *topic, byte *payload, unsigned int length) {
 Serial.print("Message arrived in topic: ");
 Serial.println(topic);
 Serial.print("Message:");
 
 for (int i = 0; i < length; i++) {
  Serial.print((char) payload[i]);
 }
 
 Serial.println();
 Serial.println(" - - - - - - - - - - - -");
}

void dataPubSub(){
  double oneReading = (scale.get_units(5)/1000);
  float peso1 = roundf((oneReading - 2.60) * 10) / 10;
  String peso = dtostrf(peso1, 3, 1, buffer);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
        
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
          
  // Calculate the distance
  distanceCm = duration * SOUND_VELOCITY/2;

  Serial.print("Distancia (cm): ");
  Serial.println(distanceCm);

  Serial.print("Peso (kg): ");
  Serial.println(peso);

  //connecting to a mqtt broker
  client.setServer("192.168.0.115", MQTT_PORT);
  client.setCallback(callback);

  while (!client.connected()) {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
  
    Serial.printf("The client %s connects to mosquitto mqtt broker\n", client_id.c_str());
  
    if (client.connect(client_id.c_str())) {
      Serial.println("Public emqx mqtt broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  // publish and subscribe
 client.publish(topic,String(distanceCm).c_str());
 client.subscribe(topic);

 client.publish(topic2,String(peso).c_str());
 client.subscribe(topic2);
}


void setup() {
  Serial.begin(115200); // Starts the serial communication
  Serial.setTimeout(2000);

 // Wait for serial to initialize.
  while(!Serial) { }

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); 
  scale.set_scale(76.925);
  //scale.tare(); // reset the scale to 0  

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  connectToWifi();
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.println("Connecting to WiFi..");
  }
  dataPubSub();
  delay(1000);
  Serial.println("A mimir");
  ESP.deepSleep(5e6);
}

void loop() {

}