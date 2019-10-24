// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include "settings.h"
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "FS.h"

#include "SoftwareSerial.h"

#include <FastLED.h>

int delayval = 10; // delay for half a second

//chevron
CRGB leds[NUM_LEDS];
#define FASTLED_SHOW_CORE 0
#define BRIGHTNESS 255

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001008)
#warning "Requires FastLED 3.1.8 or later; check github for latest code."
#endif

// choose the ntp server that serves your old_currentzone
#define NTP_OFFSET 2 * 60 * 60          // In seconds
#define NTP_INTERVAL 60 * 1000          // In miliseconds
#define NTP_ADDRESS "0.fr.pool.ntp.org" //  NTP SERVER

WiFiUDP ntpUDP;
NTPClient old_currentClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

//old_currentr
unsigned long old_current;

//
uint8_t state = 98;

void setup()
{
        Serial.begin(115200);
        SPIFFS.begin();

        // We start by connecting to a WiFi network
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println(ssid);
        // Wifi with
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        while (WiFi.waitForConnectResult() != WL_CONNECTED)
        {
                Serial.println("Connection Failed! Rebooting...");
                FastLED.delay(500);
                ESP.restart();
        }
        setupOTA();
        initSettings();

        // start server and old_current client
        server.begin();
        old_currentClient.begin();
        //led
        //Initialize the lib for the ledstrip
        FastLED.addLeds<WS2812B, DATA_PIN_LEDS, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
        FastLED.setBrightness(BRIGHTNESS);
        FastLED.setDither(0);
        ClearAllLedData();
        FastLED.show();
        /**/
        old_current = millis();
}

void loop()
{
        unsigned long current;
        // if OTA called we need this
        ArduinoOTA.handle();
        WiFiClient client = server.available(); // listen for incoming clients
        if (client)
        { // If a new client connects,
                clientRequest(client);
        }
        if (state == 99)
        {
                old_current = millis();
                state = 0;
        }
        current = millis();
        /**/
        /*
        for(int i=0;i<100;i++){
                setPixel(i,255,0,0);
                FastLED.show();
                FastLED.delay(200);
        }
        */
        /**/
        if ((state == 0) && (current - old_current < 2000))
        {
                poweron();
                state = 1;
        }
        else if ((state == 1) && (current - old_current >= 2000))
        {
                poweroff();
                state = 2;
        }
        else if ((state == 2) && (current - old_current >= 3000))
        {
                glowFast();
                powerOnHandle(1);
                powerOnHandle(1);
                state = 99;
        }
        else if (state == 50)
        {
                poweron();
        }
        else if (state == 51)
        {
                poweroff();
        }
}

/*******
 * 
 */

void poweron()
{
        // For the first 14 leds, make them orange, starting from pixel number 0.
        for (int i = 0; i < Aleds; i++)
        {
                setPixel(i, 255, 50, 0); // Set leds to Orange Color
                FastLED.show();          // This sends the updated pixel color to the hardware.
                FastLED.delay(delayval);         // Delay for a period of old_current (in milliseconds).
        }

        // Fill up 84 leds with blue, starting with pixel number 14.
        for (int i = Aleds; i < Bleds; i++)
        {
                setPixel(i, 0, 250, 200); // Set leds to Blue Color
                FastLED.show();           // This sends the updated pixel color to the hardware.
                FastLED.delay(delayval);          // Delay for a period of old_current (in milliseconds).
        }

        // Fill up 9 leds with orange, starting from pixel number 84.
        for (int i = Bleds; i < Cleds; i++)
        {
                setPixel(i, 250, 50, 0); //Set leds to Orange Color
                FastLED.show();          // This sends the updated pixel color to the hardware.
                FastLED.delay(delayval);         // Delay for a period of old_current (in milliseconds).
        }
}

void poweroff()
{
        for (int i = Cleds - 1; i >= Bleds; i--)
        {
                setPixel(i, 0, 0, 0); //Set leds to Orange Color
                FastLED.show();       // This sends the updated pixel color to the hardware.
                FastLED.delay(delayval);      // Delay for a period of old_current (in milliseconds).
        }
        for (int i = Bleds - 1; i >= Aleds; i--)
        {
                setPixel(i, 0, 0, 0); // Set leds to Blue Color
                FastLED.show();       // This sends the updated pixel color to the hardware.
                FastLED.delay(delayval);      // Delay for a period of old_current (in milliseconds).
        }

        for (int i = Aleds - 1; i >= 0; i--)
        {
                setPixel(i, 0, 0, 0); // Set leds to Orange Color
                FastLED.show();       // This sends the updated pixel color to the hardware.
                FastLED.delay(delayval);      // Delay for a period of old_current (in milliseconds).
        }
}

void powerOnHandle(bool up)
{
        uint8_t y = 3;
        uint8_t z = 2;
        uint8_t N = 6;

        for (int8_t n = (up ? -N : Aleds); (up ? n < Aleds + 1 : n >= -N); (up ? n++ : n--))
        {
                for (uint8_t x = 0; x < Aleds / 2; x++)
                {
                        uint8_t hn = n + (N - x);
                        if (hn < Aleds)
                        {
                                setColor(hn, ipow(x, y), ipow(x, z), 0);
                        }
                        uint8_t tn = (Aleds / 2 - n) + (N - x) + Bleds;
                        if (tn >= Bleds && tn < Cleds)
                        {
                                setColor(tn, ipow((N - x), y), ipow((N - x), z), 0);
                        }
                }
                FastLED.show();
                FastLED.delay(30);
        }
}

void glowFast()
{
        uint8_t powered = 0;
        uint16_t run = 1000;
        for (uint16_t i = 0; i <= run; i++)
        {
                for (uint16_t j = 0; j <= 255; j++)
                {
                        uint8_t n = (j + i) % SwordUp;
                        if (n <= powered)
                        {
                                uint8_t s = round(((fastSin(j / 2) + 1) / 2) * 255);
                                uint8_t t = round(((fastSin(j / 10) + 1) / 2) * 200);
                                setColor(n, 255, s, t);
                                setColor(Cleds - n, 255, s, t);
                        }
                }
                if (i == run - SwordUp)
                {
                        powered--;
                }
                if (i >= run - SwordUp)
                {
                        setColor((SwordUp - (i - (run - SwordUp))), 0, 0, 0);
                        setColor(Cleds - (SwordUp - (i - (run - SwordUp))), 0, 0, 0);
                        powered--;
                }
                else if (powered <= SwordUp)
                {
                        powered++;
                }
                FastLED.show();
                FastLED.delay(0);
        }
}

int ipow(int base, int exp)
{
        int result = 1;
        while (exp)
        {
                if (exp & 1)
                        result *= base;
                exp >>= 1;
                base *= base;
        }

        return max(min(result, 255), 0);
}

void setColor(int8_t n, int8_t p, int8_t s, int8_t t)
{
        if ((n >= 0 && n <= 19) || (n >= 86 && n <= 89))
        {
                setPixel(n, p, t, s);
        }
        else if (n >= 20 && n <= 85)
        {
                setPixel(n, t, s, p);
        }
}

#define API 3.1415926
#define ATU 6.2831853
#define M1 1.2732395
#define M2 0.4052847

double fastSin(double x)
{
        if (x < -API)
                x += ATU;
        else if (x > API)
                x -= ATU;

        //compute sine
        if (x < 0)
                return M1 * x + M2 * x * x;
        else
                return M1 * x - M2 * x * x;
}

/****************/

void initSettings()
{
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.println("Place this IP address into a browser window");
        // make an ip address you can read
        IPAddress myIP = WiFi.localIP();
        ipStr = String(myIP[0]) + "." + String(myIP[1]) + "." + String(myIP[2]) + "." + String(myIP[3]);
}

/*******************/

void setupOTA()
{
        ArduinoOTA.setHostname("ESP32_Stepper"); // Hostname for OTA
        ArduinoOTA.setPassword(my_OTA_PW);       // set in credidentials.h
        ArduinoOTA
            .onStart([]() {
                    String type;
                    if (ArduinoOTA.getCommand() == U_FLASH)
                            type = "sketch";
                    else // U_SPIFFS
                            type = "filesystem";

                    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                    Serial.println("Start updating " + type);
            });
        ArduinoOTA.onEnd([]() {
                Serial.println("\nEnd");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR)
                        Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR)
                        Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR)
                        Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR)
                        Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR)
                        Serial.println("End Failed");
        });

        ArduinoOTA.begin(); // Start OTA
}

/*****************************/

/*** lEDS ****/
//Clears the data for all configured ledstrip
void ClearAllLedData()
{
        for (word ledNr = 0; ledNr < NUM_LEDS; ledNr++)
        {
                setPixel(ledNr, 0, 0, 0);
        }
        FastLED.show();
}

void fillAll(int r, int g, int b)
{
        ClearAllLedData();
        FastLED.show();
        for (uint32_t i = 0; i < NUM_LEDS; i++)
        {
                setPixel(i, r, g, b);
        }
        FastLED.show();
}

void setPixel(int num, int r, int g, int b)
{
        leds[num].setRGB(r, g, b);
}

/*****************************/

void clientRequest(WiFiClient client)
{
        Serial.println("==========================");
        Serial.println("New Client."); // print a message out in the serial port
        String currentLine = "";       // make a String to hold incoming data from the client
        while (client.connected())
        { // loop while the client's connected
                if (client.available())
                {                               // if there's bytes to read from the client,
                        char c = client.read(); // read a byte, then
                        Serial.write(c);        // print it out the serial monitor
                        header += c;
                        if (c == '\n')
                        { // if the byte is a newline character
                                // if the current line is blank, you got two newline characters in a row.
                                // that's the end of the client HTTP request, so send a response:
                                if (currentLine.length() == 0)
                                {
                                        if (header.indexOf("GET /") >= 0)
                                        {
                                                String URI = midString(header, "GET ", " ");
                                                if (loadFromSpiffs(URI, client))
                                                        break;
                                        }
                                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                                        // and a content-type so the client knows what's coming, then a blank line:
                                        client.println("HTTP/1.1 200 OK");
                                        client.println("Content-type:text/html");
                                        client.println("Connection: close");
                                        client.println();
                                        // turns the GPIOs on and off
                                        if (header.indexOf("POST /ON") >= 0)
                                        {
                                                state = 50;
                                        }
                                        if (header.indexOf("POST /OFF") >= 0)
                                        {
                                                state = 51;
                                        }
                                        if (header.indexOf("POST /AUTO") >= 0)
                                        {
                                                state = 99;
                                        }
                                        break;
                                }
                                else
                                { // if you got a newline, then clear currentLine
                                        currentLine = "";
                                }
                        }
                        else if (c != '\r')
                        {                         // if you got anything else but a carriage return character,
                                currentLine += c; // add it to the end of the currentLine
                        }
                }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
}

String midString(String str, String start, String finish)
{
        int locStart = str.indexOf(start);
        if (locStart == -1)
                return "";
        locStart += start.length();
        int locFinish = str.indexOf(finish, locStart);
        if (locFinish == -1)
                return "";
        return str.substring(locStart, locFinish);
}

bool loadFromSpiffs(String path, WiFiClient client)
{
        String dataType = "text/plain";
        if (path.endsWith("/"))
                path += "index.htm";

        if (!SPIFFS.exists(path))
                return false;
        if (path.endsWith(".src"))
                path = path.substring(0, path.lastIndexOf("."));
        else if (path.endsWith(".svg"))
                dataType = "image/svg+xml";
        else if (path.endsWith(".html"))
                dataType = "text/html";
        else if (path.endsWith(".htm"))
                dataType = "text/html";
        else if (path.endsWith(".css"))
                dataType = "text/css";
        else if (path.endsWith(".js"))
                dataType = "application/javascript";
        else if (path.endsWith(".png"))
                dataType = "image/png";
        else if (path.endsWith(".gif"))
                dataType = "image/gif";
        else if (path.endsWith(".jpg"))
                dataType = "image/jpeg";
        else if (path.endsWith(".ico"))
                dataType = "image/x-icon";
        else if (path.endsWith(".xml"))
                dataType = "text/xml";
        else if (path.endsWith(".pdf"))
                dataType = "application/pdf";
        else if (path.endsWith(".zip"))
                dataType = "application/zip";
        File dataFile = SPIFFS.open(path.c_str(), "r");

        /*if (server.hasArg("download")) dataType = "application/octet-stream";
           if (server.streamFile(dataFile, dataType) != dataFile.size()) {
           }
         */
        //client.print(F("POST "));
        //client.print(path);
        client.println("HTTP/1.1 200 OK");
        client.print(F("Content-Type: "));
        client.println(dataType);
        client.print(F("Host: "));
        client.println(WiFi.localIP());
        if (!(path.endsWith(".css") || path.endsWith("/")))
                client.println(F("Cache-Control: max-age=864000"));
        client.println(F("Connection: close"));
        client.print(F("Content-Length: "));
        client.println(dataFile.size());
        client.println();
        while (dataFile.available())
        {
                client.write(dataFile.read());
        }
        client.flush();
        dataFile.close();
        client.stop();

        return true;
}
