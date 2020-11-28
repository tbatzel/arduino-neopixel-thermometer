#include "arduino_secrets.h" 
#include <SPI.h>

#define debugPrint 1
#define PRINTFREQ 10000




//**************************************************/
// Define and Initialize RFM69 Radio 
#include <RH_RF69.h>
#include <RHReliableDatagram.h>
#define RF69_FREQ 915.0
#define DEST_ADDRESS   1
#define MY_ADDRESS     2

// Pins for Arduino MEGA 2560 using Adafruit Radio FeatherWing - RFM69HCW
#define RFM69_CS      5
#define RFM69_INT     3 // Mega pins that can do interrupts: 2, 3, 18, 19, 20, 21
#define RFM69_RST     6 
#define LED           13

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);
// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

int16_t packetnum = 0;  // packet counter



//**************************************************/
// Define and Initialize Adafruit NeoPixel Matrix
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>

// Define Neopixel Matrix
#define NeoMatrixPin 8
#define NeoMatrixHeight 8
#define NeoMatrixWidth 32        
#define NeoMatrixNumPixels 30    // Number of pixels to blink
#define ledBrightness 40

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








// Initial setup function
void setup() {
  Serial.begin(115200);

  //Random Seed
  randomSeed(analogRead(A1));


  //**************************************************/
  // Setup RFM69 Radio
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);
  
  // manual reset, because the demo said to.
  delay(10);
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
   
  if (!rf69_manager.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  } 

  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW
  
  //uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  uint8_t key[] = rfEncryptionKey;
  rf69.setEncryptionKey(key);
  
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");


  //**************************************************/
  // Setup Adafruit NeoPixel Matrix
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(ledBrightness);
  matrix.setTextColor(matrix.Color(32, 100, 128));  // 128, 128, 128
  matrix.fillScreen(colorBlack);
  matrix.show(); 


  nextPrint = millis() + PRINTFREQ;
}



// Dont put this on the stack, per the instructions.
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];




//**************************************************/
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
  if (millis() > nextPrint)
  {  

    // Send a RF request for the temperature
    char radiopacket[20] = "Temp?";
    itoa(packetnum++, radiopacket+13, 10);
  
    if (rf69_manager.sendtoWait((uint8_t *)radiopacket, strlen(radiopacket), DEST_ADDRESS)) {
      // Now wait for a reply from the server
      uint8_t len = sizeof(buf);
      uint8_t from;   
      if (rf69_manager.recvfromAckTimeout(buf, &len, 2000, &from)) {
        buf[len] = 0; // zero out remaining string
          Serial.print("Got reply from #"); Serial.print(from);
          Serial.print(" [RSSI :");
          Serial.print(rf69.lastRssi());
          Serial.print("] : ");
          Serial.println((char*)buf);   
          Serial.println("");  

          // Convert uint8_t buffer to formatted char array
          String tempFCurrentStr = String((char*)buf);          // buffer -> char array -> string
          float tempFCurrent = tempFCurrentStr.toFloat();       // string -> float
          dtostrf(tempFCurrent, 3, 1, chr_tempFCurrent_small);  // float to fixed length char array
                
      } else {
        Serial.println("No reply, from other RF device.");
      }
    } else {
      Serial.println("Sending failed (no ack)");
    }
    nextPrint = millis() + PRINTFREQ;
  } // end  if (millis() > nextPrint)


  matrix.setTextColor(matrix.Color(32, 100, 128));  // nightime = 64,0,0

  // Print the temp to the matrix
  printCenter(chr_tempFCurrent_small);


  // nightime + display off = matrix.fillScreen(colorBlack);

  matrix.show();

  delay(20);  
} // end main void loop











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
