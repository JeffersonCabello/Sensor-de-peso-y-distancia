//#include <Arduino.h>
//#include "HX711.h"
//#include "soc/rtc.h"
//#include "BluetoothSerial.h"
//#include "esp_adc_cal.h"
//#include <Preferences.h>

// Pines para el sensor de peso HX711
//const int LOADCELL_DOUT_PIN = 16;
//const int LOADCELL_SCK_PIN = 4;

//HX711 scale;
//Preferences preferences;

// Pines para el sensor ultrasonico
const int trigPin = 5;
const int echoPin = 18;

//Velocidad del sonido en cm/uS
#define SOUND_SPEED 0.034

long duration;
float distanceCm;

void setup() {
  Serial.begin(9600); // Starts the serial communication
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  //scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  //preferences.begin("NPC-Balanzas", false); 
  //double calibracion = preferences.getDouble("calibracion",14.83);
  //scale.set_scale(14.83);
  //scale.tare(); // reset the scale to 0
  //preferences.end();
  
}

void loop() {

  //double oneReading = (scale.get_units(5)/1000);

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

  //Serial.print("Peso (Kg): ");
  //Serial.println(oneReading);
  
  delay(1000);
}