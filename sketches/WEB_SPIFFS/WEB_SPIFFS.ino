#include <const.h>
#include <SPIFFS.h>

#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
// const char* ssid = "YOUR_WIFI_NAME";
// const char* password = "YOUR_WIFI_PASSWORD";

// Web server URL for the BMP file
const char* bmpFile = "fuel.bmp";
const char* bmpURL = "https://raw.githubusercontent.com/QubitInfinity/Guage/refs/heads/main";

const bool forceDownload = false;

void setup() {
  Serial.begin(57600);
  Serial.println("Setup Started ...");
  delay(500);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount file system");
    return;
  }

  // Check if the BMP file exists; fetch from web only if not found
  if (forceDownload || !SPIFFS.exists("/" + String(bmpFile) )) {
    Serial.println("BMP file not found on SPIFFS or overwriting, fetching from web: " + String(bmpURL) + "/" +String(bmpFile));
    if (!fetchBMPfromWeb()) {
      Serial.println("Failed to fetch BMP file from web");
      return;
    }
  } else {
    Serial.println("BMP file found on SPIFFS");
  }
  listDir("/");

}

void loop() {
}

// Function to list files in a directory
void listDir(const char *dirname) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = SPIFFS.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("DIR : ");
      Serial.println(file.name());
    } else {
      Serial.print("FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

bool fetchBMPfromWeb() {
  HTTPClient http;
  http.begin(String(bmpURL) + "/" + String(bmpFile));
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    File file = SPIFFS.open("/" + String(bmpFile) , FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      http.end();
      return false;
    }

    // Stream the data from the web directly into the file
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[250];
    int len;
    int retry = 10;

    while (retry > 0) {
      len = stream->readBytes(buffer, sizeof(buffer));
      if (len > 0) {
        file.write(buffer, len);  // Write buffer to file
        retry = 10;  // Reset retry count when data is read successfully
      } else if (len == 0) {
        retry--;  // Decrease retry count when no data is read
        delay(20);  // Short delay before retrying
      } else {
        break;  // Break if len is negative (stream error)
      }
    }

    file.close();
    Serial.println("BMP downloaded and saved successfully");
  } else {
    Serial.printf("HTTP GET failed, error: %d\n", httpResponseCode);
    http.end();
    return false;
  }

  http.end();
  return true;
}

