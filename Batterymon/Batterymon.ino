/*  Bluetooth Battery monitor
    
    By Alessandro Nesta 2024
    
    Youtube channel: https://www.youtube.com/@alessandronestaadventures

    This is a Battery monitor that connects directly to Battery BMS.
    It has been tested on Eco-Worthy 12V 100Ah Battery, but it should work with most of the chinese made battery and BMS.

    The project is inspired by: https://github.com/kolins-cz/Smart-BMS-Bluetooth-ESP32

    LVGL has been used for the GUI.

    Hardware: ESP32-2432S028 commonly known as CYD (Cheap Yellow Display)
    The code can be easily adapted to any other touchscreen display.

    The boot logo can be changed using the LVGL Image converter (https://lvgl.io/tools/imageconverter) and replacing ghe bootlogo.c file.
    With this code, the boot image is the Wingamm logo, because I have a Wingamm camper. You can replace with any logo, just be sure to match your screen resolution.

    This first version works with one battery.

    Future plan:
    - Settings and preferences screen.
    - Possibility to monitor multiple batteries in parallel.

    Libraries version:
    lvgl by kisvegabor version 9.2.2
    TFT_eSPI by Bodmer version 2.5.43
    XPT2046_Touchscreen by Paul Stoffegen version 1.4

    Please note that you need to configure TFT_eSPI and LVGL library to match your Display configuration.
    In this repository you will find the files for my CYD display which is the version with also USB-C.

    The files are: User_Setup.h and lv_conf.h
    The path are: 
    "Your Sketchbook location"\libraries\TFT_eSPI\User_Setup.h
    "Your Sketchbook location"\libraries\lv_conf.h

    Please note that depending on your LCD, you may have different configuration for these 2 files.
*/

#include <lvgl.h>

#include <TFT_eSPI.h>

#include <XPT2046_Touchscreen.h>

// include bluetooth libraries
#include "BLEDevice.h"
#include "BLEScan.h"
#include "mydatatypes.h"

// library for deep sleep
#include "esp_sleep.h"

// library for permanently storing setup data
#include <Preferences.h>

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define SLEEP_TIMER 120   //number of seconds to go to sleep

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

lv_obj_t * batvolt, * batlevel, * batamp, * battemp, * arc, * statusstring;

//#define TRACE commSerial.println(__FUNCTION__)
#define TRACE

//HardwareSerial Serial(0);

//---- global variables ----

static boolean doConnect = false;
static boolean BLE_client_connected = false;
static boolean doScan = false;

packBasicInfoStruct packBasicInfo;  //here shall be the latest data got from BMS
packEepromStruct packEeprom;        //here shall be the latest data got from BMS
packCellInfoStruct packCellInfo;    //here shall be the latest data got from BMS

const byte cBasicInfo3 = 3;  //type of packet 3= basic info
const byte cCellInfo4 = 4;   //type of packet 4= individual cell info

unsigned long previousMillis = 0;
unsigned long previousMillisl = 0;
const long myinterval = 5000;
const long interval = 4000;

bool toggle = false;
bool newPacketReceived = false;
int i = 0;

int counter = 0;
int sleeptimer = SLEEP_TIMER; 

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

// Get the Touchscreen data
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;

    // Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
    /*Serial.print("X = ");
    Serial.print(x);
    Serial.print(" | Y = ");
    Serial.print(y);
    Serial.print(" | Pressure = ");
    Serial.print(z);
    Serial.println();*/
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void draw_image(void) {
  LV_IMAGE_DECLARE(bootlogo);
  lv_obj_t * img1 = lv_image_create(lv_screen_active());
  lv_image_set_src(img1, &bootlogo);
  lv_obj_align(img1, LV_ALIGN_CENTER, 0, 0);
}

void lv_create_main_gui(void) {
  // Create a text label aligned center on top ("Hello, world!")
  lv_obj_t * text_label = lv_label_create(lv_screen_active());
  lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);    // Breaks the long lines
  lv_label_set_text(text_label, "Bluetooth BMS Battery Monitor");
  lv_obj_set_width(text_label, 300);    // Set smaller width to make the lines wrap
  lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lv_scr_act(), lv_color_white(), LV_PART_MAIN);
  lv_obj_align(text_label, LV_ALIGN_CENTER, 0, -100);

  // Bluetooth symbol
  lv_obj_t * text_label1 = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_font(text_label1, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(text_label1, LV_COLOR_MAKE(0x00,0x00,0xFF), 0);
  lv_label_set_text(text_label1, LV_SYMBOL_BLUETOOTH);
  lv_obj_align(text_label1, LV_ALIGN_TOP_RIGHT, -10, 10);

  // Arc with Battery percent
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_arc_color(&style, lv_color_hex(0x222222));
  
  arc = lv_arc_create(lv_screen_active());
  lv_obj_set_size(arc, 150, 150);
  lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
  lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_style(arc, &style, 0);
  lv_arc_set_rotation(arc, 135);
  lv_arc_set_bg_angles(arc, 0, 270);
  lv_arc_set_value(arc, 0);
  lv_obj_center(arc);

  batlevel = lv_label_create(lv_screen_active());
  lv_label_set_text(batlevel, "0 %");
  lv_obj_set_style_text_font(batlevel, &lv_font_montserrat_32, 0);
  lv_obj_center(batlevel);
  //lv_obj_align_to(batlevel, arc, LV_ALIGN_CENTER, 0, 0);

  // Show current in AMPERE
  batamp = lv_label_create(lv_screen_active());
  lv_label_set_text(batamp, "0.00 A");
  lv_obj_set_style_text_font(batamp, &lv_font_montserrat_18, 0);
  lv_obj_align(batamp, LV_ALIGN_RIGHT_MID, -10, -40);

  // Show battery temperature
  battemp = lv_label_create(lv_screen_active());
  lv_label_set_text(battemp, "0.00 °C");
  lv_obj_align(battemp, LV_ALIGN_RIGHT_MID, -10, 40);

  // show battery voltage
  batvolt = lv_label_create(lv_screen_active());
  lv_label_set_text(batvolt, "0.00 V");
  lv_obj_set_style_text_font(batvolt, &lv_font_montserrat_20, 0);
  lv_obj_align(batvolt, LV_ALIGN_LEFT_MID, 10, -40);

  statusstring = lv_label_create(lv_screen_active());
  lv_label_set_text(statusstring, "Starting");
  lv_obj_align(statusstring, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void lcdConnectingStatus(uint8_t state) {
  switch (state) {
    case 0:
      lv_label_set_text(statusstring, "connecting to BMS...");
      break;
    case 1:
      lv_label_set_text(statusstring, "created client");
      break;
    case 2:
      lv_label_set_text(statusstring, "connected to server");
      break;
    case 3:
      lv_label_set_text(statusstring, "service not found");
      break;
    case 4:
      lv_label_set_text(statusstring, "found service");
      break;
    case 5:
      lv_label_set_text(statusstring, "char. not found");
      break;
    case 6:
      lv_label_set_text(statusstring, "Connected");
      break;
    default:
      break;
  }
}

void showInfoLcd() {

  TRACE;
  String voltage = String((float)packBasicInfo.Volts / 1000) + " V";
  lv_label_set_text(batvolt, voltage.c_str());

  String amps = String((float)packBasicInfo.Amps / 1000) + " A";
  lv_label_set_text(batamp, amps.c_str());

  String percent = String(packBasicInfo.CapacityRemainPercent) + " %";
  lv_label_set_text(batlevel, percent.c_str());
  lv_arc_set_value(arc, packBasicInfo.CapacityRemainPercent);

  String temp = String((float)packBasicInfo.Temp1 / 10) + " °C";
  lv_label_set_text(battemp, temp.c_str());

}



void setup() {
  //String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  //Serial.begin(115200);
  //Serial.println(LVGL_Arduino);
  
  // Setup wakeup after deep sleep just touching the display.
  esp_sleep_enable_ext1_wakeup(0x1000000000, ESP_EXT1_WAKEUP_ALL_LOW);
  //ESP_EXT1_WAKEUP_ANY_HIGH
  //ESP_EXT1_WAKEUP_ALL_LOW
  //PIN: XPT2046_IRQ (GPIO#36)

  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
  touchscreen.setRotation(2);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  //background color?
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_STATE_DEFAULT);
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // Set the callback function to read Touchscreen input
  lv_indev_set_read_cb(indev, touchscreen_read);

  //show boot logo
  draw_image();
  lv_task_handler();
  Serial.begin(115200, SERIAL_8N1, 3, 1);
  Serial.println("Starting BMS dashboard application...");
  bleStartup();
  bleRequestData();
  delay(5000);
  lv_tick_inc(5000);
  lv_obj_clean(lv_scr_act());
  
  // Function to draw the GUI (text, buttons and sliders)
  lv_create_main_gui();


}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  counter++;
  if (counter == 200) {
    bleRequestData();
    counter = 0;
    sleeptimer--;
  }
  //Reset Timer if screen is touched.
  if (touchscreen.touched()) { sleeptimer = SLEEP_TIMER; }
  if (sleeptimer == 0) { esp_deep_sleep_start(); }
  delay(5);           // let this time pass
}