#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define HOST "https://do-an-212.herokuapp.com/api/v1/device"
#define ledPin 4
const char *ssid = "hello la xin chao";
const char *password = "29042011";
StaticJsonDocument<512> doc;
#define DHT_SENSOR_PIN 5 // ESP32 pin  connected to DHT11 sensor
#define DHT_SENSOR_TYPE DHT11
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

int currentState;
int userState;
float currentHumi;
float currentTempC;
unsigned long previousMillis = 0;
unsigned long interval = 30000;

void setup()
{

  Serial.begin(9600);
  dht_sensor.begin(); // initialize the DHT sensor
  // initialize LCD
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  pinMode(ledPin, OUTPUT);
}

void jsonDataPost(float heatIndex, float humidity, float temperature)
{

  StaticJsonDocument<128> jsonDoc;
  jsonDoc["deviceType"] = "sensor";
  jsonDoc["temperature"] = temperature;
  jsonDoc["heatIndex"] = heatIndex;
  jsonDoc["humidity"] = humidity;

  char jsonBuffer[512];
  serializeJson(jsonDoc, jsonBuffer);
  Serial.println(jsonBuffer);
  HTTPClient httPost;
  httPost.begin(HOST "/624a56c92ece6342f406f59b");
  httPost.addHeader("Content-Type", "application/json");
  int httpResponceCode = httPost.PATCH(jsonBuffer);
  if (httpResponceCode > 0)
  {
    // String response = httPost.getString();
    Serial.print("PATCH Request status code: ");
    Serial.println(httpResponceCode);
    // Serial.println(response);
    // Serial.println("Patched");
  }
  else
  {
    Serial.print("err:");
    Serial.println(httpResponceCode);
  }
  httPost.end();
}

void httpGet()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(HOST "/624a57a6b282e150330274cf");
    //    http.addHeader("Content-Type","text/plain");
    int httpResponceCode = http.GET();
    if (httpResponceCode > 0)
    {
      Serial.print("GET Request status code: ");
      Serial.println(httpResponceCode);
      DeserializationError error = deserializeJson(doc, http.getString());

      if (error)
      {
        Serial.print(F("deserializeJson() failed with error code "));
        Serial.println(error.f_str());
        return;
      }

      // const char *status = doc["status"]; // "success"

      JsonObject data = doc["data"];
      // const char *data_id = data["_id"];                  // "624a57a6b282e150330274cf"
      const char *data_name = data["name"]; // "Light Bulb"
      // const char *data_description = data["description"]; // "Indoor Device"
      // const char *data_deviceType = data["deviceType"];   // "regDevice"
      bool data_deviceStatus = data["deviceStatus"]; // true
      // const char *data_startDate = data["startDate"];     // "2022-04-04T02:27:48.094Z"
      userState = data_deviceStatus ? HIGH : LOW;
      currentState = digitalRead(ledPin);
      if (userState != currentState)
      {
        digitalWrite(ledPin, userState);
        Serial.print(data_name);
        Serial.print(": ");
        Serial.println(userState == 1 ? "ON" : "OFF");
      }
      else
      {
        Serial.println("Device status remained!");
      }
    }
    else
    {
      Serial.print("err:");
      Serial.println(httpResponceCode);
    }
    http.end();
  }
  else
  {
    Serial.println("wifi err");
  }
}

void readDHTData()
{
  // read humidity
  float humi = dht_sensor.readHumidity();
  // read temperature in Celsius
  float tempC = dht_sensor.readTemperature();
  // read temperature in Fahrenheit

  // check whether the reading is successful or not
  if (isnan(tempC) || isnan(humi))
  {
    Serial.println("Failed to read from DHT sensor!");
  }
  else if (humi != currentHumi || tempC != currentTempC)
  {
    currentHumi = humi;
    currentTempC = tempC;

    float heatIndex = dht_sensor.computeHeatIndex(tempC, humi, false);

    Serial.print("Humidity: ");
    Serial.print(humi);
    Serial.print("%");

    Serial.print("  |  ");

    Serial.print("Temperature: ");
    Serial.print(tempC);

    Serial.print("°C  ~  Heat Index: ");
    Serial.print(heatIndex);
    Serial.println("°C");

    // print message on lcd
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Humidity: ");
    lcd.print(humi);
    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.print(tempC);
    // delay(2000);

    // send data to server
    jsonDataPost(heatIndex, humi, tempC);
  }
  else
  {
    Serial.println("Sensor status remained!");
  }
}

void loop()
{
  unsigned long currentMillis = millis();
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval))
  {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
  httpGet();
  readDHTData();
}
