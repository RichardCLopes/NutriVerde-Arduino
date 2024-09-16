#include <DHT.h>

#define DHTPIN 2      // Pino digital onde o DHT22 está conectado
#define DHTTYPE DHT22 // Define o tipo de sensor DHT

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial3.begin(115200);

  dht.begin();
  delay(15000);
}

float temperature, pressure;

void loop() { 

  //le os dados
  float humidity = dht.readHumidity();     // Lê a umidade
  float temperature = dht.readTemperature(); // Lê a temperatura

   //verifica se foram lidos corretamente,
   if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Falha na leitura do DHT22!");
  } else {
    // Envia os dados para o ESP8266 via Serial
    Serial.print("T:"); // Indicador de temperatura
    Serial.println(temperature);
    Serial.print("H:"); // Indicador de umidade
    Serial.println(humidity);
  }

  //Verifica se há dados recebidos e imprime
  while(Serial3.available()){
    Serial.print((char)Serial3.read());
  }

  //Envia os dados
  Serial3.print(temperature);
  Serial3.print(',');
  Serial3.print(humidity);

  //Espera 15 segundos para fazer o loop
  delay(15000);
}
