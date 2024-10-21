#if defined(ESP32)
#include <WiFi.h>
#include <time.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <time.h>
#endif
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

//define min timestamp
#define MIN_TIMESTAMP 1609459200 

// Define WiFi credentials
#define WIFI_SSID "arduinom"
#define WIFI_PASSWORD "asd12345"
//#define WIFI_SSID "Desktop_F9010136"
//#define WIFI_PASSWORD "gtx2080TI"


// Define Firebase API Key, Project ID, and user credentials
#define API_KEY "AIzaSyBewwdWp3eRIRNYfgdoiOcjqJAcuvIrO_g"
#define FIREBASE_PROJECT_ID "nutriverdeiot2"
#define USER_EMAIL "richardiasp@gmail.com"
#define USER_PASSWORD "123456"

// Define Firebase Data object, Firebase authentication, and configuration
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

struct SensorData {
  float tempar;
  float umidade;
  float templiq;
  float tds;
  float uvt;
  float sph;
  float vazao;
};

SensorData sensores = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// Inicializa o WIFI
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Recebe as leituras de sensores via Serial do Mega2560
int getSensorReadings() {
  if (Serial.available() == 0) return 0;
  char text[200] = { 0 };
  if (Serial.readBytesUntil('\n', (byte*)text, 200)) {
    Serial.println(text);
    if (sscanf(text, "%f,%f,%f,%f,%f,%f,%f", &sensores.tempar, &sensores.umidade, &sensores.templiq, &sensores.tds, &sensores.uvt, &sensores.sph, &sensores.vazao) == 7) return 1;
  }
  return 0;
}

//Conecta no Firebase
void conectFirebase() {
  // Print Firebase client version
  Serial.printf("Firebase Client %s\n\n", FIREBASE_CLIENT_VERSION);

  // Assign the API key
  config.api_key = API_KEY;

  // Assign the user sign-in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the callback function for the long-running token generation task
  config.token_status_callback = tokenStatusCallback;

  // Begin Firebase with configuration and authentication
  Firebase.begin(&config, &auth);

  // Reconnect to Wi-Fi if necessary
  Firebase.reconnectWiFi(true);
}

// Configurar a sincronização de horário via NTP
void configTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
}


// Envia dados para Firebase
void enviarParaFirebase(String path, FirebaseJson &jsondoc) {
  int maxRetries = 3;  // Definir o número máximo de tentativas
  int attempts = 0;
  bool success = false;

  while (attempts < maxRetries && !success) {
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), jsondoc.raw())) {
      Serial.printf("Data sent successfully to %s\n", path.c_str());
      success = true;  // O envio foi bem-sucedido, sair do loop
    } else {
      Serial.printf("Error sending data to %s: %s\n", path.c_str(), fbdo.errorReason().c_str());
      attempts++;
      delay(1000);  // Espera 1 segundo antes de tentar novamente
    }
  }

  if (!success) {
    Serial.println("Failed to send data after multiple attempts.");
  }
}


// Valida dados
bool isValidReadings() {
  return !(isnan(sensores.tempar) || isnan(sensores.umidade) || isnan(sensores.templiq) || isnan(sensores.tds) || isnan(sensores.uvt) || isnan(sensores.sph) || isnan(sensores.vazao));
}

//-----------------SETUP---------------------
void setup() {
  Serial.begin(115200);
  initWiFi();
  conectFirebase();
  configTime();
  delay(2000);
}


//----------------LOOP----------------------
void loop() {
    
  if (getSensorReadings() == 0) return;

  // Obter o timestamp atual
  time_t now = time(nullptr);
  struct tm* timeinfo = gmtime(&now);
  char timestamp[40];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", timeinfo);

  // Append "Z" to indicate UTC time
  strcat(timestamp, "Z");
  delay(1000);
 //-------------------------------------------------------LOAD FIREBASE--------------------------------------------
  FirebaseJson jsonDoc;
  
  if (isValidReadings() && now > MIN_TIMESTAMP) {
    
    jsonDoc.set("fields/medicao/doubleValue", sensores.tempar);
    jsonDoc.set("fields/datahora/timestampValue", timestamp);
    enviarParaFirebase("tempar/DHT22a_" + String(timestamp), jsonDoc);
  
    jsonDoc.clear();  // Limpa o conteúdo para reutilização
  
    jsonDoc.set("fields/medicao/doubleValue", sensores.umidade);
    jsonDoc.set("fields/datahora/timestampValue", timestamp);
    enviarParaFirebase("umidade/DHT22u_" + String(timestamp), jsonDoc);
  
    jsonDoc.clear();
  
    jsonDoc.set("fields/medicao/doubleValue", sensores.templiq);
    jsonDoc.set("fields/datahora/timestampValue", timestamp);
    enviarParaFirebase("templiq/DS18B20_" + String(timestamp), jsonDoc);
  
    jsonDoc.clear();
  
    jsonDoc.set("fields/medicao/doubleValue", sensores.tds);
    jsonDoc.set("fields/datahora/timestampValue", timestamp);
    enviarParaFirebase("tds/TDS_" + String(timestamp), jsonDoc);
  
    jsonDoc.clear();
  
    jsonDoc.set("fields/medicao/doubleValue", sensores.uvt);
    jsonDoc.set("fields/datahora/timestampValue", timestamp);
    enviarParaFirebase("radiacao/UV_" + String(timestamp), jsonDoc);
  
    jsonDoc.clear();
  
    jsonDoc.set("fields/medicao/doubleValue", sensores.sph);
    jsonDoc.set("fields/datahora/timestampValue", timestamp);
    enviarParaFirebase("ph/PH_" + String(timestamp), jsonDoc);
  
    jsonDoc.clear();
  
    jsonDoc.set("fields/medicao/doubleValue", sensores.vazao);
    jsonDoc.set("fields/datahora/timestampValue", timestamp);
    enviarParaFirebase("vazao/YFS201_" + String(timestamp), jsonDoc);
    
    jsonDoc.clear();

    jsonDoc.set("fields/tempar/doubleValue", sensores.tempar);
    jsonDoc.set("fields/umidade/doubleValue", sensores.umidade);
    jsonDoc.set("fields/templiq/doubleValue", sensores.templiq);
    jsonDoc.set("fields/tds/doubleValue", sensores.tds);
    jsonDoc.set("fields/radiacao/doubleValue", sensores.uvt);
    jsonDoc.set("fields/ph/doubleValue", sensores.sph);
    jsonDoc.set("fields/vazao/doubleValue", sensores.vazao);
    jsonDoc.set("fields/datahora/timestampValue", timestamp);
    
    enviarParaFirebase("allsensors/AllSensors_" + String(timestamp), jsonDoc);
    
    jsonDoc.clear();
  }else {
      Serial.println("Failed to read data.");
    }


  // Delay before the next reading
  delay(14000);
}
