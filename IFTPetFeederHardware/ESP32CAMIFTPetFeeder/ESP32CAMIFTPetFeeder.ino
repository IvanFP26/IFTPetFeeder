#include "WiFi.h"
#include "nvs_flash.h"
#include "Arduino.h"
#include "WiFi.h"
#include "BluetoothSerial.h"
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <LittleFS.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define API_KEY "AIzaSyA6R1rzEfKVWiNFu1sTIbMHkH7zQ3EYgBk"
#define USER_EMAIL "petfeederift@gmail.com"
#define USER_PASSWORD "123456"
#define STORAGE_BUCKET_ID "iftpetfeedercoba.appspot.com"
#define FILE_PHOTO_PATH "/photo.jpg"
#define BUCKET_PHOTO "/data/photo.jpg"

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define pirPin 13

int MAX_RETRY_COUNT = 3;

int val = 0;
bool motionState = false;
boolean takeNewPhoto = true;
unsigned long lastPhotoTime = 0;
String downloadURL, tokens;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;
bool taskCompleted = false;
const char* IFT = "/IFTPetFeeder1";
const char* Token = "TW01";
BluetoothSerial SerialBT;
bool signupOK = false;

String inputBuffer = "";
String WIFI_SSID = "";
String WIFI_PASSWORD = "";
bool receivedSSID = false;
bool receivedPASS = false;
bool setupWiFi = false;


char ssid_buffer[32] = "";
char password_buffer[64] = "";

void setup() {
  Serial.begin(115200);
  SerialBT.begin("IFTPETFEEDER");
  Serial.println("The device started, now you can pair it with bluetooth!");
  pinMode(pirPin, INPUT);
  connect();
  initLittleFS();
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  initCamera();
  configF.database_url = "https://iftpetfeedercoba-default-rtdb.asia-southeast1.firebasedatabase.app/";
  configF.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  if (Firebase.signUp(&configF, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", configF.signer.signupError.message.c_str());
  }

  configF.token_status_callback = tokenStatusCallback;
  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  checkFirebaseStatus();
  BTConnectWIFI();

  val = digitalRead(pirPin);
  //Jika ada pergerakan di sekitar sensor PIR maka perintah akan dieksekusi
  if (val == HIGH){
    Serial.println("Motion detected!");
    SerialBT.println("Motion detected!");
    delay(3000);
    //Mengambil foto dari ESP32CAM
    capturePhotoSaveLittleFS();
    //Mengupload foto ke Firebase
    UploadPhototoFirebase();
    //Menunggu 10 detik untuk sensor kembali melakukan deteksi
    unsigned long startTime = millis();
    Serial.println("Waiting for 10 seconds");
    SerialBT.println("Waiting for 10 seconds");
    while (millis() - startTime < 10000) { // Loop selama 10 detik
      delay(1); // Tunggu 1 ms untuk menghindari memblokir loop sepenuhnya
      BTConnectWIFI();
    }
  }
}

void connect(){
  // Initialize NVS
  if (nvs_flash_init() != ESP_OK) {
    Serial.println("Error initializing NVS");
    return;
  }

  nvs_handle_t nvs_handle;
  if (nvs_open("storage", NVS_READONLY, &nvs_handle) == ESP_OK) {
    size_t ssid_len = sizeof(ssid_buffer);
    size_t password_len = sizeof(password_buffer);
    nvs_get_str(nvs_handle, "ssid", ssid_buffer, &ssid_len);
    nvs_get_str(nvs_handle, "password", password_buffer, &password_len);
    nvs_close(nvs_handle);
    String ssid_message = "SSID:";
    ssid_message += ssid_buffer;
    ssid_message += "\n";

    String password_message = "PASS:";
    password_message += password_buffer;
    password_message += "\n";

    delay(1000);
    Serial.write(ssid_message.c_str(), ssid_message.length());
    delay(1000);
    Serial.write(password_message.c_str(), password_message.length());

    // Connect to Wi-Fi
    Serial.println("Attempting to connect to Wi-Fi...");
    WiFi.begin(ssid_buffer, password_buffer);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      BTConnectWIFI();
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Error opening NVS");
  }
}

void BTConnectWIFI(){
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }

  if (SerialBT.available()) {
    char incomingChar = (char)SerialBT.read();
    inputBuffer += incomingChar;
    if (inputBuffer.endsWith(Token)) {
      Serial.println("Detected 'IFT'. Please enter SSID and PASSWORD.");
      SerialBT.println("Detected 'IFT'. Please enter SSID and PASSWORD.");
      inputBuffer = "";
      String Token2 = Token;
      Token2 += "\n";

      Serial.write(Token2.c_str(), Token2.length());
      while (!(receivedSSID && receivedPASS)) {
        if (SerialBT.available() > 0) {
          String message = SerialBT.readStringUntil('\n');
          message.trim();
          if (message.startsWith("SSID:")) {
            WIFI_SSID = message.substring(5);
            WIFI_SSID.trim();
            receivedSSID = true;
            Serial.println("SSID received: " + WIFI_SSID);
          } else if (message.startsWith("PASS:") && receivedSSID) {
            WIFI_PASSWORD = message.substring(5);
            WIFI_PASSWORD.trim();
            receivedPASS = true;
            Serial.println("Password received: " + WIFI_PASSWORD);
          }
        }
      }
      Serial.println("Both SSID and Password received, continuing with next steps...");
      saveCredentialsToNVS(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
      connect();
      receivedSSID = false;
      receivedPASS = false;
    }
    if (inputBuffer.length() > 20) {
      inputBuffer = "";
    }
  }
}

void checkFirebaseStatus() {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready, reconnecting...");
    reconnectFirebase();
  }
}

void reconnectFirebase() {
  Serial.println("Reconnecting to Firebase...");
  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);
}

//Fungsi untuk mengupload photo yang telah diambil ke firebase
void UploadPhototoFirebase(){
  //Jika status wifi masih tersambung maka program akan dieksekusi
  if (WiFi.status() == WL_CONNECTED) {
    //Jika masih terhubung ke firebase maka program akan dieksekusi
    if (Firebase.ready() && signupOK){
      //Mengupload photo ke firebase dari LittleFS memori internal
      Serial.print("Uploading picture... ");
      if (!Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, FILE_PHOTO_PATH, mem_storage_type_flash, BUCKET_PHOTO, "image/jpeg", fcsUploadCallback)) {
        Serial.println("Upload failed");
        Serial.println(fbdo.errorReason());
      }
    } else {
      Serial.println("Firebase not ready or sign up not OK.");
      Serial.println(fbdo.errorReason());
    }
  } else {
    Serial.println("WiFi not connected. Cannot upload photo.");
  }
}

//Fungsi untuk menyimpan SSID dan Password wifi ke NVS
void saveCredentialsToNVS(const char *ssid, const char *password) {
  nvs_handle_t nvs_handle;
  //Membuka NVS lalu menghapus isi didalam NVS, lalu menyimpan SSID dan Passwod
  if (nvs_open("storage", NVS_READWRITE, &nvs_handle) == ESP_OK) {
    //Menghapus isi di dalam NVS
    nvs_erase_key(nvs_handle, "ssid");
    nvs_erase_key(nvs_handle, "password");
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
  } else {
    Serial.println("Error opening NVS");
    return;
  }

  //Membuka NVS lalu menyimpan SSID dan Password yang baru
  if (nvs_open("storage", NVS_READWRITE, &nvs_handle) == ESP_OK) {
    //Mengisi nilai baru ke NVS
    nvs_set_str(nvs_handle, "ssid", ssid);
    nvs_set_str(nvs_handle, "password", password);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    Serial.println("New SSID and Password saved to NVS");
  } else {
    Serial.println("Error opening NVS");
  }
}

//Fungsi untuk mengambil photo lalu menyimpan di LittleFS memori internal
void capturePhotoSaveLittleFS( void ) {
  //Proses pengambilan gambar
  camera_fb_t* fb = NULL;
  for (int i = 0; i < 4; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }
  fb = esp_camera_fb_get();
  //Jika proses pengambilan gambar gagal, maka esp32cam akan memulai ulang
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }  
  //Menyimpan gambar ke LittleFS penyimpanan internal
  Serial.printf("Picture file name: %s\n", FILE_PHOTO_PATH);
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  }
  else {
    file.write(fb->buf, fb->len);
    Serial.print("The picture has been saved in ");
    Serial.print(FILE_PHOTO_PATH);
    Serial.print(" - Size: ");
    Serial.print(fb->len);
    Serial.println(" bytes");
  }
  file.close();
  esp_camera_fb_return(fb);
}

void initLittleFS(){
  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("LittleFS mounted successfully");
  }
}

void initCamera(){
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  //mengatur kualitas kamera
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 20;
    config.fb_count = 1;
  }

  //Jika camera pada ESP32CAM gagal di inisialisasi maka ESP32CAM akan melakukan restart
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  } 
}

void fcsUploadCallback(FCS_UploadStatusInfo info){
  if (info.status == firebase_fcs_upload_status_init){
      Serial.printf("Uploading file %s (%d) to %s\n", info.localFileName.c_str(), info.fileSize, info.remoteFileName.c_str());
      SerialBT.printf("Uploading file %s (%d) to %s\n", info.localFileName.c_str(), info.fileSize, info.remoteFileName.c_str());
  }
  else if (info.status == firebase_fcs_upload_status_upload)
  {
      Serial.printf("Uploaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
      SerialBT.printf("Uploaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
  }
  else if (info.status == firebase_fcs_upload_status_complete)
  {
      Serial.println("Upload completed\n");
      SerialBT.println("Upload completed\n");
      FileMetaInfo meta = fbdo.metaData();
      Serial.printf("Name: %s\nBucket: %s\ncontentType: %s\nSize: %d\nGeneration: %lu\nMetageneration: %lu\nETag: %s\nCRC32: %s\nTokens: %s\nDownload URL: %s\n", meta.name.c_str(), meta.bucket.c_str(), meta.contentType.c_str(), meta.size, meta.generation, meta.metageneration, meta.etag.c_str(), meta.crc32.c_str(), meta.downloadTokens.c_str(), fbdo.downloadURL().c_str());
      SerialBT.printf("Name: %s\nBucket: %s\ncontentType: %s\nSize: %d\nGeneration: %lu\nMetageneration: %lu\nETag: %s\nCRC32: %s\nTokens: %s\nDownload URL: %s\n", meta.name.c_str(), meta.bucket.c_str(), meta.contentType.c_str(), meta.size, meta.generation, meta.metageneration, meta.etag.c_str(), meta.crc32.c_str(), meta.downloadTokens.c_str(), fbdo.downloadURL().c_str());
      downloadURL = fbdo.downloadURL().c_str();
      tokens = meta.downloadTokens.c_str();
      Firebase.RTDB.setString(&fbdo, String(IFT) + "/IFTPetFeeder/ImageCapture/Link", downloadURL);
      Firebase.RTDB.setString(&fbdo, String(IFT) + "/IFTPetFeeder/ImageCapture/Tokens", tokens);
  }
  else if (info.status == firebase_fcs_upload_status_error){
      Serial.printf("Upload failed %s\n", info.errorMsg.c_str());
      SerialBT.printf("Upload failed %s\n", info.errorMsg.c_str());
      ESP.restart();
  }
}
