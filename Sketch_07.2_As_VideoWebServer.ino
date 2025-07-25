/**********************************************************************
  Filename    : Video Web Server
  Description : The camera images captured by the ESP32S3 are displayed on the web page.
  Auther      : www.freenove.com
  Modification: 2022/11/01
**********************************************************************/
#include "esp_camera.h"
#include <WiFi.h>
#include "sd_read_write.h"

#include "cpu-test.h" 
#include "CMXsafe/settings.h"
#include "CMXsafe/cmxsafe.h"
#include "CMXsafe/fw_ports.h"
#include "CMXsafe/sm.h"
#include "CMXsafe/ota.h"
#include "CMXsafe/remoteAttest.h"
#include "fw_ports.h"


// Select camera model
#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM

#include "camera_pins.h"

const char* ssid     = "********";   //input your wifi name
const char* password = "********";   //input your wifi passwords

void cameraInit(void);
void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  cameraInit();
  sdmmcInit();
  removeDir(SD_MMC, "/video");
  createDir(SD_MMC, "/video");
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();
  xTaskCreatePinnedToCore(camera_task, "Camera task", 10500, NULL, 5, NULL, 1);
  TaskHandle_t Task1Handle = NULL; 
  xTaskCreatePinnedToCore(setup_ota, "setup ota", 16384, NULL, 0, NULL, 1);
  xTaskCreatePinnedToCore(ssh_port_forwarding_task, "SSH Port Forwarding Task", 10500, &streamPort, 5, &Task1Handle, 1);
  xTaskCreatePinnedToCore(ssh_port_forwarding_task, "SSH Port Forwarding Task3", 10500, &otaPort, 5, NULL, 1);
  xTaskCreatePinnedToCore(ssh_port_forwarding_task, "SSH Port Forwarding Task4", 10500, &attestationPort, 5, NULL, 1);
  setup_sm();
  xTaskCreatePinnedToCore(idleTask,"Idle Task",1024, NULL,4,NULL,1);

  int paramAttest = 5;
  remoteAttest::setup_remoteAttest(&paramAttest);

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

//Used to monitor RAM and ROM usage
uint32_t totalFlash;
uint32_t usedFlash;
uint32_t freeFlash;

void loop() {
  // put your main code here, to run repeatedly:
  delay(10000);
        if (psramFound()) {
        Serial.println("PSRAM is available!");
        Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    } else {
        Serial.println("PSRAM is not available on this board.");
    }

    // Display ROM size information
    Serial.printf("Total Flash Memory: %d bytes\n", ESP.getFlashChipSize());
    Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Cores: %d\n", ESP.getChipCores());
    Serial.printf("Total Heap: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Min Free Heap: %d bytes\n", ESP.getMinFreeHeap());
    Serial.printf("Max Alloc Heap: %d bytes\n", ESP.getMaxAllocHeap());

    totalFlash = ESP.getFlashChipSize();
    usedFlash = ESP.getSketchSize();
    freeFlash = totalFlash - usedFlash;

    Serial.printf("Total Flash Memory: %u bytes\n", totalFlash);
    Serial.printf("Used Flash Memory (ROM + Code): %u bytes\n", usedFlash);
    Serial.printf("Free Flash Memory: %u bytes\n", freeFlash);

    measure_idle_cpu();
 
}

void cameraInit(void){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_CIF;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  // for larger pre-allocated frame buffer.
  if(psramFound()){
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    // Limit the frame size when PSRAM is not available
    config.frame_size = FRAMESIZE_CIF;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  s->set_vflip(s, 1); // flip it back
  s->set_brightness(s, 1); // up the brightness just a bit
  s->set_saturation(s, 0); // lower the saturation
}

