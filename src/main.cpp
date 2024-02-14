#include <Arduino.h>
#include <sstream>
#include <WiFi.h>
#include "PubSubClient.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ticker.h>

#define DEFAULT_SERIAL_BAUD 115200

#define ONE_WIRE_BUS_PIN 4

OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);

#define MEASUREMENT_INTERVAL_MS 30000
Ticker ticker;

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

void wifi_client_setup_dhcp(char* wifi_ssid, char* wifi_password, bool auto_reconnect = true) {
    Serial.println("<WiFi> Setup");

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(auto_reconnect);

    Serial.println("<WiFi> Connecting...");

    WiFi.begin(wifi_ssid, wifi_password);

    auto start_time = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start_time) <= 30000) {
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("<WiFi> Connection established!");
        Serial.println(WiFi.localIP());
        Serial.println(WiFi.localIPv6());
        Serial.println("<WiFi> Enabled");
    } else {
        Serial.println("<WiFi> Connection failed!");
        Serial.println(WiFi.status());
        Serial.println("<WiFi> Failed. Restarting...");
        delay(2500);
        ESP.restart();
    }
}

#define PUBSUB_SERVER_ADDRESS IPAddress(0, 0, 0, 0)
#define PUBSUB_SERVER_PORT 1883
#define PUBSUB_USERNAME "temperature-probe"
#define PUBSUB_PASSWORD ""
#define PUBSUB_CLIENT_ID "temperature-probe"
#define PUBSUB_TOPIC "temperature-probe"

WiFiClient wifi_client;
PubSubClient pub_sub_client(wifi_client);

void mqtt_setup(IPAddress server_ip, int server_port, char* username, char* password, char* client_id) {
    Serial.println("<MQTT> Setup");

    Serial.println("<MQTT> Connecting...");
    pub_sub_client.setServer(server_ip, server_port);
    boolean connected = pub_sub_client.connect(client_id, username, password);

    if (connected) {
        Serial.println("<MQTT> Connection established!");
    } else {
        Serial.println("<MQTT> Connection failed!");
        Serial.println(pub_sub_client.state());
        Serial.println("<MQTT> Failed. Restarting...");

        delay(2500);
        ESP.restart();
    }
}

bool mqtt_publish(char* topic, char* content) {
    if (!pub_sub_client.connected()) {
        Serial.println("<MQTT> Not Connected. Restarting...");

        delay(2500);
        ESP.restart();
    }

    return pub_sub_client.publish(topic, content);
}

std::string get_unique_device_id() {
    std::stringstream stringStream;
    stringStream << std::hex << ESP.getEfuseMac();
    return stringStream.str();
}

std::string json_sensor_payload(int index, float celsius, float fahrenheit) {
    std::stringstream payload = std::stringstream();
    payload
        << R"({"device_id":")" << get_unique_device_id()
        << R"(","sensor_id":)" << std::to_string(index)
        << R"(,"celsius":)" << std::to_string(celsius)
        << R"(,"fahrenheit":)" << std::to_string(fahrenheit)
        << R"(})";
    return payload.str();
}

void read_and_publish_temperature() {
    Serial.println("<Measurement> Measuring Temperature...");
    sensors.setWaitForConversion(true);
    sensors.requestTemperatures();
    for (int index = 0; index < sensors.getDS18Count(); index++) {
        float tempC = sensors.getTempCByIndex(index);
        float tempF = sensors.getTempFByIndex(index);
        std::string payload = json_sensor_payload(index, tempC, tempF);
        Serial.println(("<Measurement> " + payload).c_str());
        mqtt_publish((char*) PUBSUB_TOPIC,(char*) payload.c_str());
    }
}

void setup() {
    delay(2500);
    // setup serial out
    Serial.begin(DEFAULT_SERIAL_BAUD);
    while (!Serial)
        delay(100);
    Serial.println("<Serial> Enabled");
    // setup sensors
    sensors.begin();
    Serial.println("<Sensors> Enabled");
    // setup wifi
    wifi_client_setup_dhcp((char*) WIFI_SSID, (char*) WIFI_PASSWORD);
    // setup mqtt
    mqtt_setup(PUBSUB_SERVER_ADDRESS, PUBSUB_SERVER_PORT, (char*) PUBSUB_USERNAME, (char*) PUBSUB_PASSWORD, (char*) PUBSUB_CLIENT_ID);
    // initial data
    read_and_publish_temperature();
    // setup ticker
    ticker.attach_ms(MEASUREMENT_INTERVAL_MS, read_and_publish_temperature);
}

void loop() {
    pub_sub_client.loop();
}