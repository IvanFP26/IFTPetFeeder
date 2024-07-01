#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <ArduinoJson.h>
#include <time.h>
#include <ESP32Servo.h> 
#include <NewPing.h>
#include <HX711_ADC.h>

#define API_KEY "AIzaSyA6R1rzEfKVWiNFu1sTIbMHkH7zQ3EYgBk"
#define DATABASE_URL "https://iftpetfeedercoba-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "petfeederift@gmail.com"
#define USER_PASSWORD "123456"

FirebaseData fbdt;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

String WIFI_SSID;
String WIFI_PASSWORD;
bool receivedSSID = false;
bool receivedPASS = false;

JsonObject schedule;
String firebaseJsonString;
//DeserializationError error;
bool success;
DynamicJsonDocument doc(512);
struct Task {
  int Action;
  String Day;
  String Food;
  int Gram;
  String Time;
};
const int MAX_TASKS = 10; 
Task tasks[MAX_TASKS];

String sFoodPositionData;
String mFoodPositionData;
int mCheckValue;
int mGramValue;
int GramValue;
int StapleValue = 0;
int SnackValue = 0;
String DateFB;
int StapleFeed = 0;
int SnackFeed = 0;

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
#define servo180pin        2
#define servo360staplepin  4
#define servo360snackpin   26
int servo180position = 0;

#define HX711_dout  13
#define HX711_sck   15
HX711_ADC LoadCell(HX711_dout, HX711_sck);

#define buttonPin1  35
#define buttonPin2  21
#define buttonPin3  34

#define LedRed   14
#define LedGreen 27

int staplestatus = 0;
int snackstatus = 0;
#define trigPin   19
#define echoPin   18
#define trigPin2  22
#define echoPin2  23
NewPing Distance1(trigPin, echoPin);
NewPing Distance2(trigPin2, echoPin2);

int buttonState1 = 0;
int buttonState2 = 0;
int buttonState3 = 0;

unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;
unsigned long previousMillis4 = 0;

void setup() {
  Serial.begin(115200);
  servo180.attach(servo180pin);
  servo360staple.attach(servo360staplepin);
  servo360snack.attach(servo360snackpin);

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
    if (buttonState1 == LOW) {
      //Mengatur variabel posisi makanan menjadi "Staple" dan mengeluarkan makanan sebanyak 10 gram
      mFoodPositionData = "Staple";
      mGramValue = 10;
      Feeding();
    }

    //Button kedua jika ditekan
    if (buttonState2 == LOW) {
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
    if (buttonState3 == LOW){
      //Mengatur variabel posisi makanan menjadi "Snack" dan mengeluarkan makanan sebanyak 10 gram
      mFoodPositionData = "Snack";
      mGramValue = 10;
      Feeding();
    }
    //LastButtonState(); 
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
  //fbdt.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

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
  ////Mengambil isi node FoodPosition dari json ke variabel schedule
  sFoodPositionData = doc["FoodPosition"].as<String>();
  //sFoodPositionData = fbdt.stringData();
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
    //mengambil data dalam bentuk json dari firebase
    success = Firebase.RTDB.getJSON(&fbdt, String(IFT) + "/IFTPetFeeder");
    if (!success) {
      Serial.println("Failed to get JSON from Firebase: " + fbdt.errorReason());
      return;
    }
    firebaseJsonString = fbdt.to<FirebaseJson>().raw(); //Menyimpan data json yang sudah diambil dari firebase dalam bentuk data masih mentah
    deserializeJson(doc, firebaseJsonString); //Melakukan deserialisasi terhadap data json yang masih mentah
    schedule = doc["Schedule"]; //Mengambil isi node Schedule dari json ke variabel
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

//Fungsi untuk melakukan inisialisasi load cell
void InitLoadcell(){
  LoadCell.begin(); //Menginisialisasi load cell
  float calibrationValue = 1171.40; //Variabel untuk menyimpan nilai kalibrasi load cell
  unsigned long stabilizingtime = 2000; //Variabel untuk menyimpan nilai stabilisasi load cell
  boolean _tare = true; //Digunakan untuk mengatur nol jika kondisi load cell tidak ada beban
  LoadCell.start(stabilizingtime, _tare); //Melakukan proses pembacaan loadcell
  LoadCell.setCalFactor(calibrationValue); //Mengatur faktor kalibrasi loadcell yang sudah dilakukan sebelumnya
}

//Fungsi melakukan perhitungan berat pada Loadcell
void ReadLoadCell(){
  LoadCell.update(); //Memperbarui data dari loadcell
  float CurrentLoad = LoadCell.getData(); //Menyimpan nilai pembacaan dari loadcell
  //Menampilkan isi berat makanan pada wadah makanan
  Serial.print("Load_cell output val: ");
  Serial.println(CurrentLoad);
}

//Fungsi (1) untuk menjalankan servo 360 makanan pokok
void ServoCStaple(){
  //Servo berputar searah jarum jam selama 0,05 detik
  servo360staple.write(115);
  delay(50);
  //Servo berhenti selama 1,5 detik
  servo360staple.write(90);
  delay(1500);
  //Menyimpan nilai variabel 1 untuk digunakan di history
  StapleFeed = 1;
}

//Fungsi untuk menjalankan servo 360 makanan pokok jika terdapat kemacetan pada spiral makanan
void ServoCStapleStack(){
  //Servo berputar berlawanan jarum jam selamat 0,4 detik
  servo360staple.write(70);
  delay(400);
  //Servo berputar searah jarum jam 1 detik
  servo360staple.write(115);
  delay(1000);
  //Servo berhenti selama 1,5 detik
  servo360staple.write(90);
  delay(1500);
}

//Fungsi untuk memberhentikan servo 360 makanan pokok
void StopServoCStaple(){
  servo360staple.write(90);
}

//Fungsi untuk menjalankan servo 360 camilan
void ServoCSnack() {
  //Servo berputar searah jarum jam selama 0,05 detik
  servo360snack.write(115);
  delay(50);
  //Servo berhenti selama 1,5 detik
  servo360snack.write(90);
  delay(1500);
  //Menyimpan nilai variabel 1 untuk digunakan di history
  SnackFeed = 1;
}

//Fungsi untuk menjalankan servo 360 camilan jika terdapat kemacetan pada spiral makanan
void ServoCSnackStack(){
  //Servo berputar berlawanan jarum jam selamat 0,4 detik
  servo360snack.write(70);
  delay(400);
  //Servo berputar searah jarum jam 1 detik
  servo360snack.write(115);
  delay(1000);
  //Servo berhenti selama 1,5 detik
  servo360snack.write(90);
  delay(1500);
}

//Fungsi untuk memberhentikan servo 360 camilan
void StopServoCSnack() {
  servo360snack.write(90);
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

//Fungsi untuk melakukan pengecekan isi penampungan makanan pokok dan camilan
void StapleSnackInfo() {
  //Mengukur jarak isi dari penampungan makanan pokok dan camilan
  unsigned int distanceStaple = Distance1.ping_cm();
  unsigned int distanceSnack = Distance2.ping_cm();

  //Melakukan pengukuran yang didapat dari penampungan makanan pokok
  staplestatus = max(0, min(100, static_cast<int>(round((1 - ((float)distanceStaple - 2) / (22 - 2)) * 100))));
  if (distanceStaple > 22) {
      staplestatus = 0;
  }
  Serial.print("Staple Storage: ");
  Serial.print(staplestatus);
  Serial.println("%");

  //Melakukan pengukuran yang didapat dari penampungan camilan
  snackstatus = max(0, min(100, static_cast<int>(round((1 - ((float)distanceSnack - 2) / (22 - 2)) * 100))));
  if (distanceSnack > 22) {
      snackstatus = 0;
  } 
  Serial.print("Snack Storage: ");
  Serial.print(snackstatus);
  Serial.println("%");
  
  //Mengirimkan hasil sensor HC-SR04 ke Database
  Firebase.RTDB.setInt(&fbdt, String(IFT) + "/IFTPetFeeder/Storage/Snack", snackstatus);
  Firebase.RTDB.setInt(&fbdt, String(IFT) + "/IFTPetFeeder/Storage/Staple", staplestatus);
}

//Fungsi untuk melakukan pengeluaran makanan secara manual
void ManualFeeding(){
  mCheckValue = doc["Feeding"]["Check"]; //Mengambil isi dari atribut check di dalam objek Feeding ke variabel dalam bentuk int
  mFoodPositionData = doc["Feeding"]["FoodPosition"].as<String>(); //Mengambil isi dari atribut FoodPosition di dalam objek Feeding ke variabel dalam bentuk string
  mGramValue = doc["Feeding"]["Gram"]; //Mengambil isi dari atribut Gram di dalam objek Feeding ke variabel dalam bentuk int
  //Melakukan pengecekan terhadap variabel mCheckValue apakah bernilai 1 atau 0
  if (mCheckValue == 1){
    Feeding(); //Menjalankan fungsi feeding untuk melakukan pengeluaran makanan sesuai variabel mFoodPositionData dan mGramValue
    //Mengembalikan nilai menjadi 0 dan mengirimkannya ke firebase
    mCheckValue = 0; 
    Firebase.RTDB.setInt(&fbdt, String(IFT) + "/IFTPetFeeder/Feeding/Check", mCheckValue);
  }
}

//Fungsi untuk mengeluarkan makanan
void Feeding(){
  //Jika kondisi Staple atau makanan pokok maka akan dieksekusi
  if (mFoodPositionData == "Staple") {
    Status();
    ServoPSTNStaple(); //Memindahkan posisi wadah makanan ke makanan pokok
    Serial.print(mGramValue);
    Serial.println(" Gram");
    //Melakukan perhitungan isi berat makanan pada wadah makanan yang ada selama 3 detik
    unsigned long startTime = millis();
    while (millis() - startTime < 3000) {
      ReadLoadCell();
    }
    //Melakukan pengeluaran makanan sesuai dengan gram yang dibutuhkan dengan rumus berat sekarang ditambah berat yang diinginkan
    Serial.println("Dispensing Staple Foods");
    float TargetLoad = LoadCell.getData() + mGramValue;
    int counter = 0; //Counter untuk menghitung berapa kali ServoCStaple() dijalankan
    unsigned long startTimeOverall = millis(); // Waktu mulai keseluruhan
    while (LoadCell.getData() < TargetLoad) {
      // Periksa apakah sudah 10 menit (600000 milidetik)
      if (millis() - startTimeOverall >= 600000) {
          Serial.println("The loop stops because it has been 10 minutes");
          break;
      }
      ServoCStaple(); //Menjalankan fungsi untuk memutarkan servo 360 makanan pokok
      counter++; //Menambahkan nilai counter
      //Melakukan perhitungan isi berat makanan pada wadah makanan yang ada selama 1 detik
      unsigned long startTime = millis();
      float previousLoad = floor(LoadCell.getData() * 10) / 10.0; // Simpan berat sebelum loop, dipotong ke satu desimal
      while (millis() - startTime < 1000) {
        ReadLoadCell();
      }
      // Potong berat saat ini ke satu desimal
      float currentLoadRounded = floor(LoadCell.getData() * 10) / 10.0;
      // Jika berat tidak berubah sampai satu desimal, cek counter
      if (previousLoad == currentLoadRounded) {
          if (counter % 3 == 0) {
              ServoCStapleStack(); // Jalankan fungsi ServoCStapleStack() setiap kelipatan 3 kali
          }
      } else {
          counter = 0; // Reset counter jika berat berubah
      }
    }
    StopServoCStaple();
    // Setiap makanan yang sudah dikeluarkan akan masuk ke dalam history yang ada di database
    HistoryAllTime();
  }
  //Jika kondisi Snack atau camilan maka akan dieksekusi
  if (mFoodPositionData == "Snack") {
    Status();
    ServoPSTNSnack();
    Serial.print(mGramValue);
    Serial.println(" Gram");
    //Melakukan perhitungan isi dari berat makanan yang ada selama 3 detik
    unsigned long startTime = millis();
    while (millis() - startTime < 3000) {
      ReadLoadCell();
    }
    //Melakukan pengeluaran makanan sesuai dengan gram yang dibutuhkan dengan rumus berat sekarang ditambah berat yang diinginkan
    Serial.println("Dispensing Snack Foods");
    float TargetLoad = LoadCell.getData() + mGramValue;
    int counter = 0; // Counter untuk menghitung berapa kali ServoCStaple() dijalankan
    unsigned long startTimeOverall = millis(); // Waktu mulai keseluruhan
    while (LoadCell.getData() < TargetLoad) {
      // Periksa apakah sudah 10 menit (600000 milidetik)
      if (millis() - startTimeOverall >= 600000) {
          Serial.println("The loop stops because it has been 10 minutes");
          break; // Keluar dari loop
      }
      ServoCSnack();
      counter++; // Increment counter setiap kali ServoCStaple() dijalankan
      unsigned long startTime = millis();
      float previousLoad = floor(LoadCell.getData() * 10) / 10.0; // Simpan berat sebelum loop, dipotong ke satu desimal
      while (millis() - startTime < 1000) {
        ReadLoadCell();
      }
      // Potong berat saat ini ke satu desimal
      float currentLoadRounded = floor(LoadCell.getData() * 10) / 10.0;
      // Jika berat tidak berubah sampai satu desimal, cek counter
      if (previousLoad == currentLoadRounded) {
          if (counter % 3 == 0) {
              ServoCSnackStack(); // Jalankan fungsi ServoCStapleStack() setiap kelipatan 3 kali
          }
      } else {
          counter = 0; // Reset counter jika berat berubah
      }
    }
    StopServoCSnack();
    // Setiap makanan yang sudah dikeluarkan akan masuk ke dalam history yang ada di database
    HistoryAllTime();
  }
}

//Fungsi melakukan pembacaan tombol
void ButtonState(){
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);
  buttonState3 = digitalRead(buttonPin3);
}

//Fungsi untuk menyimpan informasi waktu
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
  // Variabel untuk menghitung jumlah tugas yang telah diproses
  Serial.println("----Scheduler----");
  int taskCount = 0;

  // Memproses setiap tugas dalam objek "Schedule"
  for (JsonPair task : schedule) {
    // Memeriksa apakah objek tugas valid
    if (task.value().isNull()) {
      Serial.println("Task object is null.");
      continue;
    }

    // Mendapatkan nilai dari masing-masing properti tugas
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

    // Memeriksa kondisi sebelum menjalankan Feeding()
    if (tasks[taskCount].Action == 1){
      if (tasks[taskCount].Day == timeWeekDay || tasks[taskCount].Day == "Everyday") {
        if (tasks[taskCount].Time == HourMinuteNTP){
          mFoodPositionData = tasks[taskCount].Food;
          mGramValue = tasks[taskCount].Gram;
          // Memeriksa mFoodPositionData
          if (mFoodPositionData == "Staple") {
            if (staplestatus > 0) {
              Feeding();
            }
          } 
          else if (mFoodPositionData == "Snack") {
            if (snackstatus > 0) {
              Feeding();
            }
          }
        }
      }
    }
    taskCount++;
  }
}