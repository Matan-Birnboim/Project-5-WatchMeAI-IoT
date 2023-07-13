#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <BluetoothSerial.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

MAX30105 particleSensor;
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);
BluetoothSerial SerialBT;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

unsigned long imuLastUpdateTime = 0;
unsigned long pulseLastUpdateTime = 0;
unsigned long imuUpdateInterval = 50;   // Update interval for IMU data (in milliseconds)
unsigned long pulseUpdateInterval = 50; // Update interval for pulse data (in milliseconds)
float beatsPerMinute;
int beatAvg;
  float x;
  float y;
  float z;

void setup()
{
#ifndef ESP8266
  while (!Serial);     // will pause Zero, Leonardo, etc until serial console opens
#endif
  Serial.begin(115200);

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  
  SerialBT.begin("WatchMeAI"); //Bluetooth device name
}

void loop()
{
  long irValue = particleSensor.getIR();
  checkForBeat(irValue);
  sensors_event_t event;
  accel.getEvent(&event);
  unsigned long currentMillis = millis();

  if (currentMillis - imuLastUpdateTime >= imuUpdateInterval) {
    x = event.acceleration.x;
    y = event.acceleration.y;
    z = event.acceleration.z;
    //Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
    //Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
    //Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");
    imuLastUpdateTime = currentMillis;
  }

  if (currentMillis - pulseLastUpdateTime >= pulseUpdateInterval) {
    if (checkForBeat(irValue) == true)
    {
      //We sensed a beat!
      long delta = millis() - lastBeat;
      lastBeat = millis();
  
      beatsPerMinute = 60 / (delta / 1000.0);
  
      if (beatsPerMinute < 255 && beatsPerMinute > 20)
      {
        rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
        rateSpot %= RATE_SIZE; //Wrap variable
  
        //Take average of readings
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    

    }
    /*Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.println(beatAvg);
    if (irValue < 50000)
      Serial.print(" No finger?");*/
      
  }
  SerialBT.println((String)"Accelerometer:" + x + ", " + y + ", " + z + ", " + beatsPerMinute + ", " + beatAvg);
  Serial.println((String)"Accelerometer:" + x + ", " + y + ", " + z + ", " + beatsPerMinute + ", " + beatAvg);

  delay(50);
}
