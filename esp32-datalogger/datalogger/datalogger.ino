#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// USING LVGL Arduino V9.1.0
// HAD TO MODIFY LIB, REPLACED 'esp_ptr_dma_capable(data)' WITH 'false'

#include <lvgl.h>
#include <Arduino_GFX_Library.h>

#define PIN_BL   D5
#define PIN_DC   D3
#define PIN_CS   D4 
#define PIN_SCK  SCK
#define PIN_MOSI MOSI
#define PIN_MISO -1
#define PIN_RST  D9

#define ROTATION 1
#define IS_IPS   true

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, PIN_MISO);
Arduino_GFX *gfx = new Arduino_ST7789(bus, PIN_RST, ROTATION, IS_IPS);

/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

static lv_obj_t * label;

uint32_t screenWidth;
uint32_t screenHeight;
uint32_t bufSize;
lv_display_t *disp;
lv_color_t *disp_draw_buf;

#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char *buf)
{
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}
#endif

uint32_t millis_cb(void)
{
  return millis();
}

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);

  /*Call it to tell LVGL you are ready*/
  lv_disp_flush_ready(disp);
}


static uint32_t test = 0;

BLEAdvertising *pAdvertising = NULL;
char mfgData[8] = {0xFF, 0xAB, 0x01, 0x02, 0x00, 0x00, 0x00, 0x01};





static void draw_event_cb(lv_event_t * e)
{
    lv_draw_task_t * draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t * base_dsc = (lv_draw_dsc_base_t*)draw_task->draw_dsc;

    if(base_dsc->part != LV_PART_ITEMS) {
        return;
    }

    lv_draw_fill_dsc_t * fill_dsc = lv_draw_task_get_fill_dsc(draw_task);
    if(fill_dsc) {
        lv_obj_t * chart = (lv_obj_t*)lv_event_get_target(e);
        int32_t * y_array = lv_chart_get_y_array(chart, lv_chart_get_series_next(chart, NULL));
        int32_t v = y_array[base_dsc->id2];

        uint32_t ratio = v * 255 / 100;
        fill_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_GREEN), lv_palette_main(LV_PALETTE_RED), ratio);
    }
}

/**
 * Recolor the bars of a chart based on their value
 */
#define NUM_CHART_DATA_POINTS 40
static lv_obj_t * chart;
static lv_chart_series_t * chart_series;
static uint8_t chartDataPoints[NUM_CHART_DATA_POINTS] = {0};

void lv_example_chart_4(void)
{
    /*Create a chart1*/
    chart = lv_chart_create(lv_screen_active());
    lv_chart_set_type(chart, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(chart, NUM_CHART_DATA_POINTS);
  
    lv_chart_set_div_line_count(chart, 0, 0);

    // lv_chart_set_div_line_count(chart, 3, 5);
    // lv_obj_set_style_pad_all(chart, -2, 0); // Remove outer division padding

    lv_obj_set_style_radius(chart, 0, 0); // Remove rounded corners
    lv_obj_set_style_pad_column(chart, 1, 0); // Set padding between bars
    lv_obj_set_style_border_side(chart, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_color(chart, lv_color_black(), 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x474747), 0);
    lv_obj_set_size(chart, screenWidth, 100);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, 0);

    chart_series = lv_chart_add_series(chart, lv_color_hex(0xff0000), LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_add_event_cb(chart, draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(chart, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    // uint32_t i;
    // for(i = 0; i < NUM_CHART_DATA_POINTS; i++) {
    //     lv_chart_set_next_value(chart, chart_series, lv_rand(10, 90));
    // }
}


void setup()
{
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("MR2");

  // Start advertising
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

  BLEDevice::startAdvertising();


  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino_GFX LVGL_Arduino example v9");

  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println(LVGL_Arduino);

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(GREEN);

  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);

  lv_init();

  /*Set a tick source so that LVGL will know how much time elapsed. */
  lv_tick_set_cb(millis_cb);

  /* register print function for debugging */
#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print);
#endif

  screenWidth = gfx->width();
  screenHeight = gfx->height();

  bufSize = screenWidth * 40;

  disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf)
  {
    // remove MALLOC_CAP_INTERNAL flag try again
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
  }

  if (!disp_draw_buf)
  {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  }
  else
  {
    disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, disp_draw_buf, NULL, bufSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), LV_PART_MAIN);

    label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello Arduino, I'm LVGL!(V" GFX_STR(LVGL_VERSION_MAJOR) "." GFX_STR(LVGL_VERSION_MINOR) "." GFX_STR(LVGL_VERSION_PATCH) ")");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
  }

  lv_example_chart_4();

  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  Serial.println("Setup done");
}

static uint32_t counter = 0;
void loop() {
  uint16_t tempSensorVoltageMv = analogReadMilliVolts(A2);
  uint16_t tempMvConstrained = constrain(tempSensorVoltageMv, 1276, 1997);
  int16_t tempC = (int16_t)(198.0 - 0.572*tempMvConstrained + 0.000302*tempMvConstrained*tempMvConstrained);

  uint16_t pressureSensorVoltageMv = analogReadMilliVolts(A1);
  uint8_t oilPressurePSI = (uint8_t)((0.0382 * pressureSensorVoltageMv) - 12.5);

  #define LABEL_STRING (" Pressure Sensor Voltage: %d mV \n" \
                        " Oil Pressure: %d PSI \n\n"         \
                        " Temp Sensor Voltage: %d mV \n"     \
                        " Oil Temp: %d C")

  lv_label_set_text_fmt(label, LABEL_STRING, pressureSensorVoltageMv, oilPressurePSI, tempSensorVoltageMv, tempC);

  if ((counter % 200) == 0U)
  {
    lv_chart_set_next_value(chart, chart_series, lv_rand(10, 90));
  }

  lv_task_handler(); /* let the GUI do its work */

  mfgData[3] = (mfgData[3] + 1);
  mfgData[5] = (mfgData[3] + 1);

  BLEAdvertisementData advertisementData = BLEAdvertisementData();
  advertisementData.setManufacturerData(String(mfgData, sizeof(mfgData)));
  pAdvertising->setAdvertisementData(advertisementData);

  delay(5);
  counter++;
}
