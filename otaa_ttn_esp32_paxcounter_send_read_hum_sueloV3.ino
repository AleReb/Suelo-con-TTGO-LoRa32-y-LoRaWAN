 /*
   pinout especial del paxcounter tiene monitoreo de bateria y usa otros pines para conectarse

  TTGO LoRa32 V2.1.6:
  ESP32          LoRa (SPI)  SDCARD(SPI)    Display (I2C)  LED           BAT
  -----------    ----------  -----------    -------------  ------------------
  GPIO5  SCK     SCK
  GPIO27 MOSI    MOSI
  GPIO19 MISO    MISO
  GPIO18 SS      NSS
  GPIO23         RST
  GPIO26         DIO0
  GPIO33         DIO1
  GPIO32         DIO2
  GPIO21 SDA                     SDA
  GPIO22 SCL                     SCL
  GPIO25                                                 LED COMO SE USA UN NEOPIXEL NO SE USA EL LED DEL ESP
  GPIO35                                                 LECTURA 100K BAT
  GPIO34                                                 LECTURA panel solar 12k / 3.3k
  GPIO14                 SD_CLK
  GPIO2                  SD_MISO
  GPIO15                 SD_MOSI
  GPIO13                 SD_CS
  GPIO12                 DAT2 // no se usa
  GPIO4                  DAT1 // no se usa con la libreria mySD



*/
//  SPI port #2:  SD Card Adapter
#define  SD_CLK     14
#define  SD_MISO    02
#define  SD_MOSI    15
#define  SD_CS      13
#define  LoRa_CS    18
#define  Select    LOW   //  Low CS means that SPI device Selected
#define  DeSelect  HIGH  //  High CS means that SPI device Deselected

#include <mySD.h>
#include <SPI.h>
#include <lmic.h>
#include <hal/hal.h>


#include <U8x8lib.h>

#define DISABLE_PING 0
#define DISABLE_BEACONS 0

#include <Wire.h>

#include <HardwareSerial.h>
HardwareSerial mySerial2(1);

const int numReadings  = 20;
int indexD             = 0; // El indice de la lectura actual
float readings[numReadings]; // Lecturas de la entrada analogica
float total           = 0.0; // Total
float average         = 0.0; // Promedio
float voltageLevel;
float volsolar; //================================================= solar
float volsolar12V;//=============================================== solar
String inData;
float hum;
String mensaje ;
boolean sd = true;
boolean err = false;

#include <Adafruit_NeoPixel.h>
#define PIN        25 // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 1 // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int Vlev = 35;
int Slev = 34;
RTC_DATA_ATTR char filenameCSV[25] = ""; //para construir el archivo csv base cada vez que se prenda y guarde
                                       
//estos estan invertidos 
//static const u1_t PROGMEM APPEUI[8] = {0xF6, 0x96, 0x15, 0x16, 0x70, 0xD7, 0x49, 0x7B}; // iq5
//static const u1_t PROGMEM APPEUI[8] = {0x5C, 0x8D, 0x08, 0x67, 0x49, 0x15, 0x96, 0x62}; // iq4
//static const u1_t PROGMEM APPEUI[8] = {0x2C, 0x95, 0x81, 0xAB, 0x49, 0x5E, 0x47, 0x46}; // iq2 cercano
//static const u1_t PROGMEM APPEUI[8] = {0x43, 0x80, 0x0B, 0x7E, 0xA9, 0xD5, 0x3D, 0x5E}; // iq1 estaca87
static const u1_t PROGMEM APPEUI[8] = {0x3C, 0x5A, 0x0B, 0xA5, 0xD7, 0x53, 0xDD, 0x1A}; // iq6  quemado
void os_getArtEui(u1_t * buf) {
  memcpy_P(buf, APPEUI, 8);
}
//0xD0, 0xC2, 0x4E, 0xC9, 0x2C, 0x18, 0xB9, 0x92

//static const u1_t PROGMEM DEVEUI[8] = {0x92, 0xB9, 0x18, 0x2C, 0xC9, 0x4E, 0xC2, 0xD0}; // iq5 SECRET_APP_KEY;  CERCANO

//static const u1_t PROGMEM DEVEUI[8] = {0xA5, 0xF0, 0x09, 0x9A, 0x21, 0xDE, 0x3C, 0x56}; // iq4 SECRET_APP_KEY; SOLDADURA

//static const u1_t PROGMEM DEVEUI[8] = {0x19, 0x7B, 0x05, 0xD0, 0x7E, 0xD5, 0xB3, 0x70}; // iq2 SECRET_APP_KEY; 
//static const u1_t PROGMEM DEVEUI[8] = {0x0A, 0xEA, 0xFE, 0x47, 0x1F, 0xF9, 0x53, 0x3A}; // iq1
static const u1_t PROGMEM DEVEUI[8] = {0x44, 0xF7, 0x2A, 0xE1, 0x53, 0xA7, 0x09, 0x22}; // iq6


void os_getDevEui(u1_t * buf) {
  memcpy_P(buf, DEVEUI, 8);
}
//86 9C A6 1D 6 4 E 61 E1 01 35 77 CB F5 F4 D8 40
//static const u1_t PROGMEM APPKEY[16] = {0xE3, 0x43, 0xB7, 0x90, 0x32, 0xF1, 0x94, 0xF9, 0x55, 0x13, 0x00, 0x98, 0x59, 0x05, 0x1D, 0x21}; // iq5 este suele funcionar a la primera
//86 9C A6 1D 6 4 E 61 E1 01 35 77 CB F5 F4 D8 40
//static const u1_t PROGMEM APPKEY[16] = {0xEC, 0x5A, 0x84, 0x6B, 0x54, 0xBE, 0x24, 0xE9, 0x6F, 0x15, 0xA5, 0xE2, 0x38, 0x6B, 0x55, 0x0A}; // iq4 este suele funcionar a la primera
//static const u1_t PROGMEM APPKEY[16] = {0x3E, 0xEE, 0xF7, 0x53, 0x76, 0xD4, 0x26, 0x15, 0xB2, 0x40, 0x69, 0xFB, 0x58, 0x00, 0xED, 0x81}; // iq2 este suele funcionar a la primera
//static const u1_t PROGMEM APPKEY[16] = {0x6E, 0x84, 0x73, 0x88, 0xF2, 0xED, 0xA6, 0xE8, 0x7A, 0x56, 0x8E, 0x6C, 0xF7, 0xD2, 0x11, 0x65}; // iq1 este suele funcionar a la primera
static const u1_t PROGMEM APPKEY[16] = {0x86, 0x9C, 0xA6, 0x1D, 0x64, 0xE6, 0x1E, 0x10, 0x15, 0x35, 0x77, 0xCB, 0xF5, 0xF4, 0xD8, 0x40}; // iq6 este suele funcionar a la primera

void os_getDevKey(u1_t * buf) {
  memcpy_P(buf, APPKEY, 16);
}

ext::File root; 
ext::File dataFile;
ext::File myFile;

//For TTGO LoRa32 V2.x use:
U8X8_SSD1306_128X64_NONAME_HW_I2C display_(/*rst*/ U8X8_PIN_NONE);

static uint8_t mydata[55];
static int counter;

static osjob_t sendjob;
int runs = 0;
const unsigned TX_INTERVAL = 45;
// For TTGO LoRa32 V2.1.6
const lmic_pinmap lmic_pins = {
  .nss = LoRa_CS,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 23,
  .dio = {/*dio0*/ 26, /*dio1*/ 33, /*dio2*/ 32}
};


void printHex2(unsigned v) {
  v &= 0xff;
  if (v < 16)
    Serial.print('0');
  Serial.print(v, HEX);
}

void onEvent(ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;

    case EV_JOINED:
      Serial.println(F("EV_JOINED")); {
        u4_t netid = 0;
        devaddr_t devaddr = 0;
        u1_t nwkKey[16];
        u1_t artKey[16];
        LMIC_getSessionKeys( & netid, & devaddr, nwkKey, artKey);
        Serial.print("netid: ");
        Serial.println(netid, DEC);
        Serial.print("devaddr: ");
        Serial.println(devaddr, HEX);
        Serial.print("AppSKey: ");
        for (size_t i = 0; i < sizeof(artKey); ++i) {
          if (i != 0)
            Serial.print(" ");
          printHex2(artKey[i]);
        }
        Serial.println("");
        Serial.print("NwkSKey: ");
        for (size_t i = 0; i < sizeof(nwkKey); ++i) {
          if (i != 0)
            Serial.print(" ");
          printHex2(nwkKey[i]);
        }
        Serial.println();
        display_.drawString(0, 5, "Network Joined");
      }


      initSD();
      // Disable link check validation (automatically enabled
      // during join, but because slow data rates change max TX
      // size, we don't use it in this example.
      LMIC_setLinkCheckMode(0);
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      display_.drawString(0, 3, "Data Sent");
      WriteReadSD();
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));

        display_.drawString(0, 4, "Data received");
        Serial.println(F("Data is "));

        // Change the following codes to process incoming data !!
        for (int counter = 0; counter < LMIC.dataLen; counter++) {
          Serial.print(LMIC.frame[LMIC.dataBeg + counter], HEX);
        }
        Serial.println(F(" "));

      }
      else
        display_.drawString(0, 4, "        ");
      // Schedule next transmission
      os_setTimedCallback( & sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    case EV_TXSTART:
      Serial.println(F("EV_TXSTART"));
      break;
    case EV_TXCANCELED:
      Serial.println(F("EV_TXCANCELED"));
      break;
    case EV_JOIN_TXCOMPLETE:
      Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(250);
     pixels.setPixelColor(0, pixels.Color(255, 0, 255));
    pixels.show();   // Send the updated pixel colors to the hardware.
     pixels.show();   // Send the updated pixel colors to the hardware.
    delay(250);
      break;

    default:
      Serial.print(F("Unknown event: "));
      Serial.println((unsigned) ev);
      break;
  }
}

void do_send(osjob_t * j) {
  display_.clear();
  display_.drawString(0, 0, "LSenderV0.3TTL"); /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// version
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {

    ++counter;
    mydata[0] = 0; //este es el mensaje completo
    mensaje = "";
    if (err == false) {
  
      mensaje = mensaje + String(counter) + "," + String(volsolar) + "," + String(hum / 100);
   // mensaje = mensaje + String(voltageLevel) + "eN:" + String(counter) + "HS:" + String(hum / 100) + "%";
   //la estructura nueva de los mensajes estan separados por coma  + String(voltageLevel) +  "," +
    }
    if (err == true) {
      mensaje = mensaje + String(counter) + "," + String(volsolar) + ",err";
     
    }

    Serial.println(mensaje);
    int str_lenM = mensaje.length() + 1;  // esta funcion es para asignarle el valor de casillas al array
    char menC[str_lenM]; //este es el char array
    mensaje.toCharArray(menC, str_lenM);
    Serial.println(menC);
    strcat ((char*)mydata, menC); //copio el mensaje   ‘\0’ average

    // Prepare upstream data transmission at the next possible time. strlen((char*)mydata), 0)
    //LMIC_setTxData2(1, mydata, sizeof(mydata) + 1, 0);
    LMIC_setTxData2(1, mydata, sizeof(mydata)+1, 0);
    Serial.println(F("Packet queued"));
    display_.drawString(0, 2, "Packet Queued");
   pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(1000); // Pause before next pass through loop
  }
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();   // Send the updated pixel colors to the hardware.
 
}

void setup() {
  Serial.begin(115200);
  mySerial2.begin(115200, SERIAL_8N1, 4, 0);
   pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  uint32_t currentFrequency;

  Serial.println("Test esp32 RM95");


  display_.begin();
  display_.setFont(u8x8_font_pxplusibmcgathin_r);
  display_.drawString(0, 0, "TTGO ESP32 LoRa Test");
  Serial.println(F("Module Configured for CHILE BAND (AU915 MHz)"));


  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 5 / 100);
  //LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100);
  // Reset the MAC state. Session and pending data transfers will be discarded.
  // LMIC_reset();

  LMIC_setLinkCheckMode(0);
  LMIC_setAdrMode(false);
  LMIC_setDrTxpow(DR_SF7, 14);
  LMIC_enableSubBand(1);
  LMIC_selectSubBand(1);


  do_send( & sendjob);
  // set output pins
  pinMode(SD_CS, OUTPUT);
  pinMode(LoRa_CS, OUTPUT);
 
}
//================================================================ tiempo
unsigned long lastmillis = 0;
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 7400000;  //dos horas
//const unsigned long SECOND = 1000;
//const unsigned long HOUR = 3600*SECOND;/

/////////////==============================================================
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
 Serial.println("Restarting in 10 seconds");
  mySerial2.println('R');
  delay(1000);
 ESP.restart();
  }
   pixels.clear(); // Set all pixel colors to 'off'
   mySerial2.println(volsolar);
   
  // mySerial2.println('\r');
  // read from port 1, send to port 0:
  if (mySerial2.available()) {
    
    char recieved = mySerial2.read();
    inData += recieved;
    // Process message when new line character is recieved
    if (recieved == '\n')
    {

      Serial.print(" Serial 2: ");
      Serial.print(inData);
      Serial.print(" to hum: ");
      hum = inData.toFloat();
      Serial.print(hum / 100);
      // Length (with one extra character for the null terminator)
      int str_len =  inData.length() + 1;
      // Prepare the character array (the buffer)
      char char_array[str_len];
      // Copy it over
      inData.toCharArray(char_array, str_len);
      //Serial.println(char_array);
      if (char_array[0] == 'E') {
        pixels.setPixelColor(0, pixels.Color(250, 0, 0));
        pixels.show();   // Send the updated pixel colors to the hardwar
        //Serial.println( " error de sensor ");
        err = true;
        display_.setCursor(0, 7);  
        display_.print("err sensor: "+String(inData));
      } else {
        err = false;
        pixels.setPixelColor(0, pixels.Color(0, 250, 0));
        pixels.show();   // Send the updated pixel colors to the hardwar
       
      }
         // You can put some if and else here to process the message juste like that:
      inData = ""; // Clear recieved buffer
    }
  }

  int batR = analogRead(Vlev);
  int SolarR = analogRead(Slev);
  volsolar  =( (SolarR / 4095.0)* 3.3)*4.63; // calculate voltage level
  voltageLevel = (batR / 4095.0) * 2 * 1.1 * 3.3; // calculate voltage level
  display_.setCursor(0, 4);
  display_.print("humS " + String(hum / 100) + "%");
  display_.setCursor(0, 6);
  display_.print("B:" + String(voltageLevel) + "V-Bs:" + String(volsolar ) + "V");
   if (err == false) { 
      display_.drawString(0, 7, "Sensor OK");
    }
    
  os_runloop_once();
}
void WriteReadSD() {

  static uint32_t debs;
  if (millis() - debs >= 3000 && sd == true ) {
    debs = millis();
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.

    digitalWrite(LoRa_CS, DeSelect);
    digitalWrite(SD_CS, Select);    //  SELECT (Low) SD Card SPI
    myFile = SD.open(filenameCSV, FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) {
      Serial.print("Writing to " + String(filenameCSV));
      myFile.println(mensaje);
      // close the file:
      myFile.close();
      Serial.println("done.");
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening " + String(filenameCSV));
    }

    // re-open the file for reading:
    myFile = SD.open(filenameCSV);
    if (myFile) {
      Serial.println(filenameCSV);

      // read from the file until there's nothing else in it:
      while (myFile.available()) {
        Serial.write(myFile.read());
      }
      // close the file:
      myFile.close();
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening " + String(filenameCSV));
    }
    digitalWrite(SD_CS, DeSelect);
    delay( 1000 );
    digitalWrite(LoRa_CS, Select);   //  SELECT (low) LoRa SPI
  }


}
void initSD() {
  Serial.print("Initializing SD card...");
  digitalWrite(LoRa_CS, DeSelect);
  digitalWrite(SD_CS, Select);    //  SELECT (Low) SD Card SPI
  delay(100);
  /**/
  Serial.print("Initializing SD card...");
  pinMode(SD_CS, OUTPUT);    //  SELECT (Low) SD Card SPI
  /**/
  if (!SD.begin( SD_CS, SD_MOSI, SD_MISO, SD_CLK )) {
    Serial.println("initialization failed!");
    sd = false;
    return;
  }
  Serial.println("initialization done.");
  int n = 0;
  snprintf(filenameCSV, sizeof(filenameCSV), "data%03d.csv", n); // includes a three-digit sequence number in the file name
  while (SD.exists(filenameCSV)) {
    n++;
    snprintf(filenameCSV, sizeof(filenameCSV), "data%03d.csv", n);
  }
   dataFile = SD.open(filenameCSV, FILE_READ);
  Serial.println(n);
  Serial.println(filenameCSV);
  dataFile.close();

  root = SD.open("/");

  printDirectory(root, 0);

  Serial.println(" done!");

  digitalWrite(SD_CS, DeSelect);
  delay( 1000 );
  digitalWrite(LoRa_CS, Select);   //  SELECT (low) LoRa SPI



}

void printDirectory(ext::File dir, int numTabs) {
  
  while(true) {
     ext::File entry =  dir.openNextFile();
     if (! entry) {
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');   // we'll have a nice indentation
     }
     // Print the name
     Serial.print(entry.name());
     /* Recurse for directories, otherwise print the file size */
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       /* files have sizes, directories do not */
       Serial.print("\t\t");
       Serial.println(entry.size());
     }
     entry.close();
   }
}
