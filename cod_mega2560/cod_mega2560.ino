//--------------------BIBLIOTECAS-----------------------------------------
#include <DHT.h>
#include <EEPROM.h>
#include "GravityTDS.h"


//---------------SENSOR DE UMIDADE E TEMPERATURA DO AR--------------------
#define DHTPIN 10      // Pino digital onde o DHT22 está conectado
#define DHTTYPE DHT22 // Define o tipo de sensor DHT
DHT dht(DHTPIN, DHTTYPE);
//------------------------------------------------------------------------

//-------------SENSOR DE TOTAL DE SOLIDOS DISSOLVIDOS (TDS)---------------
#define TdsSensorPin A2
GravityTDS gravityTds;
float templiq = 25.0;
float tds = 0.0;
//------------------------------------------------------------------------

//-------------------------SENSOR ULTRAVIOLETA----------------------------
#define pinSensorUV    A0
int leituraUV=0; // Variavel para armazenar a leitura da porta analógica
byte indiceUV=0; // Variavel para armazenar a conversão para indice UV
//------------------------------------------------------------------------


void setup() {
  Serial.begin(115200); //Inicializa serial
  Serial3.begin(115200); //Inicializa serial3

  //Inicializa sensores
  dht.begin(); //DHT22
  gravityTds.setPin(TdsSensorPin); //TDS
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(4096);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization
  pinMode(pinSensorUV, INPUT); //Sensor UV

  
  delay(15000);
}

void loop() { 

  //LEITURA DE DADOS DO DHT22
  float umidade = dht.readHumidity();     // Lê a umidade
  float tempar = dht.readTemperature(); // Lê a temperatura

  //LEITURA DE DADOS DO TDS
  gravityTds.setTemperature(templiq);  // Define a temperatura e executa a compensação
  gravityTds.update();  //Amostra e Calculo
  float tds = gravityTds.getTdsValue();  // Pega o valor

  //LEITURA DE DADOS DO SENSOR ULTRAVIOLETA
  leituraUV = analogRead(pinSensorUV); // Realçiza a leitura na porta analógica
  indiceUV = map(leituraUV, 0,203,0,11) ; // Converte a faixa de sinal do sensor de 0v a 1v para o índice uv de 0 a 10.
  
   //verifica se foram lidos corretamente,
   if (isnan(umidade) || isnan(tempar) || isnan(tds)) {
    Serial.println("Falha na leitura dos sensores!");
  } else {
    // Envia os dados para o ESP8266 via Serial
    Serial.print("Temperatura Ar:"); // Indicador de temperatura
    Serial.println(tempar);
    Serial.print("Umidade:"); // Indicador de umidade
    Serial.println(umidade);
    Serial.print("TDS:"); // Indicador de umidade
    Serial.println(tds);
    Serial.print("UV:"); // Indicador de umidade
    Serial.println(indiceUV);
  }


  //Verifica se há dados recebidos e imprime
  while(Serial3.available()){
    Serial.print((char)Serial3.read());
  }

  //Envia os dados
  Serial3.print(tempar);
  Serial3.print(',');
  Serial3.print(umidade);
  Serial3.print(',');
  Serial3.print(tds);
  Serial3.print(',');
  Serial3.print(indiceUV);
  Serial3.print('\n');

  //Espera 15 segundos para fazer o loop
  delay(15000);
}
