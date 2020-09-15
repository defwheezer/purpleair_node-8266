/**********************************************************************
 
 Interfacing ESP8266 NodeMCU with ILI9341 TFT display (240x320 pixel).
 https://simple-circuit.com/

// Color definitions
#define ILI9341_BLACK 0x0000       ///<   0,   0,   0
#define ILI9341_NAVY 0x000F        ///<   0,   0, 123
#define ILI9341_DARKGREEN 0x03E0   ///<   0, 125,   0
#define ILI9341_DARKCYAN 0x03EF    ///<   0, 125, 123
#define ILI9341_MAROON 0x7800      ///< 123,   0,   0
#define ILI9341_PURPLE 0x780F      ///< 123,   0, 123
#define ILI9341_OLIVE 0x7BE0       ///< 123, 125,   0
#define ILI9341_LIGHTGREY 0xC618   ///< 198, 195, 198
#define ILI9341_DARKGREY 0x7BEF    ///< 123, 125, 123
#define ILI9341_BLUE 0x001F        ///<   0,   0, 255
#define ILI9341_GREEN 0x07E0       ///<   0, 255,   0
#define ILI9341_CYAN 0x07FF        ///<   0, 255, 255
#define ILI9341_RED 0xF800         ///< 255,   0,   0
#define ILI9341_MAGENTA 0xF81F     ///< 255,   0, 255
#define ILI9341_YELLOW 0xFFE0      ///< 255, 255,   0
#define ILI9341_WHITE 0xFFFF       ///< 255, 255, 255
#define ILI9341_ORANGE 0xFD20      ///< 255, 165,   0
#define ILI9341_GREENYELLOW 0xAFE5 ///< 173, 255,  41
#define ILI9341_PINK 0xFC18        ///< 255, 130, 198
 **********************************************************************/
 
#include <Wire.h>                  // installed by default
                                   //check defwheezer: https://www.googleapis.com/youtube/v3/channels?part=statistics&id=UCfa6UutS3fmq8Csju8YQUsw&key=AIzaSyA0VUO5SvWPUTdotdbGHnIrDJEIh6BOgHM
#include <ArduinoJson.h>           // version: 5.2.0 - https://github.com/bblanchon/ArduinoJson

#include <ESP8266WiFi.h>           // version: 2.3.0
#include <WiFiClientSecure.h>
#include <NTPClient.h>             // to get time and date
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>


// Define NTP Client to get time
const long utcOffsetInSeconds = -25200;  //-8 hr offset from gmt, For UTC -8.00 : -8 * 60 * 60 : -28800
                                         //-7 hr offset from gmt for dlst, For UTC -7.00 : -7 * 60 * 60 : -25200
String formattedDate;
String dayStamp;
String timeStamp;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// for dsplay
#include <Adafruit_GFX.h>    // Core graphics library
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include <Adafruit_GFX.h>       // include Adafruit graphics library
#include <Adafruit_ILI9341.h>   // include Adafruit ILI9341 TFT library
#include <SPI.h> // needed?
 
#define TFT_CS    D2     // TFT CS  pin is connected to NodeMCU pin D2
#define TFT_RST   D3     // TFT RST pin is connected to NodeMCU pin D3
#define TFT_DC    D4     // TFT DC  pin is connected to NodeMCU pin D4
// initialize ILI9341 TFT library with hardware SPI module
// SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
// MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#define LED_pin    D8     // Blue Alert LED on top of arcade
#define SWITCH_Pin  D1   // "reset" switch on top of arcade (resets alert)
bool button_state;
//------- Replace the following! ------
//-------------------------------------
char ssid[] = "cow"; // your network SSID (name)
char password[] = "Noodler1";   // your network password

WiFiClientSecure client;

#define JSON_BUFF_DIMENSION 2500

#define PURPLE_AIR_SENSOR "/json?show=15981"

WiFiServer server(80);
int16_t aqi_color;

unsigned long api_mtbs = 120000*5; //mean time between api requests (120sec * 5 = 10 min)
unsigned long api_lasttime;     //last time api request has been done

float PM2_5_previous=0; // to check for change in count
float AQI_previous=0; // to check for change in count


float PM2_5_current=0; //count
float pm2_5_cf_1_current=0;
float AQI_current=0; //count


int dispVal = 0;

float p = 3.1415926;

void setup() {
  pinMode(LED_pin, OUTPUT);
  pinMode(SWITCH_Pin, INPUT);
  digitalWrite(LED_pin, LOW);
  button_state = 0; //led reset button has been pressed
    
  tft.begin();
  tft.setRotation(3);
  
  Serial.begin(115200);
  Serial.println("ILI9341 Test!"); 
  
  client.setInsecure();  //added to make youttubeapi work again
  // Set WiFi to station mode and disconnect from an AP if it was previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(0,10);
  tft.print("ESP8266: ");
  tft.setTextSize(2);
  tft.print(" NodeMCU");
  tft.setTextSize(3);
  tft.println(" V2");
  delay(1000);
  tft.setTextSize(3);
  tft.println("ILI9341 TFT");
  delay(1000);
  tft.println("240x320 pixel");
  delay(1000);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0,10);
  tft.println("Connecting Wifi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    tft.print(".");
    delay(500);
  }
  tft.println("");
  Serial.println("");
  Serial.println("WiFi connected!");
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0,10);
  tft.println("WiFi connected!");
  delay(1000);

  timeClient.begin();  // for interent time and date

  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0,10);
  tft.setTextSize(2);
  tft.println("NTPClient date/time");
  delay(1000);
  tft.setTextSize(3);
    
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0,0);
  
  //get time and date
  timeClient.update();
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  tft.print("Date: ");
  tft.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  tft.print("Time: ");
  tft.println(timeStamp);  
  delay(1000);
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0,10);
  tft.println("Test LED 3x");
  delay(1000);
  int i=0;
  for(i=0; i<3; i++){
    digitalWrite(LED_pin, HIGH); //test LED
    tft.println("Blink!");
    delay(100);
    digitalWrite(LED_pin, LOW);
    delay(100);
  }
  // final set-up screen
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0,0);
  gethttp_data(); //update youtube stats
  draw_data();
  //end setup
}

void gethttp_data() {
    //open a web page and read the data
  
    Serial.println("gethttp_data");
    // Connect to HTTP server
    WiFiClient client;
    client.setTimeout(10000);
    if (!client.connect("www.purpleair.com", 80)) {
      Serial.println(F("Connection failed"));
      return;
    }

    Serial.println(F("Connected!"));

    String url = String("GET " + String(PURPLE_AIR_SENSOR) + " HTTP/1.0");
    client.println(url);
    client.println(F("Host: www.purpleair.com"));
    client.println(F("Connection: close"));
    if (client.println() == 0) {
      Serial.println(F("Failed to send request"));
      return;
    }

    // Check HTTP status
    char status[32] = {0};
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.0 200 OK") != 0) {
      Serial.print(F("Unexpected response: "));
      Serial.println(status);
      return;
    }
  
    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
      Serial.println(F("Invalid response"));
      return;
    }

    // Allocate the JSON document
    // Use arduinojson.org/v6/assistant to compute the capacity.
    const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(30) + JSON_OBJECT_SIZE(39) + 1485;
    DynamicJsonDocument doc(capacity);
  
    // Parse JSON object
    DeserializationError error = deserializeJson(doc, client);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
  
    // Extract values
    Serial.println(F("Response:"));
    PM2_5_current = doc["results"][0]["PM2_5Value"].as<float>();
    pm2_5_cf_1_current = doc["results"][0]["pm2_5_cf_1"].as<float>();
    Serial.print("PM2_5Value: ");
    Serial.println(PM2_5_current, 6);
    Serial.print("pm2_5_cf_1_current: ");
    Serial.println(pm2_5_cf_1_current, 6);
    // Disconnect
    client.stop();
 }
 
unsigned draw_TimeDate() {
  timeClient.update();
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-4);
  tft.setTextColor(0xBDF7); //light grey
  tft.setTextSize(2); 
  tft.setCursor(4,222); //height = 240 - font 16 - border 2= 222
  tft.print(dayStamp);
  tft.setCursor(256,222);
  tft.println(timeStamp);
}

unsigned long drawBorder() {
  int x1, x2 = tft.width();
  int y1, y2 = tft.height();
  //Serial.println("Drawing border");
  tft.drawLine(x1, y1, x2, y1, ILI9341_PURPLE);
  tft.drawLine(x2, y1, x2, y2, ILI9341_PURPLE);
  tft.drawLine(x2, y2, x1, y2, ILI9341_PURPLE);
  tft.drawLine(x1, y2, x1, y1, ILI9341_PURPLE);
  x1 = 0+1;
  x2 = tft.width()-1;
  y1 = 0+1;
  y2 = tft.height()-1;
  int16_t data_color = getColorFromAQI(AQI_current);
  tft.drawLine(x1, y1, x2, y1, data_color);
  tft.drawLine(x2, y1, x2, y2, data_color);
  tft.drawLine(x2, y2, x1, y2, data_color);
  tft.drawLine(x1, y2, x1, y1, data_color);
}


unsigned draw_data() {
  //Serial.println("Drawing data");
  tft.setTextSize(4);

  tft.setCursor(80,30);            
  tft.setTextColor(ILI9341_WHITE);
  tft.println("PM 2.5");  

  tft.setCursor(88,65);
  int16_t data_color = getColorFromPM25(PM2_5_current);
  tft.setTextColor(data_color);
  tft.println(PM2_5_current);

  tft.setCursor(110,120);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("AQI");

  AQI_current = aqiFromPM(PM2_5_current);
  
  tft.setCursor(90,155);            
  data_color = getColorFromAQI(AQI_current);
  tft.setTextColor(data_color);
  tft.println(AQI_current);
}

void checkSwitch(){
  //switch change sensed
  bool state = digitalRead(D1);
  if(state) {
    digitalWrite(LED_pin, LOW); //reset LED to off
    button_state=1;
  }
}

void loop() {
  checkSwitch();
  drawBorder();
  draw_data();
  draw_TimeDate();
  if (millis() > api_lasttime + api_mtbs)  {
      
      gethttp_data();
   
      Serial.println("---------Stats---------");
      Serial.print("PM2_5_current: ");
      Serial.print(PM2_5_current);
      Serial.print(", PM2_5_previous: ");
      Serial.println(PM2_5_previous);
      Serial.print("AQI_current: ");
      Serial.print(AQI_current);
      Serial.print(", AQI_previous: ");
      Serial.println(AQI_previous);
      if(AQI_current <= 30 && !button_state) {
        Serial.println("air OK, Light the LED!!!");
        digitalWrite(LED_pin, HIGH);
        button_state=0;
      }
      if(AQI_current >= 35) {
        digitalWrite(LED_pin, LOW);
        button_state=0;
      }
      //draw_Inoculation();
      tft.fillScreen(ILI9341_BLACK);
      draw_data();
      draw_TimeDate();
      api_lasttime = millis();
    }
}


// Update ths routine if you like different colors for different PM 2.5 levels
// current mapping taken from https://smartairfilters.com/en/blog/difference-pm2-5-aqi-measurements/
int16_t getColorFromPM25( float pm25) {
  int pm25int = int(pm25);

  if (pm25int <= 12) {
    aqi_color = ILI9341_GREEN;
    return aqi_color;        // green
  }
  if (pm25int <= 36) {
    aqi_color = ILI9341_YELLOW;
    return aqi_color;      // moderate / yellow
  }

  if (pm25int <= 56) {
    aqi_color = ILI9341_ORANGE;
    return aqi_color;     // dark yellow
  }
  if (pm25int <= 150) {
    aqi_color = ILI9341_RED;
    return aqi_color;      // red
  }

  if (pm25int <= 250) {
    aqi_color = ILI9341_PURPLE;
    return aqi_color;   // purple
  }


  if (pm25int <= 500) {
    aqi_color = ILI9341_MAROON;
    return aqi_color;   // maroon
  }


  if (pm25int <= 1000) {
    aqi_color = ILI9341_DARKCYAN;
    return aqi_color;   // turqoise (self selected color for over 500 < 1000
  }

  // got value over 1000, this doesn't make sense, return black
    aqi_color = ILI9341_WHITE;
    return aqi_color;   // pinkish
}

int16_t getColorFromAQI( float aqi) {
  int aqiInt = int(aqi);

  if (aqiInt <= 50) {
    aqi_color = ILI9341_GREEN;
    return aqi_color;        // green
  }


  if (aqiInt <= 100) {
    aqi_color = ILI9341_YELLOW;
    return aqi_color;      // moderate / yellow
  }

  if (aqiInt <= 100) {
    aqi_color = ILI9341_ORANGE;
    return aqi_color;     // dark yellow
  }
  if (aqiInt <= 150) {
    aqi_color = ILI9341_RED;
    return aqi_color;      // red
  }

  if (aqiInt <= 200) {
    aqi_color = ILI9341_PURPLE;
    return aqi_color;   // purple
  }


  if (aqiInt <= 300) {
    aqi_color = ILI9341_MAROON;
    return aqi_color;   // maroon
  }


  if (aqiInt <= 400) {
    aqi_color = ILI9341_DARKCYAN;
    return aqi_color;   // turqoise (self selected color for over 500 < 1000
  }

  // got value over 400, we are fucked
    aqi_color = ILI9341_WHITE;
    return aqi_color;   // pinkish
}

float aqiFromPM(float pm) {
  // pm is pm2_5
  if (pm > 350.5) {
        return calcAQI(pm, 500, 401, 500, 350.5);
      } 
  else if (pm > 250.5) {
        return calcAQI(pm, 400, 301, 350.4, 250.5);
      } 
  else if (pm > 150.5) {
        return calcAQI(pm, 300, 201, 250.4, 150.5);
      } 
  else if (pm > 55.5) {
        return calcAQI(pm, 200, 151, 150.4, 55.5);
      } 
  else if (pm > 35.5) {
        return calcAQI(pm, 150, 101, 55.4, 35.5);
      } 
  else if (pm > 12.1) {
        return calcAQI(pm, 100, 51, 35.4, 12.1);
      } 
  else if (pm >= 0) {
        return calcAQI(pm, 50, 0, 12, 0);
      } 
  else {
       Serial.println("no pm value to calculate");
   }
}

float calcAQI(float Cp, int Ih, int Il, float BPh, float BPl) {
      
        float a = (Ih - Il);
        float b = (BPh - BPl);
        float c = (Cp - BPl);
        float AQI = ((a/b) * c + Il);
        return AQI;
      }
