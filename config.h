#include "AdafruitIO_WiFi.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include "Adafruit_SHT31.h"
#include <Wire.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
#define SDA_PIN 12 //GPIO12 / 6
#define SCL_PIN 14 //GPIO14 / 5

// Relay 1
#define FAN_RELAY  D0 // blau
// Relay 2
#define HEAT_RELAY D7  // orange
// Relay 3
#define HUM_RELAY  D8 // gelb
// Relay 4
#define LIGHT_RELAY D4 // weiß

// Light Times
// 8:00 - 20:00
const int lightBegin = 8;
const int lightEnd = 20;

const int fanMinutes = 15;

// Values for automation
const float TARGET_TEMP = 28.00f;
const float MIN_TEMP = 27.00f;
const float TARGET_HUMIDITY = 80.00f;
const float MIN_HUMIDITY = 70.00f;


#define IO_USERNAME  ""
#define IO_KEY       ""

#define WIFI_SSID "QuasiMentum"
#define WIFI_PASS "KAckN00bY0W"

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
