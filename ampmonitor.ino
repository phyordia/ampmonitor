
#include <stdint.h>
#include <Wire.h> // library for I2C communication 
#include <SPI.h>
#include <Adafruit_ADS1015.h>// Hardware-specific library for ADS1115 16-bit ADC
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735 TFT display
#include "tft_defs.h"
 #include "EgnaterLogo.h"



// Initializing Devices
/**********************************
  ADS1115 16-bit ADC
************************************/

//0x48  Connect address pin to GND
//0x49  Connect address pin to VDD
//0x4A  Connect address pin to SDA
//0x4B  Connect address pin to SCL

Adafruit_ADS1115 ads(0x48);

/**********************************
  OLED Display
************************************/

// TFT DISPLAY Connections
// define not needed for all pins; reference for ESP32 physical pins connections to VSPI:
// SDA  GPIO23 aka VSPI MOSI
// SCLK GPIO18 aka SCK aka VSPI SCK
// D/C  GPIO21 aka A0 (also I2C SDA)
// RST  GPIO22 aka RESET (also I2C SCL)
// CS   GPIO5  aka chip select
// LED  3.3V
// VCC  5V
// GND - GND

#define TFT_DC   2       // register select (stands for Data Control perhaps!)
#define TFT_RST  4        // Display reset pin, you can also connect this to the ESP32 reset in which case, set this #define pin to -1!
#define TFT_CS   5       // Display enable (Chip select), if not enabled will not talk on SPI bus

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


/**********************************
  Global Variables
************************************/
// Temperature
float min_Temp = 999999.0;
float max_Temp = 0.0;

String oldTimeString;
String oldTempString;
String oldBiasString;

// Display Colors
uint16_t        Display_Text_Color         = Display_Color_Green;
uint16_t        Display_Backround_Color    = Display_Color_Black;


/**********************************
  Display Functions
************************************/

void drawBitmap(int16_t x, int16_t y,const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

  int16_t i, j, byteWidth = (w + 7) / 8;
  uint8_t byte;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      if(i & 7) byte <<= 1;
      else      byte   = pgm_read_byte(bitmap + j * byteWidth + i / 8);
      if(byte & 0x80) tft.drawPixel(x+i, y+j, color);
    }
  }
}


void displayString(String old_str, String new_str, int x, int y) {

  tft.setCursor(x, y);                       // yes! home the cursor
  tft.setTextColor(Display_Backround_Color); // change the text color to the background color
  tft.print(old_str);                        // redraw the old value to erase
  tft.setCursor(x, y);                       // home the cursor
  tft.setTextColor(Display_Text_Color);      // change the text color to foreground color
  tft.print(new_str);                        // draw the new time value
}


void displayTemp(float T) {
  String newString = "T: " + String(T) + "C  (" + String(min_Temp) + " - " + String(max_Temp) + ")"  ;
  displayString(oldTempString, newString, 0, 80);
  oldTempString = newString;
}

void displayBias(float mV) {
  String newString = "Bias (mV): " + String(mV) + "mV";// + String(min_Temp) + " - " + String(max_Temp) + ")"  ;
  displayString(oldBiasString, newString, 0, 100);
  oldBiasString = newString;
}


void displayUpTime() {
  unsigned long upSeconds = millis() / 1000;    // calculate seconds, truncated to the nearest whole second
  unsigned long days = upSeconds / 86400;       // calculate days, truncated to nearest whole day
  upSeconds = upSeconds % 86400;                // the remaining hhmmss are
  unsigned long hours = upSeconds / 3600;       // calculate hours, truncated to the nearest whole hour
  upSeconds = upSeconds % 3600;                 // the remaining mmss are
  unsigned long minutes = upSeconds / 60;       // calculate minutes, truncated to the nearest whole minute
  upSeconds = upSeconds % 60;                   // the remaining ss are

  //  // allocate a buffer
  char tmp_str[32] = { 0 };

  // construct the string representation
  sprintf(
    tmp_str,
    "Up Time: %02lu:%02lu:%02lu",
    hours, minutes, upSeconds
  );

  String newTimeString = String(tmp_str);
  displayString(oldTimeString, newTimeString, 0, 60);
  oldTimeString = newTimeString;
}



/**********************************
  Device Functions
************************************/

float getBias() {
  int adc = ads.readADC_Differential_0_1();
  float voltage = (adc * 0.1875);
  Serial.print("V12:");
  Serial.println(voltage, 7);
  return voltage;
  
}

float getTemp() {
  int ADC_Pin = 3;
  int adc = ads.readADC_SingleEnded(ADC_Pin);
  float voltage = (adc * 0.1875);
  float temp = 100*(voltage-500)/1000; // converts voltage into temperature 10mv=1C
  if (temp < min_Temp) min_Temp = temp;
  if (temp > max_Temp) max_Temp = temp;
  
  return temp;


}


/**********************************
  Array Functions
************************************/

int get_max_pos(float arr[], int len) {
  int m = 0;
  for (int i = 0; i < len; ++i) {
    if (arr[i] >= arr[m])
      m = i;
  }
  return m;
}

int get_min_pos(float arr[], int len) {
  int m = 0;
  for (int i = 0; i < len; ++i) {
    if (arr[i] <= arr[m])
      m = i;


  }
  return m;
}

float get_smooth_avg(float arr[], int len, int min_pos, int max_pos) {
  float sum_val = 0.0;

  for (int i = 0; i < len; ++i) {
    if (i == min_pos || i == max_pos) continue;
    sum_val += arr[i];
  }

  return sum_val / (len - 2);
}



/**********************************
  Main Functions
************************************/

void setup() {
  Serial.begin(115200);

  // settling time
  Serial.println (" in setup");

  // initialise the display
  tft.initR();
  tft.setFont();
  tft.setRotation(1);
  tft.fillScreen(Display_Backround_Color);
  tft.setTextColor(Display_Text_Color);
  tft.setTextSize(1);

  // Initialize ADS
  ads.begin();

  // Draw Logo
    tft.drawXBitmap(0, 0, EgnaterLogo_bits, EgnaterLogo_width, EgnaterLogo_height, Display_Color_Yellow);

//    tft.drawBitmap(0, 0, EgnaterLogo, 140, 50, Display_Color_Yellow);

}

void loop() {
  //  tft.fillScreen(Display_Backround_Color);
  //  for (int i = 0; i < 160; ++i)
  //    tft.drawFastVLine(i, 0, tft.height(), Display_Backround_Color);
  //  display.clear();
  //  displayBias(getBias());

  // Get Samples
  //
  //  int nsamples = 10;
  //  float Temps[nsamples] = {0.0};
  //
  //  // Sample
  //  for (int i = 0; i < nsamples; ++i) {
  //
  //    Temps[i] = getTemp();
  //    delay(1000/nsamples);
  //  }
  //
  //  // Smooth samples
  //  int min_pos_Temp = get_min_pos(Temps, nsamples);
  //  int max_pos_Temp = get_max_pos(Temps, nsamples);
  //  float avg_Temp = get_smooth_avg(Temps, nsamples, min_pos_Temp, max_pos_Temp);
  //
  //  for (int i = 0; i < nsamples; ++i) {
  //    Serial.print(Temps[i]);
  //    Serial.print(", ");
  //  }
  //  Serial.println("");
  //  Serial.println("Min: " + String(Temps[min_pos_Temp]));
  //  Serial.println("Min: " + String(Temps[max_pos_Temp]));
  //  Serial.println("Avg: " + String(avg_Temp));
  //
  //    if(avg_Temp < min_Temp) min_Temp = avg_Temp;
  //    if(avg_Temp > max_Temp) max_Temp = avg_Temp;
  //
  // Display




  displayUpTime();
  displayTemp(getTemp());
  displayBias(getBias());
  delay(1000);

}
