// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX

#include <SPI.h>
#include <RH_RF95.h>

/* for feather m0 */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

/* Feather m0 w/wing 
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     6    // "D"
*/

// Change to 434.0 or other frequency, must match RX's freq!
// USA: 915
//#define RF95_FREQ 915.0
// EU: 868
#define RF95_FREQ 868.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);

  while (!Serial);
  Serial.begin(9600);
  delay(100);

  Serial.println("Feather LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
}
Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
}
Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  //rf95.setTxPower(23, false);
rf95.setTxPower(14, false);

Serial.print("yes? > ");Serial.flush();
}

int16_t packetnum = 0;  // packet counter, we increment per xmission
char led = 0;

char buffer[128];
size_t buf_cap = 127;
size_t buf_len = 0;

void loop()
{
    size_t avail = Serial.available();
    if (avail) {
        int b = Serial.read();
        if (b != -1 && b != '\n') {
            buffer[buf_len++] = (char) b;
            buffer[buf_len] = 0;
            Serial.print((char)b);
            Serial.flush();
            delay(10);
        }
        if (b == '\n' || buf_len == buf_cap) {
            Serial.println();
            if (!rf95.send((uint8_t*)buffer, buf_len)) {
                Serial.println("Sending failed!");
            } else {
                rf95.waitPacketSent();
            }
            buf_len = 0;
            Serial.print("yes? > ");Serial.flush();
        }
    }

    if (rf95.waitAvailableTimeout(100)) {
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        // Should be a reply message for us now
        if (rf95.recv(buf, &len)) {
            char buffer[64];
            snprintf(buffer, 64, "< (rssi %d) : ", rf95.lastRssi());
            Serial.print(buffer);
            Serial.println((char*)buf);
        } else {
            Serial.println("TIMEOUT?");
        }
    } else {
    }
    delay(10);
}
