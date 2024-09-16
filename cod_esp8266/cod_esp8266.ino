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

float temperature = 0.0;
float humidity = 0.0;

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
    if (sscanf(text, "%f,%f", &temperature, &humidity) == 2) return 1;
  }
  return 0;
}

//Conecta no Firebase
void conectFirebase() {
  // Print Firebase client version
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

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
    Serial.printf("Temperature = %.2f ºC \n", temperature);
    Serial.printf("Humidity = %.2f \n", humidity);
    Serial.println();

  // Obter o timestamp atual
  time_t now = time(nullptr);
  struct tm* timeinfo = gmtime(&now);
  char timestamp[40];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", timeinfo);

  // Append "Z" to indicate UTC time
  strcat(timestamp, "Z");

  // Define a unique document ID for each record (e.g., using timestamp)
  String documentPath = "Sensores/DHT22_" + String(timestamp);

  // Create a FirebaseJson object for storing data
  FirebaseJson content;

  // Check if the values are valid (not NaN)
  if (!isnan(temperature) && !isnan(humidity)) {
    // Set the 'Temperature' and 'Humidity' fields in the FirebaseJson object
    content.set("fields/Temperature/doubleValue", temperature);
    content.set("fields/Humidity/doubleValue", humidity);

    // Add a timestamp field as a Firestore timestamp
    content.set("fields/Timestamp/timestampValue", timestamp);

  // Use the createDocument method to add a new document with each reading
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    } else {
      Serial.println(fbdo.errorReason());
    }
  } else {
    Serial.println("Failed to read DHT data.");
  }

  // Delay before the next reading
  delay(10000);
}
