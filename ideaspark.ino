#include "config.h"

AdafruitIO_Feed *temperatureFeed = io.feed("Temperature");
AdafruitIO_Feed *humidityFeed = io.feed("Humidity");
AdafruitIO_Feed *fanFeed = io.feed("Fan");
AdafruitIO_Feed *humidifierFeed = io.feed("Humidifier");
AdafruitIO_Feed *lightFeed = io.feed("Light");
AdafruitIO_Feed *heatFeed = io.feed("Heat");

const long utcOffsetInSeconds = 3600;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

bool fanOn = false;
bool humOn = false;
bool lightOn = false;
bool heatOn = false;
int hours = 0;
int minutes = 0;
int seconds = 0;

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL_PIN, SDA_PIN, U8X8_PIN_NONE);
Adafruit_SHT31 sht31 = Adafruit_SHT31();

void setup(void) {
  pinMode(FAN_RELAY, OUTPUT);
  pinMode(HUM_RELAY, OUTPUT);
  pinMode(LIGHT_RELAY, OUTPUT);
  pinMode(HEAT_RELAY, OUTPUT);
  digitalWrite(FAN_RELAY, LOW);
  digitalWrite(HEAT_RELAY, LOW);
  digitalWrite(HUM_RELAY, LOW);
  digitalWrite(LIGHT_RELAY, HIGH);
  // Init serial 
  //Serial.begin(115200);
  // Init display
  u8g2.begin();

  // wait for serial monitor to open
  //while(! Serial);
  drawConnectionStatus(0, 10, "Connecting to Adafruit", false);

  // connect to io.adafruit.com
  bool connected = connectAdafruit();

  delay(500);
  drawConnectionStatus(0, 10, "Connecting to NTP", true);
  
  delay(500);
  timeClient.begin();

  // Init the temp / hum sensor
  drawConnectionStatus(0, 10, "Init Sensor", true);
  sht31.begin(0x44);
  delay(500);

  drawConnectionStatus(0, 10, "Init Relay", true);

  // test all relays
  delay(500);
  drawConnectionStatus(0, 10, "FAN Relay", true);
  digitalWrite(FAN_RELAY, HIGH);
  
  delay(500);
  drawConnectionStatus(0, 10, "HEAT Relay", true);
  digitalWrite(HEAT_RELAY, HIGH);
  
  delay(500);
  drawConnectionStatus(0, 10, "HUM Relay", true);
  digitalWrite(HUM_RELAY, HIGH);
  
  delay(500);
  drawConnectionStatus(0, 10, "LIGHT Relay", true);
  digitalWrite(LIGHT_RELAY, LOW);

  delay(500);
  digitalWrite(FAN_RELAY, LOW);
  digitalWrite(HUM_RELAY, LOW);
  digitalWrite(LIGHT_RELAY, HIGH);
  digitalWrite(HEAT_RELAY, LOW);
  delay(500);
}

bool connectAdafruit() {
  Serial.print("Connecting to Adafruit IO");

  io.connect();
  bool connected = true;
  // wait for a connection
  int x = 0;
  while(io.status() < AIO_CONNECTED) {
    x = x+5;
    drawConnectionStatus(x, 25, ".", false);

    Serial.print(".");
    if (x > 150) {
      Serial.println("Connection Error");
      connected = false;
      break;
    }
    delay(500);
  }
  if (connected) {
    drawConnectionStatus(0, 10, "Connected!", true);
  } else {
    drawConnectionStatus(0, 10, "Connection failed", true);
  }
  return connected;
}

String getStatusString(bool status) {
  if (status) {
    return "an";
  } else {
    return "aus";
  }
}
void loop(void) {
  timeClient.update();

  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  seconds =  timeClient.getSeconds();
  float temp = sht31.readTemperature();
  float humidity = sht31.readHumidity();
  bool humidifierUpdated = updateHumidifier(humidity);
  bool heaterUpdated = updateHeater(temp);
  bool lightUpdated = updateLight(hours);
  bool fanUpdated = updateFan(minutes);
  updateFeeds(temp, humidity, heaterUpdated, humidifierUpdated, lightUpdated, fanUpdated);
  drawDisplay(temp, humidity);
  delay(5000);  
}

void updateFeeds(float temp, 
    float humidity, 
    bool heaterUpdated, 
    bool humidifierUpdated, 
    bool lightUpdated, 
    bool fanUpdated) {
   
  String sensorData = "T: " + String(temp) + " - " + "H: " + String(humidity) + "  " + getTimeString();
  drawConnectionStatus(0, 10, sensorData.c_str(), true);

  drawConnectionStatus(0, 25, "Update feeds", false);
  bool temperatureSended = temperatureFeed->save(temp);
  bool humiditySended = humidityFeed->save(humidity);
  drawConnectionStatus(0, 37, "Temp/Hum feed updated", false);
  drawConnectionStatus(0, 49, "Relay status updated", false);
  bool heatSended = true;
  bool humidifierSended = true;
  bool lightSended = true;
  bool fanSended = true;
  if (heaterUpdated) {
    heatFeed->save(getBoolValue(heatOn));
  }
  if (humidifierUpdated) {
    humidifierFeed->save(getBoolValue(humOn));
  }
  if (lightUpdated) {
    lightFeed->save(getBoolValue(lightOn));
  }
  if (fanUpdated) {
    fanFeed->save(getBoolValue(fanOn));
  }
  if (temperatureSended && humiditySended && heatSended && humidifierSended && lightSended && fanSended) {
    drawConnectionStatus(0, 62, "All feeds updateted :)", false);
    delay(1500);  
  } else {
    drawConnectionStatus(0, 62, "Error on feed update :(", false);
    delay(1500);  
    connectAdafruit();
  }

}

int getBoolValue(bool value) {
  if (value) {
    return 1;
  } else {
    return 0;
  }
}

bool updateHeater(float temp) {
  bool valueChanged = false;
  if (temp < MIN_TEMP) {
    if (!heatOn) {
      // we use inverse relay here
      digitalWrite(HEAT_RELAY, HIGH);
      heatOn = true;
      valueChanged = true;
    }
  } else if (temp > TARGET_TEMP){
    if (heatOn) {
      digitalWrite(HEAT_RELAY, LOW);
      heatOn = false;
      valueChanged = true;
    }
  }  
  return valueChanged;
}


bool updateHumidifier(float humidity) {
  bool valueChanged = false;
  if (humidity < MIN_HUMIDITY) {
    if (!humOn) {
      digitalWrite(HUM_RELAY, HIGH);
      humOn = true;
      valueChanged = true;
    }
  } else if (humidity > TARGET_HUMIDITY) {
    if (humOn) {
      digitalWrite(HUM_RELAY, LOW);
      humOn = false;
      valueChanged = true;
    }
  }
  return valueChanged;
}

bool updateFan(int minutes) {
  bool valueChanged = false;
  if (minutes <= fanMinutes) {
    if(!fanOn) {
      digitalWrite(FAN_RELAY, HIGH);
      fanOn = true;
      valueChanged = true;
    }
  } else {
     if (fanOn) {
      digitalWrite(FAN_RELAY, LOW);
      fanOn = false;
      valueChanged = true;
    }
  }
  return valueChanged;
}

bool updateLight(int hours) {
  bool valueChanged = false;
  if ((hours > lightBegin-1) && (hours < lightEnd+1)) {
    if (!lightOn) {
      digitalWrite(LIGHT_RELAY, LOW);
      lightOn = true;
      valueChanged = true;
    }
  } else {
    if (lightOn) {
      digitalWrite(LIGHT_RELAY, HIGH);
      lightOn = false;
      valueChanged = true;
    }
  }
  return valueChanged;
}

void drawConnectionStatus(int x, int y, String text, bool clearDisplay) {
  if (clearDisplay) {
    u8g2.clearBuffer();
  }
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(x, y, text.c_str());
  u8g2.sendBuffer();
}

String getTimeString() {
  String minutesString = String(minutes);
  if (minutesString.length() == 1) {
    minutesString = "0" + minutesString;
  }
  String timeString = String(hours) + ":" + String(minutesString);
  return timeString;
}

void drawDisplay(float temp, float humidity) {
  String sensorData = "T: " + String(temp) + " - " + "H: " + String(humidity) + "  " + getTimeString();
  String relayLabelFan = "Ventilator:";
  String relayLabelHumidifier = "Luftbefeuchter: ";
  String relayLabelLight = "Licht:";
  String relayLabelHead = "Heizung:";

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,10, sensorData.c_str());
  // Labels
  u8g2.drawStr(0,25,relayLabelHead.c_str());
  u8g2.drawStr(0,37,relayLabelHumidifier.c_str());
  u8g2.drawStr(0,49,relayLabelLight.c_str());
  u8g2.drawStr(0,62,relayLabelFan.c_str());
  // Data
  u8g2.drawStr(110,25,getStatusString(heatOn).c_str());
  u8g2.drawStr(110,37,getStatusString(humOn).c_str());
  u8g2.drawStr(110,49,getStatusString(lightOn).c_str());
  u8g2.drawStr(110,62,getStatusString(fanOn).c_str());

  u8g2.sendBuffer();
}
