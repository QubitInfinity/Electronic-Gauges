#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <SPIFFS.h>

// TFT display pins
#define TFT_CS 14
#define TFT_RST 17
#define TFT_DC 15
#define LIGHTER_BLUE display.color565(100, 150, 255)

TaskHandle_t Task1;
float voltage;
int analogValue;

// const char* bmpFileName = "/temp.bmp";
const char* bmpFileName = "/fuel.bmp";
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

GFXcanvas16 canvas(239, 239);
// GFXcanvas16 imgCanvas(20, 20);

int bmpWidth = 240;
int bmpHeight = 240;
int rowSize = ((bmpWidth * 3 + 3) & ~3); // Rows are padded to 4-byte boundary
int padding = rowSize - bmpWidth * 3;    // Calculate padding bytes

// Define a buffer size (multiple rows or a set number of bytes)
const int BUFFER_SIZE = rowSize * 1; // Adjust this size based on available RAM
uint8_t* fileBuffer = (uint8_t*)malloc(BUFFER_SIZE);
uint16_t* screenBuffer = (uint16_t*)malloc(bmpWidth * sizeof(uint16_t));

int infoCount = 0;
// Needle properties
float needleAngle = -45;        // Starting angle in degrees (Empty)
float minAngle = -45;           // Minimum angle (Empty)
float maxAngle = 225;           // Maximum angle (Full)

// Needle center coordinates and length
int centerX = bmpWidth / 2;
int centerY = bmpHeight / 2;
int needleLength = 76;       // Length of the main part of the needle
int needleWidth = 5;         // Slightly increased width for better blanking
int bottomProtrusion = 30;   // Length of the protruding bottom part of the needle

float rad = needleAngle * PI / 180.0;
float radPerp = rad + PI / 2; // Perpendicular angle for current position

// Calculate the needle's top end coordinates
int xEnd = centerX + needleLength * cos(rad);
int yEnd = centerY - needleLength * sin(rad);

// Calculate the extended bottom end coordinates for the current position
int xBottom = centerX - bottomProtrusion * cos(rad);
int yBottom = centerY + bottomProtrusion * sin(rad);

// Calculate the offsets for needle thickness on both sides
int offsetX = needleWidth * cos(radPerp);
int offsetY = needleWidth * sin(radPerp);

// Pre-calculate coordinates for the current needle position
int x1 = centerX + offsetX;
int y01 = centerY - offsetY;
int x2 = centerX - offsetX;
int y2 = centerY + offsetY;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Setup starting ...");

  xTaskCreatePinnedToCore(
                          Task1code,   /* Task function. */
                          "Task1",     /* name of task. */
                          1000,       /* Stack size of task */
                          NULL,        /* parameter of the task */
                          1,           /* priority of the task */
                          &Task1,      /* Task handle to keep track of created task */
                          1);          /* loop() runs on 1 */
  delay(1000);

  analogSetAttenuation(ADC_11db);
  if (!fileBuffer || !screenBuffer) {
    Serial.println("Failed to allocate memory for buffers");
    return;
  }

  display.init(240, 240, SPI_MODE3);
  display.fillScreen(ST77XX_BLACK);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount file system");
    return;
  }
  // Render the BMP file directly from SPIFFS
  renderBMP();

  // imgCanvas.fillScreen(ST77XX_CYAN);

}

void loop() {
  unsigned long t1 = millis();
  // canvas.fillCircle(centerX, centerY, 76, ST77XX_WHITE);
  // canvas.drawRGBBitmap(80, 80, imgCanvas.getBuffer(), 20, 20);

  drawNeedle(); // Draw the needle at the current position
  display.drawRGBBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height());

  if(++infoCount == 100){
    Serial.println("Redraw time: " + String(millis() - t1));
    Serial.print("Analog: ");
    Serial.print(analogValue);
    Serial.print(", Voltage: ");
    Serial.println(voltage);
    Serial.print("Free DRAM: ");
    Serial.println(heap_caps_get_free_size(MALLOC_CAP_8BIT));

    // Free memory in IRAM (Instruction RAM)
    Serial.print("Free IRAM: ");
    Serial.println(heap_caps_get_free_size(MALLOC_CAP_EXEC));

    // Free memory that is DMA-capable
    Serial.print("Free DMA-capable Memory: ");
    Serial.println(heap_caps_get_free_size(MALLOC_CAP_DMA));
    infoCount = 0;
  }



  yield();

}

void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for(;;){
      for(int t = 0; t<2;t++){
        // read the input on analog pin GPIO36:
        analogValue = analogRead(36);
        // Rescale to potentiometer's voltage (from 0V to 3.3V):
        voltage = (voltage + floatMap(analogValue, 0, 4095, -45, 225))/2;
        delay(5);
      }

      needleAngle = needleAngle + (voltage - needleAngle)/3;
      delay(20);
  }
}

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void renderBMP() {
  File bmpFile = SPIFFS.open(bmpFileName, "r");
  if (!bmpFile) {
    Serial.println("Failed to open BMP file");
    return;
  }

  // Parse BMP header to skip to pixel data
  uint8_t header[54]; // BMP header is 54 bytes
  bmpFile.read(header, 54);
  int bufferIndex = 0; // Index within the buffer
  int bytesInBuffer = 0; // Number of valid bytes in the buffer

  // Read and render BMP line by line
  for (int y = bmpHeight - 1; y >= 0; y--) { // BMP rows are stored bottom to top
    // If there's not enough data in the buffer, refill it
    if (bufferIndex >= bytesInBuffer) {
      bytesInBuffer = bmpFile.read(fileBuffer, BUFFER_SIZE);
      bufferIndex = 0;
    }
    // Process each pixel in the current row
    for (int x = 0; x < bmpWidth; x++) {
      if (bufferIndex + 3 <= bytesInBuffer) {
        uint8_t b = fileBuffer[bufferIndex++]; // Blue component
        uint8_t g = fileBuffer[bufferIndex++]; // Green component
        uint8_t r = fileBuffer[bufferIndex++]; // Red component
        screenBuffer[x] = display.color565(r, g, b); // Convert to RGB565 format
      } else {
        // Handle edge case where buffer ends mid-row
        Serial.println("Buffer underrun");
        break;
      }
    }

    // Skip padding bytes if necessary
    bufferIndex += padding;

    // Draw the entire line
    canvas.drawRGBBitmap(0, y, screenBuffer, bmpWidth, 1);
  }

  bmpFile.close();
}

// Function to draw and blank the needle using two enlarged triangles
void drawNeedle() {
  // Blanking
  canvas.fillTriangle(x1, y01, x2, y2, xEnd, yEnd, ST77XX_WHITE);
  canvas.fillTriangle(x1, y01, x2, y2, xBottom, yBottom, ST77XX_WHITE);

  canvas.drawTriangle(x1, y01, x2, y2, xEnd, yEnd, ST77XX_WHITE);
  // tft.drawTriangle(x1, y01, x2, y2, xBottom, yBottom, ST77XX_WHITE);

  // Convert current angle from degrees to radians
   rad = needleAngle * PI / 180.0;
   radPerp = rad + PI / 2; // Perpendicular angle for current position

  // Calculate the needle's top end coordinates
   xEnd = centerX + needleLength * cos(rad);
   yEnd = centerY - needleLength * sin(rad);

  // Calculate the extended bottom end coordinates for the current position
   xBottom = centerX - bottomProtrusion * cos(rad);
   yBottom = centerY + bottomProtrusion * sin(rad);

  // Calculate the offsets for needle thickness on both sides
   offsetX = needleWidth * cos(radPerp);
   offsetY = needleWidth * sin(radPerp);

  // Pre-calculate coordinates for the current needle position
   x1 = centerX + offsetX;
   y01 = centerY - offsetY;
   x2 = centerX - offsetX;
   y2 = centerY + offsetY;

  // Draw the circular base before drawing the needle
  canvas.fillCircle(centerX, centerY, 14, LIGHTER_BLUE);

  // Draw the top portion of the needle as a filled triangle for thickness
  canvas.fillTriangle(x1, y01, x2, y2, xEnd, yEnd, ST77XX_ORANGE);

  // Draw the bottom portion of the needle as an extended, thicker section
  canvas.fillTriangle(x1, y01, x2, y2, xBottom, yBottom, ST77XX_ORANGE);

  // Draw the outline of the needle
  canvas.drawTriangle(x1, y01, x2, y2, xEnd, yEnd, ST77XX_RED);
  //tft.drawTriangle(x1, y01, x2, y2, xBottom, yBottom, ST77XX_RED);

  // Draw the circular base of the needle
  canvas.fillCircle(centerX, centerY, 10, ST77XX_BLUE);
}