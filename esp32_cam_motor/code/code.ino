#include "secrets.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#define CAMERA_MODEL_WROVER_KIT
#include "camera_pins.h"

#define FLASH_PIN 2
#define STEPPER_PIN_1 13
#define STEPPER_PIN_2 12
#define STEPPER_PIN_3 14
#define STEPPER_PIN_4 27

const char* ssid = NETWORK_NAME; // Your wifi name like "myWifiNetwork"
const char* password = PASSWORD; // Your password to the wifi network like "password123"
const char* websocket_server_host = "192.168.0.150";
const uint16_t websocket_server_port = 8887;

int flashlight = 0;
int steps = 40;

using namespace websockets;
WebsocketsClient client;
camera_config_t config;

void onEventsCallback(WebsocketsEvent event, String data) {
  if(event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("Connection Opened");
  } else if(event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Connection Closed");
    ESP.restart();
  }
}

void onMessageCallback(WebsocketsMessage message) {
  String data = message.data();
  int index = data.indexOf("=");
  if(index != -1) {
    String key = data.substring(0, index);
    String value = data.substring(index + 1);
    
    if(key == "ON_BOARD_LED_1") {
      if(value.toInt() == 1) {
        flashlight = 1;
        digitalWrite(FLASH_PIN, HIGH);
      } else {
        flashlight = 0;
        digitalWrite(FLASH_PIN, LOW);
      }
    } else if(key == "MOTOR_FORWARD") {
      forward();
    } else if(key == "MOTOR_BACKWARD") {
      backward();
    }
    
    Serial.println("Key: " + key + " Value: " + value);
  }
}

void cam_conf_init() {
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
}

void forward() {
  for (int j = 0; j < steps; j++) {
    for (int i = 0; i < 4; i++) {
      switch(i){
        case 0:
        digitalWrite(STEPPER_PIN_1, HIGH);
        digitalWrite(STEPPER_PIN_2, LOW);
        digitalWrite(STEPPER_PIN_3, LOW);
        digitalWrite(STEPPER_PIN_4, LOW);
        break;
        case 1:
        digitalWrite(STEPPER_PIN_1, LOW);
        digitalWrite(STEPPER_PIN_2, HIGH);
        digitalWrite(STEPPER_PIN_3, LOW);
        digitalWrite(STEPPER_PIN_4, LOW);
        break;
        case 2:
        digitalWrite(STEPPER_PIN_1, LOW);
        digitalWrite(STEPPER_PIN_2, LOW);
        digitalWrite(STEPPER_PIN_3, HIGH);
        digitalWrite(STEPPER_PIN_4, LOW);
        break;
        case 3:
        digitalWrite(STEPPER_PIN_1, LOW);
        digitalWrite(STEPPER_PIN_2, LOW);
        digitalWrite(STEPPER_PIN_3, LOW);
        digitalWrite(STEPPER_PIN_4, HIGH);
        break;
      }
      delay(2);
    }
  }
}

void backward() 
{
  for (int j = 0; j < steps; j++) {
    for (int i = 0; i < 4; i++) {
      switch(i){
        case 0:
        digitalWrite(STEPPER_PIN_1, LOW);
        digitalWrite(STEPPER_PIN_2, LOW);
        digitalWrite(STEPPER_PIN_3, LOW);
        digitalWrite(STEPPER_PIN_4, HIGH);
        break;
        case 1:
        digitalWrite(STEPPER_PIN_1, LOW);
        digitalWrite(STEPPER_PIN_2, LOW);
        digitalWrite(STEPPER_PIN_3, HIGH);
        digitalWrite(STEPPER_PIN_4, LOW);
        break;
        case 2:
        digitalWrite(STEPPER_PIN_1, LOW);
        digitalWrite(STEPPER_PIN_2, HIGH);
        digitalWrite(STEPPER_PIN_3, LOW);
        digitalWrite(STEPPER_PIN_4, LOW);
        break;
        case 3:
        digitalWrite(STEPPER_PIN_1, HIGH);
        digitalWrite(STEPPER_PIN_2, LOW);
        digitalWrite(STEPPER_PIN_3, LOW);
        digitalWrite(STEPPER_PIN_4, LOW);
      }
      delay(2);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // camera init
  cam_conf_init();
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) { return; }

  sensor_t * s = esp_camera_sensor_get();

  s->set_contrast(s, 0);   
  s->set_raw_gma(s, 1);
  s->set_vflip(s, 1);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  pinMode(FLASH_PIN, OUTPUT);
  pinMode(STEPPER_PIN_1, OUTPUT);
  pinMode(STEPPER_PIN_2, OUTPUT);
  pinMode(STEPPER_PIN_3, OUTPUT);
  pinMode(STEPPER_PIN_4, OUTPUT);

  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  while(!client.connect(websocket_server_host, websocket_server_port, "/")) { delay(500); }
}

void loop() {
  client.poll();
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb)
  {
    esp_camera_fb_return(fb);
    return;
  }

  client.sendBinary((const char*) fb->buf, fb->len);
  esp_camera_fb_return(fb);

  String output = "ON_BOARD_LED_1=" + String(flashlight);
  Serial.println(output);

  client.send(output);
}
