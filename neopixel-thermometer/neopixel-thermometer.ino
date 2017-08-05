
#define debugPrint 1

#include <SoftwareSerial.h>
SoftwareSerial serialInc(2, 8); // RX, TX   // Uses about 950 sketch, and 115 memory


// Adafruit NeoPixel Matrix
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>


// Adafruit Temperature Sensors I2C Data
#include <Wire.h>
#include "Adafruit_MCP9808.h"

// Hardware definitions
#define lightSensorPin A0


//Blinky Code
#define DRAWWAIT 10

#define MINBLINKY 0
#define MAXBLINKY 128 //255
#define PRINTFREQ 3500

#define NeoMatrixPin 6
#define NeoMatrixHeight 8
#define NeoMatrixWidth 32         //32
#define NeoMatrixNumPixels 30    // 60 Number of pixels to blink



// Adafruit_NeoMatrix DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)


// Initialize Hardware
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(NeoMatrixWidth, NeoMatrixHeight, NeoMatrixPin, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG, NEO_GRB + NEO_KHZ800);
Adafruit_MCP9808 tempsensor0 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor1 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor2 = Adafruit_MCP9808();


// User definded settings
#define ledDimMin  4              // Min LED brightness for dim content, out of 0-255
#define ledDimMax   40            // Max LED brightness for dim content, out of 0-255
#define lightSensorLuxMin 4       // Min clamp for measured room light value in lux
#define lightSensorLuxMax 50      // Max clamp for measured room light value in lux
#define logUpdateFreq 45000      // Frequency to record Min/Max temps.  1,000ms = 1 second



// Global variables
float tempFCurrent;
unsigned long timeLast;
uint8_t ledDim = ledDimMax;
const float lightSensorRawRange = 1024; // 3.3v
const float lightSensorLogRange = 5.0; // 3.3v = 10^5 lux
const uint16_t colorBlack = matrix.Color(0, 0, 0);
char chr_tempFCurrent_small[5];

unsigned long nextPrint = 0;

unsigned int serialRxSuccess = 0;
unsigned int serialRxFail = 0;

float val1_Min;
float val1_Max;
float val2_Min;
float val2_Max;

struct pixel
{
  int x;
  int y;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

pixel pixels[NeoMatrixNumPixels]; //Uses 420 bytes, at 60 pixels.  Each pixel is 7 bytes.







// Initial setup function
void setup() {
  Serial.begin(38400);
  serialInc.begin(38400);

  if (debugPrint) Serial.println(F("Initializing..."));


  if (debugPrint) Serial.println(F("Wire.begin()"));
  Wire.begin();

  //Random Seed
  randomSeed(analogRead(A1));

  //LED Matrix initialization
  if (debugPrint) Serial.println(F("Initializing LED Matrix"));
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(ledDim);
  //matrix.setTextColor(matrix.Color(128, 128, 128));
  matrix.setTextColor(matrix.Color(32, 100, 128));
  matrix.fillScreen(colorBlack);

  // Initialize the temp sensors
  if (debugPrint) Serial.println(F("Initializing Temp Sensors"));
  if (!tempsensor0.begin(0x18)) {
    if (debugPrint) Serial.println(F("Couldn't find MCP9808 at 0x18"));
    delay(1000);
  }
  if (!tempsensor1.begin(0x19)) {
    if (debugPrint) Serial.println(F("Couldn't find MCP9808 at 0x19"));
    delay(1000);
  }
  if (!tempsensor2.begin(0x1A)) {
    if (debugPrint) Serial.println(F("Couldn't find MCP9808 at 0x1A"));
    delay(1000);
  }

  nextPrint = millis() + PRINTFREQ;
}









void loop() {

  matrix.fillScreen(colorBlack);

  if (millis() % 8 == 0) {
    // Read the ambiant light level, and set the ledBright and ledDim levels.  // ***** needs to be smoother
    ReadLightLevel();
  }

  // Read the various temp sensors, set global tempFCurrent.
  ReadTempSensors();

  // Check on timer, for update log / Min/Max values
  unsigned long timeSince = TimeSinceLogUpdate();

  // Update MinMax temperature values, based on timer or being out of range
  if (timeSince >= logUpdateFreq) {
    char chr_tempFCurrent[7];
    dtostrf(tempFCurrent, 3, 2, chr_tempFCurrent);
    char chr_tempFCurrentSend[32]; //9
    sprintf(chr_tempFCurrentSend, "\n<log>%s</log>\n", chr_tempFCurrent);  // start with \n later on

    if (debugPrint) Serial.print(F("serialInc.print   "));
    if (debugPrint) Serial.println(chr_tempFCurrentSend);
    serialInc.print(chr_tempFCurrentSend);     // Send temp to logging board
    timeLast = millis();    // Update the timeLast value to now
  }


  // If the serial rx buffer has data, clear it out, then pull data again.
  if (Serial.available()) {
    delay(10); // helps ensure all of !!! arrives as one chunk. (10ms -> @38400 = 48 btyes, @9600, 12 bytes)
    // Discard the existing data in the buffer
    /*
    while (Serial.available()) {
      char c = Serial.read();
    }
    */
    checkSerialHost();
  }






  // Dim the pixels in the array
  for (uint8_t i = 0; i < NeoMatrixNumPixels; i++)
  {
    if (pixels[i].r > 0) pixels[i].r--;
    if (pixels[i].g > 0) pixels[i].g--;
    if (pixels[i].b > 0) pixels[i].b--;
    if (pixels[i].r == 0 && pixels[i].g == 0 && pixels[i].b == 0)
    {
      initPixel(i);
    }
    matrix.drawPixel(pixels[i].x, pixels[i].y, matrix.Color(pixels[i].r, pixels[i].g, pixels[i].b));
  }


  // Serial print, and other things that don't need to happen every cycle
  if (millis() > nextPrint)
  {
    if (debugPrint) printTimestamp();
    if (debugPrint) Serial.print(F("   "));
    if (debugPrint) Serial.print(F("Temperature: "));
    if (debugPrint) Serial.print(tempFCurrent);
    if (debugPrint) Serial.println(F("*F"));

    // Update the temp shown on the reader
    dtostrf(tempFCurrent, 3, 1, chr_tempFCurrent_small);

    nextPrint = millis() + PRINTFREQ;
  }



  // Print the temp to the matrix
  printCenter(chr_tempFCurrent_small);


  // Code for showing the success count on the LED board, if using hardware serial.
  /*
    if((millis() % 5000 < 1000) && (serialRxFail || serialRxSuccess) )
    {

    char serialReport[10];  //1234-678
    sprintf(serialReport, "%d-%d", serialRxSuccess, serialRxFail);
    printCenter(serialReport);
    }
    else {
    printCenter(chr_tempFCurrent_small);
    }
  */



  matrix.show();

  delay(20);
}

// ---------- End of void loop() -------------------






// ---------- Start of functions -------------------




void ReadLightLevel() {
  int ligtSensorLuxValue = (RawToLux(analogRead(lightSensorPin)));
  /*
    if (debugPrint) {
    Serial.print(F("Lux = "));
    Serial.print(ligtSensorLuxValue);
    }
  */
  ligtSensorLuxValue = constrain(ligtSensorLuxValue, lightSensorLuxMin, lightSensorLuxMax);
  /*
    if (debugPrint) {
    Serial.print(F(" Lux (constrain) = "));
    Serial.println(ligtSensorLuxValue);
    }
  */
  uint8_t ledDim_goal = map(ligtSensorLuxValue, lightSensorLuxMin, lightSensorLuxMax, ledDimMin, ledDimMax);
  if (ledDim_goal > ledDim)        ledDim++;
  else if (ledDim_goal < ledDim)   ledDim--;
  matrix.setBrightness(ledDim);

}




float RawToLux(int raw) {
  float lightSensorLogLux = raw * lightSensorLogRange / lightSensorRawRange;
  return pow(10, lightSensorLogLux);
}




void ReadTempSensors() {
  float temp0F = tempsensor0.readTempC() * 9.0 / 5.0 + 32;
  float temp1F = tempsensor1.readTempC() * 9.0 / 5.0 + 32;
  float temp2F = tempsensor2.readTempC() * 9.0 / 5.0 + 32;

  float tempFAvg = (temp1F + temp2F + (2 * temp0F)) / 4;

  // If the external sensor (temp0F) is "missing" don't use it.
  if (temp0F < 33)    tempFAvg = (temp1F + temp2F) / 2;

  /*
    if (debugPrint) {
    Serial.print(F("Temp 0: ")); Serial.print(temp0F); Serial.print(F("*F   "));
    Serial.print(F("Temp 1: ")); Serial.print(temp1F); Serial.print(F("*F   "));
    Serial.print(F("Temp 2: ")); Serial.print(temp2F); Serial.print(F("*F"));
    Serial.println();
    Serial.print(F("Temperature: ")); Serial.print(tempFAvg); Serial.print(F("*F"));
    Serial.println(F(""));
    }
  */
  tempFCurrent = tempFAvg; // Populate GlobalVar
}





unsigned long TimeSinceLogUpdate() {
  if (!timeLast) {
    timeLast = millis(); // Initialize timeLast
  }
  if (timeLast > millis()) {
    timeLast = millis(); // Rollover, occurs every 50 days, it's "good enough", without accounting for that for now // **********
  }
  unsigned long timeSince = millis() - timeLast;

  return timeSince;
}


















void checkSerialHost() {
  unsigned int serialRxSuccessTemp = serialRxSuccessTemp;




  if (Serial.available()) {
    Serial.println(F("")); printTimestamp(); Serial.println(F("     Serial.available()"));
  }
  else {
    Serial.println(F("")); printTimestamp(); Serial.println(F("     NOT: Serial.available()"));
  }




  Serial.println(F("")); printTimestamp(); Serial.println(F("     Clear out buffer"));
  // clear out existing buffer
    while (Serial.available()) {
      char c = Serial.read();
    }




  if (Serial.available()) {
    Serial.println(F("")); printTimestamp(); Serial.println(F("     Serial.available()"));
  }
  else {
    Serial.println(F("")); printTimestamp(); Serial.println(F("     NOT: Serial.available()"));
  }

    
  
  Serial.println(F("")); printTimestamp(); Serial.println(F("     serialInc.print() - send get allow command"));
  serialInc.print("\n<get>values</get>\n");



/*
  Serial.println(F("")); printTimestamp(); Serial.println(F("     delay()"));
  // 30 characters @ 38400 = 7ms; @9600 = 25ms
  delay(10);
*/

  

  if (Serial.available()) {
    Serial.println(F("")); printTimestamp(); Serial.println(F("     Serial.available()"));
  }
  else {
    Serial.println(F("")); printTimestamp(); Serial.println(F("     NOT: Serial.available()"));
  }



  
  unsigned long timebreak = millis() + 5000; 
  while(!Serial.available())  {
        Serial.println(F("")); printTimestamp(); Serial.println(F("     No serial, delay"));
        if (timebreak <= millis()) {
          Serial.println(F("")); printTimestamp(); Serial.println(F("     Break; from timeout"));
          break;
        }
        delay(10);
  }



              
  if (Serial.available()) {
    Serial.println(F("")); printTimestamp(); Serial.println(F("     Serial Availiable"));

    String serialRxData;
    serialRxData.reserve(32);

    while (Serial.available()) {
      char c = Serial.read();
      serialRxData += c;
      if (c == '\n') {
        Serial.println(F("")); printTimestamp(); Serial.println(F("     Parse Data"));
        parseSerialRxData(serialRxData);
        serialRxData = "";
      }
      else {
        // If the last character isn't \n, and there is no new data, delay before exiting the while
        timebreak = millis() + 1500;
        while(!Serial.available())  {
              Serial.println(F("")); printTimestamp(); Serial.println(F("     No serial, Not LF, delay"));
              if (timebreak <= millis()) {
                Serial.println(F("")); printTimestamp(); Serial.println(F("     Break; from timeout"));
                break;
              }
              delay(10);
        }        
      }
    }
  }
  

  if (serialRxSuccess <= serialRxSuccessTemp) serialRxFail ++;
  Serial.println(F(""));
  Serial.print(F("     Success: "));
  Serial.print(serialRxSuccess);
  Serial.print(F(", Fail: "));
  Serial.println(serialRxFail);
  
}




void parseSerialRxData(String data) {
  // Don't bother parsing if it's just 1 character
  if (data.length() <= 1) {
    return;
  }
  
  if (debugPrint) printTimestamp();
  if (debugPrint) Serial.println(F(""));
  if (debugPrint) Serial.print(F("   data ("));
  if (debugPrint) Serial.print(data.length());  
  if (debugPrint) Serial.print(F(") = "));
  if (debugPrint) Serial.println(data);



  if (data.length() == 24) {    //23 + \n = 24    12.34,56.78,90.12,34.56\n
    serialRxSuccess ++;
    if (   (data.substring(2, 3) == ".")   && (data.substring(5, 6) == ",")
           && (data.substring(8, 9) == ".")   && (data.substring(11, 12) == ",")
           && (data.substring(14, 15) == ".") && (data.substring(17, 18) == ",")
           && (data.substring(20, 21) == ".") ) {
      if (debugPrint) Serial.println(F("   ***   ***   Found Everything!!!!!"));
      val1_Min = data.substring(0, 5).toFloat();
      val1_Max = data.substring(6, 11).toFloat();
      val2_Min = data.substring(12, 17).toFloat();
      val2_Max = data.substring(18, 23).toFloat();
    }
  }

}






void printTimestamp() {
  unsigned long timeSeconds = millis() / 1000;
  unsigned long hours = timeSeconds / 3600;
  unsigned long minutes = (timeSeconds % 3600 ) / 60;
  unsigned long seconds = timeSeconds % 60;

  if (hours <= 9) Serial.print(F("0"));
  Serial.print(hours); Serial.print(F(":"));

  if (minutes <= 9) Serial.print(F("0"));
  Serial.print(minutes); Serial.print(F(":"));

  if (seconds <= 9) Serial.print(F("0"));
  Serial.print(seconds);

  Serial.print(F("   ("));
  Serial.print(timeSeconds);
  Serial.print(F(")"));

  Serial.print(F("   ("));
  Serial.print(millis());
  Serial.print(F(")"));

}






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



/*
  void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    serialRxData += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      serialRxDataComplete = true;
    }
  }
  }
*/













