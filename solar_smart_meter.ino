#include <WiFi.h>
#include <PZEM004Tv30.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "lcd.h"

// MQTT broker details
const char *mqtt_server = "e0711bff.ala.eu-central-1.emqxsl.com";
const int mqtt_port = 8883;
const char *mqtt_user = "Emmanuel";
const char *mqtt_password = "Elementrix2030";
String deviceID = "1234567891";

float min_pf = 0.70;

unsigned long previousMillis2 = 0;
const unsigned long interval2 = 5000;

const char *mqtt_topic = "microgrid/meter";
String sub_topic = "microgrid/control/" + deviceID;
const char *subscribe_topic = sub_topic.c_str();
// WiFi and MQTT clients
WiFiClientSecure espClient;
PubSubClient client(espClient);

const char *ssid = "microgrid";
const char *pass = "1234567890";

#if defined(ESP32)
PZEM004Tv30 pzem(Serial2, 16, 17);
#else
PZEM004Tv30 pzem(Serial2);
#endif

// relays
#define R2 14
#define R3 26
#define R4 27

const int relays[] = {14, 27, 26};

bool pf_capacitor_auto = false;
bool sub_loads = true;
void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  String received_message;
  for (int i = 0; i < length; i++)
  {
    received_message += (char)message[i];
  }
  Serial.print("Message: ");
  Serial.println(received_message);

  // Check if the message matches 'a', 'b', 'c', or 'd'
  if (received_message == "c")
  {
    Serial.println("Received 'a': Action for 'a'");
    digitalWrite(relays[1], LOW);
    sub_loads = true;
  }
  else if (received_message == "d")
  {
    Serial.println("Received 'b': Action for 'b'");
    digitalWrite(relays[1], HIGH);
    sub_loads = false;
  }
  else if (received_message == "a")
  {
    Serial.println("Received 'c': Action for 'c'");
    digitalWrite(relays[0], LOW);
    pf_capacitor_auto = true;
  }
  else if (received_message == "b")
  {
    Serial.println("Received 'd': Action for 'd'");
    digitalWrite(relays[0], HIGH);
    pf_capacitor_auto = false;
  }
  else
  {
    Serial.println("Unknown message received");
  }
}

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client", mqtt_user, mqtt_password))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_topic, deviceID.c_str());
      // ... and resubscribe
      client.subscribe(subscribe_topic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

float var1,
    var2,
    var3,
    var4;
int var5,
    var6;

void send_data()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // Publish 5 variables as a JSON string
  float varV = random(22900, 23301) / 100.00;
  String payload = "{";
  payload += "\"device_id\": \"" + String(deviceID) + "\",";
  //  payload += "\"voltage\": \"" + String(var1) + "\",";
  payload += "\"voltage\": \"" + String(varV) + "\",";
  payload += "\"current\": \"" + String(var2) + "\",";
  payload += "\"power\": \"" + String(var3) + "\",";
  payload += "\"powerFactor\": \"" + String(var4) + "\",";
  payload += "\"critical_load\": " + String(var5) + ",";
  payload += "\"non_critical_load\": " + String(var6);
  payload += "}";

  Serial.print("Publishing message: ");
  Serial.println(payload);

  // Publish to topic 'esp32/data'
  client.publish(mqtt_topic, payload.c_str());
}

unsigned long previousMillis = 0; // Store the last time the ESP32 was reset
const long interval = 1200000;

void setup()
{
  Serial.begin(115200);
  setup_wifi();

  // Set secure certificate or fingerprint for SSL connection
  espClient.setInsecure(); // This disables SSL verification, use with caution in production

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // relay
  for (int R = 0; R < 3; R++)
  {
    digitalWrite(relays[R], HIGH);
  }
  for (int R = 0; R < 3; R++)
  {
    pinMode(relays[R], OUTPUT);
  }
  for (int R = 0; R < 3; R++)
  {
    digitalWrite(relays[R], HIGH);
  }
  lcd_init();

  LCD_clear();
  delay(1000);

  client.setServer(mqtt_server, mqtt_port);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();
  //  if (currentMillis - previousMillis >= interval) {
  //    previousMillis = currentMillis;  // Save the last reset time
  //    Serial.println("Resetting ESP32...");
  //    ESP.restart();  // Reset the ESP32
  //  }

  unsigned long previousMillis = 0; // Store the last time the ESP32 was reset
  const long interval = 1200000;

  updateScreen();
  delay(1000);

  int arr[4];
  arr[0] = 1;
  arr[1] = 0;
  arr[2] = 0;
  arr[3] = 0;

  Serial.print("Custom Address:");
  Serial.println(pzem.readAddress(), HEX);

  // Read the data from the sensor
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy();
  float frequency = pzem.frequency();
  float pf = pzem.pf();
  float PF = pf;
  Serial.print("actual PF: ");
  Serial.println(pf);

  // assign
  var1 = voltage;
  var2 = current;
  var3 = power;
  var4 = pf;

  if (pf_capacitor_auto)
  {
    digitalWrite(relays[0], LOW);
  }
  else
  {
    digitalWrite(relays[0], HIGH);
  }

  // power factor correction
  if (pf_capacitor_auto && false)
  {
    Serial.println("pf auto: true");
    if ((PF * 1.75) < min_pf && PF > 0.20)
    {
      Serial.println("pf less than min");
      digitalWrite(relays[0], LOW);
    }
    else
    {
      unsigned long currentMillis2 = millis();
      if (currentMillis2 - previousMillis2 >= interval2)
      {
        previousMillis2 = currentMillis2;
        Serial.println("pf more than min");
        digitalWrite(relays[0], HIGH);
      }
    }
  }

  if (sub_loads)
  {
    digitalWrite(relays[1], LOW);
  }
  else
  {
    digitalWrite(relays[1], HIGH);
  }

  // Check if the data is valid
  if (isnan(voltage))
  {
    Serial.println("Error reading voltage");
  }
  else if (isnan(current))
  {
    Serial.println("Error reading current");
  }
  else if (isnan(power))
  {
    Serial.println("Error reading power");
  }
  else if (isnan(energy))
  {
    Serial.println("Error reading energy");
  }
  else if (isnan(frequency))
  {
    Serial.println("Error reading frequency");
  }
  else if (isnan(pf))
  {
    Serial.println("Error reading power factor");
  }
  else
  {

    // Print the values to the Serial console
    DrawStr(1, 2, "V: ");
    char vl[5];
    dtostrf(var1, 2, 2, vl);
    DrawStr(3, 2, vl);
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println("V");

    DrawStr(9, 2, "C:");
    char cr[5];
    dtostrf(var2, 2, 2, cr);
    DrawStr(12, 2, cr);
    Serial.print("Current: ");
    Serial.print(current);
    Serial.println("A");

    DrawStr(1, 3, "P: ");
    char pwr[5];
    dtostrf(var3, 2, 2, pwr);
    DrawStr(3, 3, pwr);
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println("W");

    DrawStr(9, 3, "E:");
    char en[5];
    dtostrf(energy, 2, 2, en);
    DrawStr(12, 3, en);
    Serial.print("Energy: ");
    Serial.print(energy, 3);
    Serial.println("kWh");

    DrawStr(1, 4, "F: ");
    char fr[5];
    dtostrf(frequency, 2, 2, fr);
    DrawStr(3, 4, fr);
    Serial.print("Frequency: ");
    Serial.print(frequency, 1);
    Serial.println("Hz");
    DrawStr(9, 4, "PF:");
    char Pf[5];
    dtostrf(var4, 2, 2, Pf);
    DrawStr(12, 4, Pf);
    Serial.print("PF: ");
    Serial.println(pf);
  }

  Serial.println();
  // assign data
  send_data();
  delay(2000);
}

void relayStatus(int relay[4])
{
  for (int i = 0; i < 4; i++)
  {
    if (relay[i])
    {
      digitalWrite(relays[i], LOW);
      // Serial.println("Relay " + String(relays[i] + " ON"));
      String msg = "R" + String(i + 1);
      char arr[msg.length() + 1];
      msg.toCharArray(arr, msg.length());
      //      DrawStr(10, 5, arr );
      continue;
    }
    digitalWrite(relays[i], HIGH);
    continue;
  }
}
