#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "HX711.h"

// Replace the next variables with your SSID/Password combination
#define WIFI_SSID "NETONE_CABELLO"
#define WIFI_PASSWORD "ncgenius1991"

// Add your MQTT Broker IP address, example:
//#define MQTT_HOST IPAddress(192, 168, 0, 113)
#define MQTT_HOST IPAddress(192, 168, 0, 108)
//#define MQTT_PORT 1833
#define MQTT_PORT 1883

// Temperature MQTT Topics
#define MQTT_PUB_DISTANCIA "tacho1/distancia"
#define MQTT_PUB_PESO "tacho1/peso"

#define MQTT_PUB_DISTANCIA2 "tacho2/distancia"
#define MQTT_PUB_PESO2 "tacho2/peso"

#define MQTT_PUB_DISTANCIA3 "tacho3/distancia"
#define MQTT_PUB_PESO3 "tacho3/peso"

const int trigPin = 15;
const int echoPin = 14;

const int LOADCELL_DOUT_PIN = 12;
const int LOADCELL_SCK_PIN = 13;
char buffer[10];
HX711 scale;

//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034

long duration;
float distanceCm;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

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

void setup() {
  Serial.begin(115200); // Starts the serial communication

 // Wait for serial to initialize.
  while(!Serial) { }

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); 
  scale.set_scale(76.925);
  scale.tare(); // reset the scale to 0  

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");

}

void data() {
  double oneReading = (scale.get_units(1)/1000);
  float peso = roundf(oneReading * 10) / 10;
  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = 10 seconds) 
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    // New DHT sensor readings
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
    distanceCm = duration * SOUND_VELOCITY/2;
    
    // Prints the distance on the Serial Monitor
    Serial.print("Distancia (cm): ");
    Serial.println(distanceCm);

    Serial.print("Peso (kg): ");
    Serial.println(peso);
    // Read temperature as Celsius (the default)
    
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //temp = dht.readTemperature(true);
    
    // Publish an MQTT message on topic esp/dht/temperature
    /*uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_DISTANCIA, 1, true, String(distanceCm).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_DISTANCIA, packetIdPub1);
    Serial.printf("Message: %.2f \n", distanceCm);*/

    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_DISTANCIA, 1, true, String(distanceCm).c_str());                            

    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_DISTANCIA2, 1, true, String(distanceCm).c_str());                            
                      
    uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_DISTANCIA3, 1, true, String(distanceCm).c_str());     

      // Publish an MQTT message on topic esp/dht/humidity
    /*uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_PESO, 1, true, String(peso).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_PESO, packetIdPub2);
    Serial.printf("Message: %.2f \n", peso);*/

    uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_PESO, 1, true, String(peso).c_str());                            

    uint16_t packetIdPub5 = mqttClient.publish(MQTT_PUB_PESO2, 1, true, String(peso).c_str());           

    uint16_t packetIdPub6 = mqttClient.publish(MQTT_PUB_PESO3, 1, true, String(peso).c_str());           


    /*unsigned long startTime = millis();

    wifi_set_sleep_type(LIGHT_SLEEP_T); 
    delay(300000);

    unsigned long endTime = millis();
    unsigned long duration = endTime - startTime;
    Serial.print("Duracion ZZZ: ");
    Serial.println(duration);*/
  }
 }

 void light_sleep(){
   wifi_station_disconnect();
   wifi_set_opmode_current(NULL_MODE);
   wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); // set sleep type, the above posters wifi_set_sleep_type() didnt seem to work for me although it did let me compile and upload with no errors 
   wifi_fpm_open(); // Enables force sleep
   gpio_pin_wakeup_enable(GPIO_ID_PIN(2), GPIO_PIN_INTR_LOLEVEL); // GPIO_ID_PIN(2) corresponds to GPIO2 on ESP8266-01 , GPIO_PIN_INTR_LOLEVEL for a logic low, can also do other interrupts, see gpio.h above
   wifi_fpm_do_sleep(0xFFFFFFF); // Sleep for longest possible time
 }

 void doDelays(){
   //delay(300000);
   delay(1000);
 }

void loop() {
  connectToWifi();
  data();
  delay(200);
  Serial.println("A mimir");
  light_sleep();
  doDelays();
  Serial.println("Buenos Dias de Dios");
}