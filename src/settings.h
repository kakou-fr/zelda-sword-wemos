// Replace with your network credentials
const char* ssid     = "SSID";
const char* password = "KEY";

#define my_OTA_PW "SSIDOTA"     // password for OTA updates

// define variables

#define DATA_PIN_LEDS D6

#define Aleds 15
#define Bleds 84
#define Cleds 93
#define NUM_LEDS 93

#define SwordUp 48

//Global variable
String ipStr = "";
String respMsg = "";
String formattedTime = "";
bool noInit = true;
