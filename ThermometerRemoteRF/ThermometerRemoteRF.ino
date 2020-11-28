#include "arduino_secrets.h" 
#include <SPI.h>



//**************************************************/
// Define and Initialize RFM69 Radio 
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

#define RF69_FREQ 915.0
#define MY_ADDRESS     1

// Pins for Feather M0 w/ RF built in
#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4
#define LED           13

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);
// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

int16_t packetnum = 0;  // packet counter, we increment per xmission



//**************************************************/
// Define and Initialize MCP9808 Temp Sensor 
#include <Wire.h>
#include "Adafruit_MCP9808.h"
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();








void setup() 
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);     

  
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
  // Setup MCP9808 Temp Sensor 

  if (!tempsensor.begin(0x18)) {
    Serial.println("Couldn't find MCP9808! Check your connections and verify the address is correct.");
    while (1);
  }
  
  tempsensor.setResolution(1);
  // Mode Resolution SampleTime
  //  0    0.5°C       30 ms
  //  1    0.25°C      65 ms
  //  2    0.125°C     130 ms
  //  3    0.0625°C    250 ms

  
}


// Dont put this on the stack, per the instructions.
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];




//**************************************************/
void loop() {
  
  // Wait for a message addressed to us from a client
  if (rf69_manager.available())
  {   
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (rf69_manager.recvfromAck(buf, &len, &from)) {
      buf[len] = 0; // zero out remaining string
      
      Serial.print("Got packet from #"); Serial.print(from);
      Serial.print(" [RSSI :");
      Serial.print(rf69.lastRssi());
      Serial.print("] : ");
      Serial.println((char*)buf);
      Blink(LED, 40, 3); //blink LED 3 times, 40ms between blinks


      // Get the current temperature
      float f = getTemperature(); 
      char data[8];
      dtostrf(f, 6, 2, data);
      
      // Send a reply back to the originator client
      if (!rf69_manager.sendtoWait((uint8_t*)data, sizeof(data), from))
        Serial.println("Sending failed (no ack)");
    }
  }
}





float getTemperature() {
  tempsensor.wake();
  float f = tempsensor.readTempF();
  Serial.print("Temp: ");  Serial.print(f, 2); Serial.println("*F.");
  tempsensor.shutdown_wake(1);
  return f;
}



void Blink(byte PIN, byte DELAY_MS, byte loops) {
  for (byte i=0; i<loops; i++)  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);
  }
}
