#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <WiFi.h>
#include "time.h"

// WiFi credentials
const char* ssid = "***";
const char* password = "***";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7200; // GMT+2
const int daylightOffset_sec = 0; // No daylight saving

struct tm timeinfo;

#define TFT_CS   14
#define TFT_RST  17
#define TFT_DC   15
#define TFT_MOSI 23
#define TFT_SCLK 18  

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
uint16_t DARKER_RED = tft.color565(139, 0, 0);
uint16_t AMBER = ST77XX_ORANGE;

// Variable to store the previous time to reduce updates
char previousTime[9] = "";
char currentTime[9] = "00:00:00";

void setup(void) {
  Serial.begin(52700);
  Serial.println(F("Initializing display and WiFi..."));

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  } 
  printESP32Capabilities();
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime(&timeinfo);
  
  // Initialize the display
  tft.init(240, 240, SPI_MODE3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  displayTimeSequence();
  
}

void loop() {
  displayLocalTime();
  delay(1000);
}

void displayLocalTime() {
  if (!getLocalTime(&timeinfo)) {
    tft.setCursor(0, 100);
    tft.setTextColor(ST77XX_RED);
    tft.println("ERROR");
    return;
  }

  // Format the time as HH:MM:SS
  strftime(currentTime, sizeof(currentTime), "%H:%M:%S", &timeinfo);

  // Check if the time has changed to avoid unnecessary updates
  if (strcmp(currentTime, previousTime) != 0) {
    drawNixieTime();
  }
}

void drawNixieTime() {
  // Set the position and style for drawing digits
  int x = 5; // Starting X position
  int y = 90; // Starting Y position
  int digitWidth = 32; // Width of each digit space

  for (int i = 0; i < strlen(currentTime); i++) {
    // Adjust position for colon if needed
    if (currentTime[i] == ':') x -= 7;

    // Draw the character, including blacking out the previous one
    drawNixieDigit(previousTime[i], currentTime[i], x, y);
    x += digitWidth; // Move to the next position

    // Adjust position back after colon
    if (currentTime[i] == ':') x -= 5;
  }
  strcpy(previousTime, currentTime);
}

void drawNixieDigit(char previousDigit, char currentDigit, int x, int y) {
  // Set the font size
  tft.setTextSize(5);

  // Black out the previous digit
  tft.setTextColor(ST77XX_BLACK); // Set color to black to clear
  tft.setCursor(x+2, y+2);
  tft.print(previousDigit); // Clear the previous digit by drawing it in black

  // Optional: Add slight offsets for a glow effect

  tft.setTextColor(DARKER_RED);
  tft.setCursor(x, y);
  tft.print(currentDigit);

  // Draw the main digit in the desired color  
  tft.setTextColor(AMBER); // Use a slightly different color for the glow effect
  tft.setCursor(x + 2, y + 2); // Offset the glow
  tft.print(currentDigit);
}

void displayTimeSequence() {
  // Loop to display "11:11:11" to "99:99:99"
  for (int i = 0; i <= 9; i++) {
    // Construct the time string with the same digit repeated
    sprintf(currentTime, "%d%d:%d%d:%d%d", i, i, i, i, i, i);
    drawNixieTime();
    delay(200);
  }
}

void printESP32Capabilities() {
  // Print chip model and revision
  Serial.print("Chip Model: ");
  Serial.println(ESP.getChipModel());
  
  Serial.print("Chip Revision: ");
  Serial.println(ESP.getChipRevision());

  // Print the number of CPU cores
  Serial.print("CPU Cores: ");
  Serial.println(ESP.getChipCores());

  // Print CPU frequency
  Serial.print("CPU Frequency: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");

  // Print available Flash size
  Serial.print("Flash Size: ");
  Serial.print(ESP.getFlashChipSize() / (1024 * 1024));
  Serial.println(" MB");

  // Print available Heap and PSRAM size
  Serial.print("Free Heap: ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println(" KB");

  Serial.print("Free PSRAM: ");
  Serial.print(ESP.getFreePsram() / 1024);
  Serial.println(" KB");

  // Print the Wi-Fi MAC address
  Serial.print("Wi-Fi MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Print Wi-Fi capabilities
  Serial.println("\nWi-Fi Capabilities:");
  Serial.print("Wi-Fi Mode: ");
  Serial.println(WiFi.getMode());

  Serial.print("Max TX Power: ");
  Serial.print(WiFi.getTxPower());
  Serial.println(" dBm");

  // Check if Bluetooth is available (for ESP32 with Bluetooth support)
  #ifdef CONFIG_BT_ENABLED
  Serial.println("Bluetooth: Available");
  #else
  Serial.println("Bluetooth: Not Available");
  #endif

  // Check if PSRAM is available
  if (ESP.getPsramSize() > 0) {
    Serial.print("PSRAM Size: ");
    Serial.print(ESP.getPsramSize() / (1024 * 1024));
    Serial.println(" MB");
  } else {
    Serial.println("PSRAM: Not Available");
  }

  // Print the SDK version
  Serial.print("SDK Version: ");
  Serial.println(ESP.getSdkVersion());
}
