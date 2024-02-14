# MQTT temperature probe  
## Required Parts
- ESP32
- DS18B20 or other Dallas Temperature IC Sensor

## Configuration  
The following values need to be adjusted in main.cpp

`WIFI_SSID` WiFi SSID  
`WIFI_PASSWORD` WiFi password  
`PUBSUB_SERVER_ADDRESS` IPv4 of the MQTT server  
`PUBSUB_SERVER_PORT` Port of the MQTT server  
`PUBSUB_USERNAME` MQTT username  
`PUBSUB_PASSWORD` MQTT password  
`PUBSUB_CLIENT_ID` MQTT client id  

## Default behaviour
Publishes temperature readouts in celsius and fahrenheit every 30 seconds as json payload to topic `temperature-probe`  
```
{
  "device_id": "<device mac id>",           // unique esp identifier
  "sensor_id": <int>,                       // id of the sensor
  "celsius": <double>,                      // temperature measurement in celsius
  "fahrenheit": <double>                    // temperature measurement in fahrenheit
}
```
