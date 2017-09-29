// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX

#include <SPI.h>
#include <RH_RF95.h>
#include <avr/dtostrf.h>

/* for feather m0 */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

#define Serial SERIAL_PORT_USBVIRTUAL

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

char prompt_str[32] = "> ";

int beaconTimeout = 60 * 10; // 30 secs
int beaconTimer = 0;
uint8_t my_id = 0;

void setup() {
    randomSeed(analogRead(0));
    my_id = random(0,255);
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, HIGH);

    pinMode(LED_BUILTIN, OUTPUT);

    //while (!Serial);
    Serial.begin(9600);
    delay(500);

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

    if (!rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096)) {
        Serial.println("setModemConfig failed");
        while (1);
    }

    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
    // you can set transmitter powers from 5 to 23 dBm:
    rf95.setTxPower(23, false);
    //rf95.setTxPower(14, false);

    snprintf(prompt_str, 32, "[%d] > ", my_id);

    delay(10);
    Serial.println("I'm ready!");
    Serial.print(prompt_str);
    Serial.flush();
}

char buffer[128] = {0};
size_t buf_cap = 128;
size_t buf_len = 0;

typedef struct {
    uint8_t ttl;
    uint8_t from;
} Header;

void send_packet(const char *data, uint8_t len) {
    if (!rf95.send((uint8_t*)data, len)) {
        Serial.println("Sending failed!");
    } else {
        rf95.waitPacketSent();
    }
}

void send_p(const Header p, const char *data, uint8_t len) {
    char buffer[256];
    memcpy(buffer, &p, sizeof(Header));
    memcpy(buffer+sizeof(Header), data, len);
    send_packet(buffer, len+sizeof(Header));
}

void print_hex(char *data, size_t len) {
    char buffer[256];
    size_t off = 0;
    for (size_t i=0; i < len; i++) {
        off += snprintf(buffer+off, 256-off, " %02x", data[i]);
        if (off >= 256-3) {
            break;
        }
    }
    Serial.println(buffer);
}

bool prompt = true;
void loop() {
    beaconTimer++;
    if (beaconTimer >= beaconTimeout) {
        char str[64];
        char flt[32];
        dtostrf(millis() / 1000.0f / 60, 14, 2, flt);
        snprintf(str, 64, "Uptime: %s min.", flt);
        send_p({1, my_id}, str, strlen(str));
        beaconTimer = 0;
    }

    size_t avail;
    while ((avail = Serial.available())) {
        int b = Serial.read();
        if (b != -1 && b != '\n' && b != '\r') {
            if (!prompt) {
                prompt = true;
                Serial.print(prompt_str);
                Serial.print(buffer+1);
            }
            buffer[buf_len++] = (char) b;
            buffer[buf_len] = 0;
            Serial.print((char)b);
            Serial.flush();
        }
        if (buf_len > 0 && (b == '\n' || b == '\r' || buf_len == buf_cap)) {
            Serial.println();
            Serial.print("...\r");Serial.flush();
            send_p(Header{1, my_id}, buffer, buf_len);
            Serial.print("   \r");Serial.flush();
            buf_len = 0;
            buffer[0] = 0;
            if (!prompt) {
                prompt = true;
                Serial.println();
            }
            Serial.print(prompt_str);
            Serial.flush();
        } else if (b == '\n' || b == '\r') {
            if (!prompt) {
                prompt = true;
            }
            Serial.println();
            Serial.print(prompt_str);
        }
    }

    if (rf95.waitAvailableTimeout(200)) {
        char receive_buf[RH_RF95_MAX_MESSAGE_LEN] = {0};
        uint8_t len = RH_RF95_MAX_MESSAGE_LEN-1;
        // Should be a reply message for us now
        if (rf95.recv((uint8_t*)receive_buf, &len) && len > 0) {
            Header *p = (Header*)receive_buf;
            char str[64];
            snprintf(str, 64, "< (rssi %d, from %d, ttl %d): ",
                rf95.lastRssi(), p->from, p->ttl);
            if (prompt) {
                prompt = false;
                Serial.println();
            }
            Serial.print(str);
            Serial.println(receive_buf+sizeof(Header));
            Serial.flush();
            if (len > 0 && p->ttl > 0) {
                p->ttl--;
                p->from = my_id;
                send_packet(receive_buf, len);
            }
        }
    } else {
    }
    delay(100);
}
