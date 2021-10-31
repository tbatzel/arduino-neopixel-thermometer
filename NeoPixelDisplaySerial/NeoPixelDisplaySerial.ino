#define SERIAL_BUFFER_SIZE 256

#include <avr/dtostrf.h>
#include <stdlib.h>


// this is a debug comment - Oct 31, 2021 13:40



#define debugPrint 1
#define PRINTFREQ 10000

int flagPin = 19;
String payload = "";
int serialRxSemaphore = 0;
int serialRxSemaphoreFinal = 0;




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
#define ledBrightness 48

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
char chr_message[5];





int failCount = 0;


// Initial setup function
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  
  //Random Seed
  randomSeed(analogRead(A1));

  pinMode(flagPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);





  //**************************************************/
  // Setup Adafruit NeoPixel Matrix
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(ledBrightness);
  matrix.setTextColor(matrix.Color(128, 128, 128));  // 128, 128, 128
  matrix.fillScreen(colorBlack);
  matrix.show(); 


  nextPrint = millis() + PRINTFREQ;

  // might as well have something on the screen while the wifi boots
  matrixPrideFlag(4);
  matrixTransPrideFlag(18);
  matrix.show(); 
  delay(1000);
  
}







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
    nextPrint = millis() + PRINTFREQ;
    
    interrupts();
    delay(10);
    Serial.println("Getting Temp");
    Serial1.write("GET_TEMP\n");
    delay(10);
    payload = Serial1.readStringUntil('\n');   
    Serial.println(payload);
    
    if (payload.charAt(6) == ':') {
      matrix.setTextColor(   colorFromHex(payload.substring(0,6))   );
      payload.substring(7).toCharArray(chr_message, 5);
      failCount = 0;
    }
    else {
      failCount ++;
      if (failCount > 3) {
        matrix.setTextColor(colorBlack);
      }
    }
    
       
    delay(50);
    noInterrupts();
    
    
  
  }

  printCenter(chr_message);  
  matrix.show();

  delay(15);  
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
    matrix.drawPixel(x+i, 2, matrix.Color(0xF7, 0xA8, 0xB8));  // Amaranth Pink
    matrix.drawPixel(x+i, 3, matrix.Color(0xC3, 0xC3, 0xC3));  // White  (FF is default, but too bright, C3 is the average of the other two colors)
    matrix.drawPixel(x+i, 4, matrix.Color(0xC3, 0xC3, 0xC3));  // White
    matrix.drawPixel(x+i, 5, matrix.Color(0xF7, 0xA8, 0xB8));  // Amaranth Pink
    matrix.drawPixel(x+i, 6, matrix.Color(0x55, 0xCD, 0xFC));  // Maya Blue

   
  }  

}



boolean isNumeric(String str) {
    unsigned int stringLength = str.length();
 
    if (stringLength == 0) {
        return false;
    }
 
    boolean seenDecimal = false;
 
    for(unsigned int i = 0; i < stringLength; ++i) {
        if (isDigit(str.charAt(i))) {
            continue;
        }
 
        if (str.charAt(i) == '.') {
            if (seenDecimal) {
                return false;
            }
            seenDecimal = true;
            continue;
        }
        return false;
    }
    return true;
}




uint8_t intFromHex(String sHex) {
  const char* cHex = sHex.c_str();
  return (uint8_t) strtol(cHex, 0, 16);
}
  
uint16_t colorFromHex(String sHex) {
  uint8_t r,g,b;
  r = intFromHex(sHex.substring(0,2));
  g = intFromHex(sHex.substring(2,4));
  b = intFromHex(sHex.substring(4,6));
  return matrix.Color(r, g, b);
}
