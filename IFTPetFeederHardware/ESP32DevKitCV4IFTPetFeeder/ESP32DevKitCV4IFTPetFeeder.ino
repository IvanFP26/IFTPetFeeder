#include <ESP32Servo.h> 
#include <NewPing.h>
#include "time.h"
#include <ArduinoJson.h>
#include <HX711_ADC.h>

#include <Firebase_ESP_Client.h>
#include <WiFi.h>

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

String WIFI_SSID;
String WIFI_PASSWORD;
#define API_KEY "AIzaSyA6R1rzEfKVWiNFu1sTIbMHkH7zQ3EYgBk"
#define DATABASE_URL "https://iftpetfeedercoba-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define STORAGE_BUCKET_ID "iftpetfeedercoba.appspot.com"
#define USER_EMAIL "petfeederift@gmail.com"
#define USER_PASSWORD "123456"

FirebaseData fbdt;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

bool receivedSSID = false;
bool receivedPASS = false;

const int MAX_TASKS = 10; 

struct Task {
  int Action;
  String Day;
  String Food;
  int Gram;
  String Time;
};

Task tasks[MAX_TASKS];

String sFoodPositionData;
String mFoodPositionData;
JsonObject schedule;
int mCheckValue;
int mGramValue;
int GramValue;
bool success;
String firebaseJsonString;
DeserializationError error;
int StapleValue = 0;
int SnackValue = 0;
String DateFB;
int StapleFeed = 0;
int SnackFeed = 0;
DynamicJsonDocument doc(512);

const char* ntpServer = "asia.pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;
String DateMonthYearNTP;
String HourMinuteNTP;
String DateNTP;
String timeWeekDay;
const char* IFT = "/IFTPetFeeder1";
const char* Token = "TW01";

Servo servo180;
Servo servo360staple;
Servo servo360snack;

const int HX711_dout = 13;
const int HX711_sck = 15;

HX711_ADC LoadCell(HX711_dout, HX711_sck);

int buttonPin1 = 35;
int buttonPin2 = 21;
int buttonPin3 = 34;

int LedRed = 14;
int LedGreen = 27;

int trigPin = 19;
int echoPin = 18;
int trigPin2 = 22;
int echoPin2 = 23;
NewPing Distance1(trigPin, echoPin);
NewPing Distance2(trigPin2, echoPin2);

int buttonState1 = 0;
int buttonState2 = 0;
int buttonState3 = 0;

int lastButtonState1 = 0;
int lastButtonState2 = 0;
int lastButtonState3 = 0;

int staplestatus = 0;
int snackstatus = 0;

int servo180position = 0;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;
unsigned long previousMillis4 = 0;

void setup() {
  Serial.begin(115200);


  servo180.attach(2);
  servo360staple.attach(4);
  servo360snack.attach(26);

  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);

  pinMode(LedRed, OUTPUT);
  pinMode(LedGreen, OUTPUT);
  
  InitLoadcell();

  ConnectfromBT();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  GetFirebase();
  StapleSnackInfo();
  StatusCheck();
}

void loop() {
  unsigned long currentMillis = millis();
  printLocalTime();

  if (currentMillis - previousMillis1 >= 10000){
    previousMillis1 = currentMillis;
    GetFirebase();
    StapleSnackInfo();
    ManualFeeding();
    StatusCheck();
  }

  if (currentMillis - previousMillis2 >= 100){
    previousMillis2 = currentMillis;
    ButtonState();
    //Button pertama jika ditekan
    if (buttonState1 != lastButtonState1 && buttonState1 == LOW) {
      //Jika isi dari penampungan makanan pokok 0 maka akan diberi peringatan, jika tidak maka akan masuk ke eksekusi "else"
      if(staplestatus == 0){
        Status();
        LedGone();
      }
      else{
        //Mengatur variabel posisi makanan menjadi "Staple" dan mengeluarkan makanan sebanyak 10 gram
        mFoodPositionData = "Staple";
        mGramValue = 10;
        Feeding();
      }
    }

    //Button kedua jika ditekan
    if (buttonState2 != lastButtonState2 && buttonState2 == LOW) {
      //Jika posisi servo berada di posisi 180 maka akan berpindah ke posisi 0 begitupun sebaliknya
      if (servo180position == 180) {
        Status();
        ServoPSTNStaple();
      }
      else {
        Status();
        ServoPSTNSnack();
      }
    }
    
    //Button ketiga jika ditekan
    if (buttonState3 != lastButtonState3 && buttonState3 == LOW){
      //Jika isi dari penampungan camilan 0 maka akan diberi peringatan, jika tidak maka akan masuk ke eksekusi "else"
      if(snackstatus == 0){
        Status();
        LedGone();
      }
      //Mengatur variabel posisi makanan menjadi "Snack" dan mengeluarkan makanan sebanyak 10 gram
      else{
        mFoodPositionData = "Snack";
        mGramValue = 10;
        Feeding();
      }
    }
    LastButtonState(); 
  }

  if (currentMillis - previousMillis3 >= 60000){
    previousMillis3 = currentMillis;
    Scheduler();
  }

  if (currentMillis - previousMillis4 >= 5000){
    previousMillis4 = currentMillis;
  }

  if (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n');
    message.trim();

    if (message.startsWith(Token)) {
      Serial.println("WifiSetting");
      ConnectfromBT();
    }
  }
}

void ConnectfromBTchanges(){
  if (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n');
    message.trim();
    if (message.startsWith(Token)) {
      Serial.println("WifiSetting");
      ConnectfromBT();
    }
  }
}

void ConnectfromBT(){
  while (!(receivedSSID && receivedPASS)) {
    if (Serial.available() > 0) {
      String message = Serial.readStringUntil('\n');
      // Memisahkan SSID dan password dari pesan yang diterima
      if (message.startsWith("SSID:")) {
        WIFI_SSID = message.substring(5); // Mengambil bagian dari pesan setelah "SSID:"
        receivedSSID = true;
        Serial.println("SSID received: " + WIFI_SSID);
      } else if (message.startsWith("PASS:") && receivedSSID) {
        WIFI_PASSWORD = message.substring(5); // Mengambil bagian dari pesan setelah "PASS:"
        receivedPASS = true;
        Serial.println("Password received: " + WIFI_PASSWORD);
      }
    }
  }
  Serial.println("Both SSID and Password received, continuing with next steps...");
  receivedSSID = false;
  receivedPASS = false;
  Connect();
}

void Connect(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
    ConnectfromBTchanges();
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  //Melakukan langkah untuk terkoneksi ke database dengan memasukkan URL Database dan API yang sudah di definisikan
  config.database_url = DATABASE_URL;
  config.api_key = API_KEY;

  //Melakukan pengaturan email dan password untuk autentikasi ke database Firebase
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  //Melakukan autentikasi menggunakan data yang sudah didefinisikan sebelumnya
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  //Melakukan tokenisasi terhadap Firebase supaya bisa berinteraksi dengan database Firebase
  config.token_status_callback = tokenStatusCallback;

  //Mengatur ukuran buffer
  fbdt.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  //Melakukan koneksi ke firebase dengan konfigurasi dan autentikasi yang sudah diatur
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.RTDB.setString(&fbdt, String(IFT) + "/Token", Token);
}

void Status(){
  Serial.println("----Status----");
}

void StatusCheck(){
  Serial.println("----Status----");
  Firebase.RTDB.getString(&fbdt, String(IFT) + "/IFTPetFeeder/FoodPosition");
  sFoodPositionData = fbdt.stringData();
  if (sFoodPositionData == "Staple") {
    ServoPSTNStaple();
  } 
  else if (sFoodPositionData == "Snack") {
    ServoPSTNSnack();
  }
}

void GetFirebase(){
  if (Firebase.ready() && signupOK) {
    Serial.println("Status Perangkat : Terhubung dengan Wi-Fi dan Firebase");
    success = Firebase.RTDB.getJSON(&fbdt, String(IFT) + "/IFTPetFeeder");
    if (!success) {
      Serial.println("Failed to get JSON from Firebase: " + fbdt.errorReason());
      return;
    }
    
    firebaseJsonString = fbdt.to<FirebaseJson>().raw();
    deserializeJson(doc, firebaseJsonString);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }
    schedule = doc["Schedule"];
    if (schedule.isNull()) {
      Serial.println("Schedule node is null.");
      return;
    }
  }
  else {
    Serial.print("Status Perangkat: ");
    Serial.println(fbdt.errorReason());
  }
}

void InitLoadcell(){
  LoadCell.begin();
  float calibrationValue = 1171.40;
  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);
  LoadCell.setCalFactor(calibrationValue);
}

//Fungsi untuk menjalankan servo 360 makanan pokok
void ServoCStaple(){
  //Servo berputar searah jarum jam 0,15 dan berputar berwalanan jarum jam 0,1 detik dan seterusnya
  servo360staple.write(97);
  delay(150);
  servo360staple.write(83);
  delay(100);
  servo360staple.write(90);
  StapleFeed = 1;
}

//Fungsi untuk memberhentikan servo 360 makanan pokok
void StopServoCStaple(){
  servo360staple.write(90);
}

//Fungsi untuk menjalankan servo 360 camilan
void ServoCSnack() {
  //Servo berputar searah jarum jam 0,15 dan berputar berwalanan jarum jam 0,1 detik dan seterusnya
  servo360snack.write(97);
  delay(150);
  servo360snack.write(83);
  delay(100);
  servo360snack.write(90);
  SnackFeed = 1;
}

//Fungsi untuk memberhentikan servo 360 camilan
void StopServoCSnack() {
  servo360snack.write(90);
  SnackFeed = 1;
}

//Fungsi mengatur posisi servo ke Staple/Makanan Pokok
void ServoPSTNStaple(){
  //Memindahkan posisi servo ke 0 derajat
  Serial.println("Position: Staple");
  for (int i = servo180position; i >= 0; i--) {
    servo180.write(i);
    delay(15);
  }
  servo180position = 0;
  //Mengatur indikator LED menjadi warna merah menyala
  digitalWrite(LedRed, HIGH);
  digitalWrite(LedGreen, LOW);
  //Mengirim informasi posisi servo ke database
  Firebase.RTDB.setString(&fbdt, String(IFT) + "/IFTPetFeeder/FoodPosition", "Staple");
  sFoodPositionData = fbdt.stringData();
}

//Fungsi mengatur posisi servo ke Snack/Camilan
void ServoPSTNSnack(){
  //Memindahkan posisi servo ke 180 derajat
  Serial.println("Position: Snack");
  for (int i = servo180position; i <= 180; i++) {
    servo180.write(i);
    delay(15);
  }
  servo180position = 180;
  //Mengatur indikator LED menjadi warna hijau menyala
  digitalWrite(LedRed, LOW);
  digitalWrite(LedGreen, HIGH);
  //Mengirim informasi posisi servo ke database
  Firebase.RTDB.setString(&fbdt, String(IFT) + "/IFTPetFeeder/FoodPosition", "Snack");
  sFoodPositionData = fbdt.stringData();
}


void StapleSnackInfo() {
  //Mengukur jarak isi dari penampungan makanan pokok dan camilan
  unsigned int distanceStaple = Distance1.ping_cm();
  unsigned int distanceSnack = Distance2.ping_cm();

  //Melakukan pengukuran yang didapat dari penampungan makanan pokok
  staplestatus = max(0, static_cast<int>(round((1 - (float)distanceStaple / 22) * 100)));
  Serial.print("Staple Storage: ");
  Serial.print(staplestatus);
  Serial.println("%");

  //Melakukan pengukuran yang didapat dari penampungan camilan
  snackstatus = max(0, static_cast<int>(round((1 - (float)distanceSnack / 22) * 100)));
  Serial.print("Snack Storage: ");
  Serial.print(snackstatus);
  Serial.println("%");
  
  //Mengirimkan hasil sensor HC-SR04 ke Database
  Firebase.RTDB.setInt(&fbdt, String(IFT) + "/IFTPetFeeder/Storage/Snack", snackstatus);
  Firebase.RTDB.setInt(&fbdt, String(IFT) + "/IFTPetFeeder/Storage/Staple", staplestatus);
}

void LedGone(){
  if (staplestatus == 0){
    Serial.println("Staple Food Empty");
  }
  else if (snackstatus == 0){
    Serial.println("Snack Food Empty");
  }
  for (int i = 0; i < 3; i++){
    digitalWrite(LedRed, HIGH);
    digitalWrite(LedGreen, HIGH);
    delay(500);
    digitalWrite(LedRed, LOW);
    digitalWrite(LedGreen, LOW);
    delay(500);
  }
  if (servo180position == 0){
    digitalWrite(LedRed, HIGH);
    digitalWrite(LedGreen, LOW);
  }
  else if (servo180position == 180){
    digitalWrite(LedRed, LOW);
    digitalWrite(LedGreen, HIGH);
  }
}

void ManualFeeding(){
  mCheckValue = doc["Feeding"]["Check"];
  mFoodPositionData = doc["Feeding"]["FoodPosition"].as<String>();
  mGramValue = doc["Feeding"]["Gram"];
  if (mCheckValue == 1){
    Feeding();
    mCheckValue = 0;
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/IFTPetFeeder/Feeding/Check", mCheckValue);
  }
}

void Feeding(){
  //Jika kondisi Staple atau makanan pokok maka akan dieksekusi
  if (mFoodPositionData == "Staple") {
    //Jika isi dari penampungan makanan pokok 0 maka akan diberi peringatan, jika tidak maka akan masuk ke eksekusi "else"
    if(staplestatus == 0){
      Status();
      LedGone();
    }
    else{
      Status();
      ServoPSTNStaple();
      Serial.print(mGramValue);
      Serial.println(" Gram");
      //Melakukan perhitungan isi dari berat makanan yang ada selama 3 detik
      unsigned long startTime = millis();
      while (millis() - startTime < 3000) {
        LoadCell.update();
        float CurrentLoad = LoadCell.getData();
        Serial.print("Load_cell output val: ");
        Serial.println(CurrentLoad);
      }
      //Melakukan pengeluaran makanan sesuai dengan gram yang dibutuhkan dengan rumus berat sekarang ditambah berat yang diinginkan
      Serial.println("Dispensing Staple Foods");
      float TargetLoad = LoadCell.getData() + mGramValue;
      while ((LoadCell.getData() < TargetLoad)) {
        ServoCStaple();
        unsigned long startTime = millis();
        while (millis() - startTime < 500) {
          LoadCell.update();
          float CurrentLoad = LoadCell.getData();
          Serial.print("Current Load");
          Serial.println(CurrentLoad);
          Serial.println(" Gram");
        }
      } 
      StopServoCStaple();
      //Setiap makanan yang sudah dikeluarkan akan masuk ke dalam history yang ada di database
      HistoryAllTime();
    }
  } 
  //Jika kondisi Snack atau camilan maka akan dieksekusi
  if (mFoodPositionData == "Snack") {
    //Jika isi dari penampungan camilan 0 maka akan diberi peringatan, jika tidak maka akan masuk ke eksekusi "else"
    if(snackstatus == 0){
      Status();
      LedGone();
    }
    else{
      Status();
      ServoPSTNSnack();
      Serial.print(mGramValue);
      Serial.println(" Gram");
      //Melakukan perhitungan isi dari berat makanan yang ada selama 3 detik
      unsigned long startTime = millis();
      while (millis() - startTime < 3000) { // Run for 3 seconds
        // check for new data/start next conversion:
        LoadCell.update();
        float CurrentLoad = LoadCell.getData();
        Serial.print("Load_cell output val: ");
        Serial.println(CurrentLoad);
      }
      //Melakukan pengeluaran makanan sesuai dengan gram yang dibutuhkan dengan rumus berat sekarang ditambah berat yang diinginkan
      Serial.println("Dispensing Snack Foods");
      float TargetLoad = LoadCell.getData() + mGramValue;
      while ((LoadCell.getData() < TargetLoad)) {
        ServoCSnack();
        unsigned long startTime = millis();
        while (millis() - startTime < 500) {
          LoadCell.update();
          float CurrentLoad = LoadCell.getData();
          Serial.print("Current Load");
          Serial.println(CurrentLoad);
          Serial.println(" Gram");
        }
      }  
      StopServoCSnack();
      //Setiap makanan yang sudah dikeluarkan akan masuk ke dalam history yang ada di database
      HistoryAllTime();
    }
  }
}

void ButtonState(){
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);
  buttonState3 = digitalRead(buttonPin3);
}

void LastButtonState(){
  lastButtonState1 = buttonState1;
  lastButtonState2 = buttonState2;
  lastButtonState3 = buttonState3;
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  char temp[20];
  strftime(temp, 20, "%m-%d-%Y", &timeinfo);
  DateMonthYearNTP = temp;

  strftime(temp, 20, "%H:%M", &timeinfo);
  HourMinuteNTP = temp;

  strftime(temp, 20, "%d", &timeinfo);
  DateNTP = temp;

  strftime(temp, 20, "%A", &timeinfo);
  timeWeekDay = temp;
}


void HistoryAllTime(){
  FirebaseJson json;

  json.add("Time", DateMonthYearNTP);
  json.add("Food Position", sFoodPositionData);
  json.add("Hour", HourMinuteNTP);
  json.add("Gram", mGramValue);

  Serial.printf("Push json... %s\n", Firebase.RTDB.pushJSON(&fbdt, String(IFT) + "/History/AllTime", &json) ? "ok" : fbdt.errorReason().c_str());
  HistoryPerDay();
}

void HistoryPerDay() {
  Firebase.RTDB.getInt(&fbdt, String(IFT) + "/History/PerDay/Staple");
  StapleValue = fbdt.intData(); 
  Firebase.RTDB.getInt(&fbdt, String(IFT) + "/History/PerDay/Snack");
  SnackValue = fbdt.intData(); 
  Firebase.RTDB.getInt(&fbdt, String(IFT) + "/History/PerDay/Gram");
  GramValue = fbdt.intData();
  Firebase.RTDB.getString(&fbdt, String(IFT) + "/History/PerDay/Date");
  DateFB = fbdt.stringData(); 
  Serial.print("DateFB: ");
  Serial.println(DateFB);
  Firebase.RTDB.setString(&fbdt, String(IFT) + "/History/PerDay/Date", DateMonthYearNTP);
  Serial.print("DateMonthYearNTP: ");
  Serial.println(DateMonthYearNTP);
  if (DateMonthYearNTP == DateFB) {
    StapleValue += StapleFeed;
    SnackValue += SnackFeed;
    GramValue += mGramValue;
    StapleFeed = 0;
    SnackFeed = 0;
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/History/PerDay/Staple", StapleValue);
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/History/PerDay/Snack", SnackValue);
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/History/PerDay/Gram", GramValue);
  } 
  else {
    StapleValue = 0;
    SnackValue = 0;
    GramValue = 0;
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/History/PerDay/Staple", StapleValue);
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/History/PerDay/Snack", SnackValue);
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/History/PerDay/Gram", GramValue);
    StapleValue += StapleFeed;
    SnackValue += SnackFeed;
    GramValue += mGramValue;
    StapleFeed = 0;
    SnackFeed = 0;
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/History/PerDay/Staple", StapleValue);
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/History/PerDay/Snack", SnackValue);
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/History/PerDay/Gram", GramValue);
  }
  Firebase.RTDB.setString(&fbdt, String(IFT) + "/History/PerDay/LastAte", HourMinuteNTP);
}


void Scheduler(){
  //Variabel untuk menghitung jumlah tugas yang telah diproses
  Serial.println("----Scheduler----");
  int taskCount = 0;

  //Memproses setiap tugas dalam objek "Schedule"
  for (JsonPair task : schedule) {
    //Memeriksa apakah sudah mencapai batas maksimum tugas
    if (taskCount >= MAX_TASKS) {
      Serial.println("Max tasks reached.");
      break;
    }

    //Memeriksa apakah objek tugas valid
    if (task.value().isNull()) {
      Serial.println("Task object is null.");
      continue;
    }

    //Mendapatkan nilai dari masing-masing properti tugas
    tasks[taskCount].Action = task.value()["Action"];
    tasks[taskCount].Day = task.value()["Day"].as<String>();
    tasks[taskCount].Food = task.value()["Food"].as<String>();
    tasks[taskCount].Gram = task.value()["Gram"];
    tasks[taskCount].Time = task.value()["Time"].as<String>();
    Serial.print("Action: ");
    Serial.print(tasks[taskCount].Action);
    Serial.print(", Day: ");
    Serial.print(tasks[taskCount].Day);
    Serial.print(", Food: ");
    Serial.print(tasks[taskCount].Food);
    Serial.print(", Gram: ");
    Serial.print(tasks[taskCount].Gram);
    Serial.print(", Time: ");
    Serial.print(tasks[taskCount].Time);
    Serial.println();
    if (tasks[taskCount].Action == 1){
      if (tasks[taskCount].Day == timeWeekDay || tasks[taskCount].Day == "Everyday") {
        if (tasks[taskCount].Time == HourMinuteNTP){
          mFoodPositionData = tasks[taskCount].Food;
          mGramValue = tasks[taskCount].Gram;
          Feeding();
        }
      }
    }
    taskCount++;
  }
}