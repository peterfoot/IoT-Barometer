#include <ArduinoJson.h>

#include "AZ3166WiFi.h"
#include "DevKitMQTTClient.h"

#include "Sensor.h"
#include "Math.h"

static bool hasWifi = false;
static bool hasIoTHub = false;

LIS2MDLSensor *magnetometer;
LPS22HBSensor *barometer;
HTS221Sensor *htSensor;
DevI2C *ext_i2c;
int mag_axes[3];

float markedPressure = 1000; // pressure in mbar to use as baseline for tracking movements
bool useSimulatedValues = false;

// called when cloud-to-device message is received
void MessageReceived(const char* message, int length)
{
  StaticJsonBuffer<200> jsonBuffer;

  // Root of the object tree.
  //
  // It's a reference to the JsonObject, the actual bytes are inside the
  // JsonBuffer with all the other nodes of the object tree.
  // Memory is freed when jsonBuffer goes out of scope.
  JsonObject& root = jsonBuffer.parseObject(message);

  Screen.print(0, (const char*)root["alert"]);
}

// called when Button A is pressed and released
void buttonPress()
{
  // store current pressure (set barometer needle to track movement)
  barometer->getPressure(&markedPressure);
}

// called when Button B is pressed and released
void simulateButtonPress()
{
  useSimulatedValues = !useSimulatedValues;
  if(useSimulatedValues)
  {
    digitalWrite(LED_USER, 1);
  }
  else{
    digitalWrite(LED_USER,0);
  }
}

// returns an angle in degrees given x and y readings from the magnetometer.
static double angleForVector(double x, double y)
{
  // angle in radians from positive y axis (north)
  double radAngle = atan2(x,y);

  // angle in degrees
  double angle = degrees(radAngle);

  // convert negative values to correct positive equivalent
  if(angle < 0)
  {
    angle = 360.0 + angle;
  }

  return angle;
}

// returns a character to indicate compass direction N,E,S,W
static char compassPointFromAngle(double angle)
{
  char compassPoint = 'N';

  if(angle > 315)
  {
    compassPoint = 'N';
  }
  else if(angle > 225)
  {
    compassPoint = 'W';
  }
  else if(angle > 135)
  {
  compassPoint = 'S';
  }
  else if(angle > 45)
  {
    compassPoint = 'E';
  }

  return compassPoint;
}


void setup() {
  // put your setup code here, to run once:

  pinMode(USER_BUTTON_A, INPUT);
  attachInterrupt(USER_BUTTON_A, buttonPress, FALLING);

  pinMode(USER_BUTTON_B, INPUT);
  attachInterrupt(USER_BUTTON_B, simulateButtonPress, FALLING);

  ext_i2c = new DevI2C(D14, D15);
  magnetometer = new LIS2MDLSensor(*ext_i2c);
  magnetometer->init(NULL);

  barometer = new LPS22HBSensor(*ext_i2c);
  barometer->init(NULL);
  
  htSensor = new HTS221Sensor(*ext_i2c);
  htSensor->init(NULL);

  if (WiFi.begin() == WL_CONNECTED)
  {
    hasWifi = true;
    Screen.print(1, "Running...");

    if (!DevKitMQTTClient_Init())
    {
      hasIoTHub = false;
      return;
    }

    DevKitMQTTClient_SetMessageCallback(MessageReceived);
    hasIoTHub = true;
  }
  else
  {
    hasWifi = false;
    Screen.print(1, "No Wi-Fi");
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  float temp;
  float pressure;
  float humidity;
  char compassPoint;

  for(int i = 0; i < 4; i++)
  {

  magnetometer->getMAxes(mag_axes);
  double magX, magY;
  // store the magnetometer results reversed because we want heading in relation to screen orientation
  magX = (double)-mag_axes[0];
  magY = (double)-mag_axes[1];

  
  barometer->getPressure(&pressure);
  htSensor->getTemperature(&temp);
  htSensor->getHumidity(&humidity);

  if(useSimulatedValues)
  {
    pressure = 980.0;
  }

  // angle is now roughly angle from magnetic north (assuming DevKit is flat on a table)
  double angle = angleForVector(magX, magY);

  // compass point character for display
  compassPoint = compassPointFromAngle(angle);

  // build screen text
  char buff[128];
  snprintf(buff, 128, "%.1fC %.0fmb\r\n%.1f %% Wind: %c", temp, pressure, humidity, compassPoint);

    Screen.print(1, buff);

  
    delay(15000);
    // check every 15 seconds
    DevKitMQTTClient_Check(false);
  }
  
  // only send once per minute  
  if (hasIoTHub && hasWifi)
  {
    char buff[128];

   // build packet to send to Azure IoT Hub
    snprintf(buff, 128, "{\"barometerEvent\":\"log\",\"temp\": %.1f,\"pressure\": %.2f,\"markedPressure\": %.2f,\"humidity\": %.2f,\"wind\": \"%c\"}", temp, pressure, markedPressure, humidity, compassPoint);
     
    if (DevKitMQTTClient_SendEvent(buff))
    {
      Screen.print(3, "Sending...");
    }
    else
    {
      Screen.print(3, "Failure...");
    }
  }
}
