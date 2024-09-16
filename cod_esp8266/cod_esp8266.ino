#if defined(ESP32)
#include <WiFi.h>
#include <time.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <time.h>
#endif
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

// Define WiFi credentials
#define WIFI_SSID "Desktop_F9010136"
#define WIFI_PASSWORD "gtx2080TI"


// Define Firebase API Key, Project ID, and user credentials
#define API_KEY "AIzaSyBewwdWp3eRIRNYfgdoiOcjqJAcuvIrO_g"
#define FIREBASE_PROJECT_ID "nutriverdeiot2"
#define USER_EMAIL "richardiasp@gmail.com"
#define USER_PASSWORD "123456"

// Define Firebase Data object, Firebase authentication, and configuration
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

float tempar = 0.0;
float umidade = 0.0;
float tds = 0.0;
float uvt = 0.0;

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
  char text[100] = { 0 };
  if (Serial.readBytesUntil('\n', (byte*)text, 100)) {
    Serial.println(text);
    if (sscanf(text, "%f,%f,%f,%f", &tempar, &umidade, &tds, &uvt) == 4) return 1;
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
    delay(500);
  }
  Serial.println();
}

//-----------------SETUP---------------------
void setup() {
  Serial.begin(115200);
  initWiFi();
  conectFirebase();
  configTime();
}


//----------------LOOP----------------------
void loop() {
    
    if (getSensorReadings() == 0) return;
    
    Serial.printf("\nTemperatura Ar = %.2f ºC \n", tempar);
    Serial.printf("Umidade = %.2f \n", umidade);
    Serial.printf("TDS = %.2f \n", tds);
    Serial.printf("UV = %.2f \n", uvt);
    Serial.println();

  // Obter o timestamp atual
  time_t now = time(nullptr);
  struct tm* timeinfo = gmtime(&now);
  char timestamp[40];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", timeinfo);

  // Append "Z" to indicate UTC time
  strcat(timestamp, "Z");

  // Define document path sensors
  String pathtar = "tempar/DHT22a_" + String(timestamp);
  String pathumi = "umidade/DHT22u_" + String(timestamp);
  String pathtds = "tds/TDS_" + String(timestamp);
  String pathuvt = "radiacao/UV_" + String(timestamp);

  // Create a FirebaseJson object for storing data sensors
  FirebaseJson conttar;
  FirebaseJson contumi;
  FirebaseJson conttds;
  FirebaseJson contuvt;

  // Check if the values are valid (not NaN)
  if (!isnan(tempar) && !isnan(umidade) && !isnan(tds)) {
    // Set fields in the FirebaseJson object
    conttar.set("fields/medicao/doubleValue", tempar);
    contumi.set("fields/medicao/doubleValue", umidade);
    conttds.set("fields/medicao/doubleValue", tds);
    contuvt.set("fields/medicao/doubleValue", uvt);

    // Add a timestamp field as a Firestore timestamp
    conttar.set("fields/datahora/timestampValue", timestamp);
    contumi.set("fields/datahora/timestampValue", timestamp);
    conttds.set("fields/datahora/timestampValue", timestamp);
    contuvt.set("fields/datahora/timestampValue", timestamp);

  //-------------------------------------------------------LOAD FIREBASE--------------------------------------------
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", pathtar.c_str(), conttar.raw())) {
      Serial.printf("Ok Conttar\n%s\n\n", fbdo.payload().c_str());
    } else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", pathumi.c_str(), contumi.raw())) {
      Serial.printf("Ok Contumi\n%s\n\n", fbdo.payload().c_str());
    } else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", pathtds.c_str(), conttds.raw())) {
      Serial.printf("Ok Conttds\n%s\n\n", fbdo.payload().c_str());
    } else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", pathuvt.c_str(), contuvt.raw())) {
      Serial.printf("Ok Conttds\n%s\n\n", fbdo.payload().c_str());
    } else {
      Serial.println(fbdo.errorReason());
    }
    
  } else {
    Serial.println("Failed to read data.");
  }

  // Delay before the next reading
  delay(10000);
}
