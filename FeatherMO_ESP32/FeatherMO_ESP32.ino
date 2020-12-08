#include <SPI.h>
#include <avr/dtostrf.h>

#define PRINTFREQ 90000





//**************************************************/
// Define and Initialize Adafruit NeoPixel Matrix
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>

// Define Neopixel Matrix
#define NeoMatrixPin 5 
#define NeoMatrixHeight 8
#define NeoMatrixWidth 32        
#define NeoMatrixNumPixels 30    // Number of pixels to blink
#define ledBrightness 32

// Initialize Hardware
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(NeoMatrixWidth, NeoMatrixHeight, NeoMatrixPin, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG, NEO_GRB + NEO_KHZ800);

// Define a pixel
struct pixel
{
  int x;
  int y;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

// Define the matrix of pixels
pixel pixels[NeoMatrixNumPixels]; // Each pixel is 7 bytes.

// Globals
const uint16_t colorBlack = matrix.Color(0, 0, 0);
unsigned long nextPrint = 0;
char chr_tempFCurrent_small[5];



//**************************************************/
// Define and Initialize ESP32 Wifi
#include <WiFiNINA.h>
#include <ArduinoJson.h>

// Configure the pins used for the ESP32 connection, ADAFRUIT_FEATHER_M0_EXPRESS
#define SPIWIFI       SPI  // The SPI port
#define SPIWIFI_SS    13   // Chip select pin
#define ESP32_RESETN  12   // Reset pin
#define SPIWIFI_ACK   11   // a.k.a BUSY or READY pin
#define ESP32_GPIO0   -1

#include "arduino_secrets.h" 

char ssid[] = SECRET_SSID;        
char pass[] = SECRET_PASS;  
int keyIndex = 0;          

int status = WL_IDLE_STATUS;

WiFiSSLClient client;










void setup() {
  Serial.begin(115200);

  //Random Seed
  randomSeed(analogRead(A1));




  //**************************************************/
  // Setup Adafruit NeoPixel Matrix
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(ledBrightness);
  matrix.setTextColor(matrix.Color(32, 100, 128));  // 128, 128, 128
  matrix.fillScreen(colorBlack);
  matrix.show(); 


  // might as well have something on the screen while the wifi boots
  matrixPrideFlag(4);
  matrixTransPrideFlag(18);
  matrix.show(); 


  nextPrint = millis() + PRINTFREQ;




  //**************************************************/
  // Setup ESP32 Wifi

  // check for the WiFi module:
  WiFi.setPins(SPIWIFI_SS, SPIWIFI_ACK, ESP32_RESETN, ESP32_GPIO0, &SPIWIFI);
  while (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    delay(1000);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  Serial.println("*This is a slow process");

  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  do {
    status = WiFi.begin(ssid, pass);
    delay(100); // wait until connected
  } while (status != WL_CONNECTED);
  Serial.println("Connected to wifi");

  printWifiStatus();


 

  Serial.println("Debug: Setup Done");
}

//uint32_t bytes = 0;








void loop() {
  matrix.fillScreen(colorBlack);

    
  // Dim the pixels in the array,
  for (uint8_t i = 0; i < NeoMatrixNumPixels; i++)
  {
    if (pixels[i].r > 0) pixels[i].r--;
    if (pixels[i].g > 0) pixels[i].g--;
    if (pixels[i].b > 0) pixels[i].b--;

    // when a pixel dims out, recreate it somewhere else.
    if (pixels[i].r == 0 && pixels[i].g == 0 && pixels[i].b == 0) {
      initPixel(i);
    }
    // nightime = don't draw new pixels
    matrix.drawPixel(pixels[i].x, pixels[i].y, matrix.Color(pixels[i].r, pixels[i].g, pixels[i].b));     
  }




  // Things that don't need to happen every cycle
  if (millis() > nextPrint) {
    Serial.println("Debug: nextPrint");
    getTemp();

  nextPrint = millis() + PRINTFREQ;    
  }// end  if (millis() > nextPrint)




  matrix.setTextColor(matrix.Color(32, 100, 128));  // nightime = 64,0,0
  printCenter(chr_tempFCurrent_small);  
  matrix.show();
  delay(20);   
}







//**************************************************/

void printCenter(String centerMessage) {
  int loc = matrix.width() / 2;
  loc = loc - ((centerMessage.length() * 6) / 2);
  matrix.setCursor(loc, 0);
  matrix.print(centerMessage);
}


//Randomizes a pixel's location and colors.  Prevents generating white colors.
void initPixel(uint8_t num) {
  pixels[num].x = random(matrix.width());
  pixels[num].y = random(matrix.height());

  uint8_t red = random(32, 128);
  uint8_t blue = random(0, 96);
  uint8_t green = ((128 - red) / 2) + ((96 - blue) / 2);

  pixels[num].r = red;
  pixels[num].g = green;
  pixels[num].b = blue;
}




void getTemp() {

  Serial.println("\nStarting connection to server...");

  // if you get a connection, report back via serial:
  if (client.connect(SERVER, 443)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.println("GET " PATH " HTTP/1.1");
    client.println("Host: " SERVER);
    client.println("Connection: close");
    client.println();
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
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
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonDocument doc(capacity);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  Serial.print("F: "); Serial.println(doc["F"].as<float>(), 1);
  
  Serial.print("F: "); 
  dtostrf( doc["F"].as<float>(), 3, 1, chr_tempFCurrent_small);

  Serial.println(chr_tempFCurrent_small);

  // Disconnect
  client.stop();

  
}






void matrixPrideFlag(uint8_t x) {
  for (uint8_t i = 0; i < 10 ; i++) {

    matrix.drawPixel(x+i, 1, matrix.Color(0xE4, 0x03, 0x03));  // Electric Red
    matrix.drawPixel(x+i, 2, matrix.Color(0xFF, 0x8C, 0x00));  // Dark Orange
    matrix.drawPixel(x+i, 3, matrix.Color(0xFF, 0xED, 0x00));  // Canary Yellow
    matrix.drawPixel(x+i, 4, matrix.Color(0x00, 0x80, 0x26));  // La Salle Green
    matrix.drawPixel(x+i, 5, matrix.Color(0x00, 0x4D, 0xFF));  // Blue (RYB)
    matrix.drawPixel(x+i, 6, matrix.Color(0x75, 0x07, 0x87));  // Patriarch
   
  }
}

void matrixTransPrideFlag(uint8_t x) {
  for (uint8_t i = 0; i < 10 ; i++) {
    matrix.drawPixel(x+i, 1, matrix.Color(0x55, 0xCD, 0xFC));  // Maya Blue
    matrix.drawPixel(x+i, 2, matrix.Color(0x55, 0xCD, 0xFC));  // Maya Blue
    matrix.drawPixel(x+i, 3, matrix.Color(0xC3, 0xC3, 0xC3));  // White  (FF is default, but too bright, C3 is the average of the other two colors)
    matrix.drawPixel(x+i, 4, matrix.Color(0xC3, 0xC3, 0xC3));  // White
    matrix.drawPixel(x+i, 5, matrix.Color(0xF7, 0xA8, 0xB8));  // Amaranth Pink
    matrix.drawPixel(x+i, 6, matrix.Color(0xF7, 0xA8, 0xB8));  // Amaranth Pink
   
  }  

}






void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
