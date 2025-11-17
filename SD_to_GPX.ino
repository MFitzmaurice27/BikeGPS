// For GPS:
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <math.h>
#include <LiquidCrystal.h>

// For SD Card:
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#define speedPoints 10

double haversine(double lat1, double long1, double lat2, double long2) { // Returns distance between two coordinates
  const double R = 3963.1; // Earth's radius in miles

  // Convert degrees to radians (trig function in arduino use radians)
  double dLat = radians(lat2 - lat1);
  double dLong = radians(long2 - long1);

  // Haversine
  double a = sin(dLat / 2) * sin(dLat / 2) + cos(radians(lat1)) * cos(radians(lat2)) * sin(dLong / 2) * sin(dLong / 2);
  double c = 2 * R * sin(sqrt(a));

  return R * c; // Distance in miles
}

double speedAvg(double array[]) {
  double sum = 0;

  for (int i = 0; i <= speedPoints; i++) {
    sum += array[i];
  }

  return sum/speedPoints;
}

// Create GPS parser object
TinyGPSPlus gps;

// Create LCD object
#define LCD_RS  22
#define LCD_E   23
#define LCD_D4  12
#define LCD_D5  27
#define LCD_D6  33
#define LCD_D7  15
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Use UART1 on ESP32 (pins vary by board)
HardwareSerial gpsSerial(1);

// SD Card Pin Definitions
#define SCK_PIN 5
#define MISO_PIN 19
#define MOSI_PIN 18
#define SD_CS A5

// Button Pins
#define redButton 14
#define greenButton 32
#define blueButton 13

// Tracks # of coordinate sets input
int logNum = 1;
int maxLogs = 500;
bool ended = 0;

// Variables
String fileName;
int fileNum = 0;
double distance = 0;
double timeIncrement = 50; // Time increment between point logs
int timeElapsed; // Total time elapsed since start of run
int logTime = 1000; // Time between data logs
double avgSpeed = 0;
double currentSpeed = 0;
int state = 0;
double speedArray[speedPoints]; // Used to calculate currentSpeed
int speedIndex = 0;
int dataShown = 1; // Determines the data shown on the display (time, distance, current speed, avg speed)
int numReadings = 4; // number of datas that can be shown on the display (currently 4)
bool blueToggle = 0; // Toggles so button doesn't react many times
bool fileBool = 0;

// Used for calculating distance
double prevLat;
double prevLong;
double newLat;
double newLong;

// Define state values
#define INIT 0;
#define WAIT 1;
#define CREATE 2;
#define TRACK 3;
#define END 4;

// Declare GPS File
File testFile;

void setup() {
  // Serial output applies for both GPS and SD Card:
  Serial.begin(115200);

  // Set up button pins
  pinMode(redButton, INPUT);
  pinMode(greenButton, INPUT);
  pinMode(blueButton, INPUT);

  // Set up SD Card SPI:
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SD_CS);

  // Set up GPS Serial
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17); 

  // Set up LCD
  lcd.begin(16, 2);
  lcd.clear();

  delay(1000);
  Serial.println("\nSD Card Test Starting...");

  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed!");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %llu MB\n", cardSize);
}

void loop() {
  if (state == 0) { // INIT STATE ------------------------------------------------------------------------------
    Serial.println("INIT STATE");
    fileNum = 0;
    distance = 0;
    avgSpeed = 0;
    logNum = 0;

    state = WAIT;

    fileBool = 0; // Used to cycle through files.

    while (fileBool == 0) {
      fileName = "/GPS_" + String(fileNum) + ".gpx";
      if (SD.exists(fileName)) {
        fileNum++;
      } else {
        fileBool = 1;
      }
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INIT STATE      ");
  }


  if (state == 1) { // WAIT STATE ----------------------------------------------------------------------------
    Serial.println("WAIT STATE");
    lcd.setCursor(0, 0);
    lcd.print("WAIT STATE      ");

    if (digitalRead(greenButton) == HIGH) {
      state = CREATE;
    } else {
      state = WAIT;
    }


  } else if (state == 2) { // CREATE STATE -----------------------------------------------------------------------
    Serial.println("CREATE STATE");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CREATE STATE    ");
    /*while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }*/

    // Resets variables for a new file to be created
    distance = 0;
    avgSpeed = 0;
    logNum = 0;

    // Write introductory data to GPX file
    // File testFile = SD.open("/GPS_1.txt", FILE_WRITE);
    fileName = "/GPS_" + String(fileNum) + ".gpx";
    testFile = SD.open(fileName, FILE_WRITE);
    if (testFile) {
      testFile.println("<?xml version=\"1.0\"?>\n<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\nxmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1\nhttp://www.topografix.com/GPX/1/1/gpx.xsd\" version=\"1.1\" creator=\"Starpath School of Navigation - http://www.starpath.com\">\n<rte>\n<name>HelpMe</name>");
      testFile.close();
      //Serial.println("Wrote intro text");
    } else {
      Serial.println("Failed to create file");
    }

    state = TRACK;
    timeElapsed = 0;


  } else if (state == 3) { // TRACK STATE ----------------------------------------------------------------------

    Serial.println("TRACK STATE");
    lcd.setCursor(0, 0);
    lcd.print("TRACK STATE     ");

    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }

    if (timeElapsed % logTime == 0) {
      if (gps.location.isUpdated()) {

        prevLat = newLat;
        prevLong = newLong;

        /*Serial.print("Latitude:  ");
        Serial.println(gps.location.lat(), 6);
        Serial.print("Longitude: ");
        Serial.println(gps.location.lng(), 6);
        Serial.print("Satellites: ");
        Serial.println(gps.satellites.value());
        Serial.print("HDOP: ");
        Serial.println(gps.hdop.value());
        Serial.print("Altitude (m): ");
        Serial.println(gps.altitude.meters());
        Serial.println("--------------------");*/

        // File testFile = SD.open("/GPS_1.txt", FILE_WRITE);
        fileName = "/GPS_" + String(fileNum) + ".gpx";
        testFile = SD.open(fileName, FILE_APPEND);
        if (testFile) {
          newLat = gps.location.lat();
          newLong = gps.location.lng();
          testFile.println("<rtept lat=\"" + String(gps.location.lat(), 6) + "\" lon=\"" + String(gps.location.lng(), 6) + "\">\n<name>" + logNum + "</name>\n</rtept>");
          testFile.close();
          //Serial.println("Wrote to GPS file " + logNum);
        } else {
          Serial.println("Failed to write");
        }

        if (logNum != 0) {
          distance += haversine(newLat, newLong, prevLat, prevLong);
          avgSpeed = (3600000*distance)/(timeIncrement*logNum);
          speedArray[speedIndex] = ((3600000*haversine(newLat, newLong, prevLat, prevLong))/timeIncrement);
        }

        if (logNum >= speedPoints) {
          currentSpeed = speedAvg(speedArray);
        }
      }

      if (digitalRead(blueButton) == HIGH) {
        if (blueToggle == 0) {
          if (dataShown >= numReadings) {
            dataShown = 1;
          } else {
            dataShown ++;
          }
        }
        blueToggle = 1;
      } else {
        blueToggle = 0;
      }

      Serial.println("Total distance: " + String(distance, 4));
      if (dataShown == 1) {
        lcd.setCursor(0, 1);
        lcd.print("Distance: " + String(distance, 4));
      } else if (dataShown == 2) {
        lcd.setCursor(0, 1);
        lcd.print("C-Speed: " + String(currentSpeed, 4));
      } else if (dataShown == 3) {
        lcd.setCursor(0, 1);
        lcd.print("AvgSpeed: " + String(avgSpeed, 4));
      } else if (dataShown == 4) {
        lcd.setCursor(0, 1);
        if ((timeElapsed / 1000) % 60 < 10) {
          lcd.print("Time:   " + String(int(timeElapsed / 60000)) + ":0" + int((timeElapsed / 1000) % 60) + "       ");
        } else {
          lcd.print("Time:   " + String(int(timeElapsed / 60000)) + ":" + int((timeElapsed / 1000) % 60) + "       ");
        }
      } else {
        lcd.setCursor(0, 1);
        lcd.print("Kill Yourself");
      }

      logNum++;
    }

    if (digitalRead(redButton) == HIGH) {
      state = END;
    } else {
      state = TRACK;
    } 


  } else if (state == 4) { // END STATE ------------------------------------------------------------------------
    Serial.println("END STATE       ");
    lcd.setCursor(0, 0);
    lcd.print("END STATE");

    fileName = "/GPS_" + String(fileNum) + ".gpx";
    testFile = SD.open(fileName, FILE_APPEND);
    if (testFile) {
      testFile.println("</rte>\n</gpx>");
      testFile.close();
      //Serial.println("Wrote ending to GPS file");
    } else {
      Serial.println("Failed to write");
    }

    fileNum++;
    state = WAIT;
  }

  delay(timeIncrement);
  timeElapsed += timeIncrement;
  Serial.println(timeElapsed);
}
