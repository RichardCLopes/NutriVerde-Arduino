//--------------------BIBLIOTECAS-----------------------------------------
#include <DHT.h>
#include <EEPROM.h>
#include "GravityTDS.h"
#include <OneWire.h>
#include <DallasTemperature.h>


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

//------------------------------SENSOR PH---------------------------------
//Para calibrar o sensor de pH é necessario seguir os seguintes passos
//1º - Medir voltagem do sensor no vinagre (pH 3) e no alvejante(pH 13)
//2º - Tirar a equação da reta em https://pt.symbolab.com/solver/line-equation-calculator
//3º - Altera os valores de valor_calibracao e multi de acordo com a equação
//Caso não tenha os materiais, é possivel calibrar utilizando outros elementos, cujo saiba o pH
//Se for possível calibrar o potenciometro do analogico utilizado em 2.5V a fórmula é -5.7 * X -21.24
int ph_pin = A4; 
//------------------------------------------------------------------------

//---------------------------SENSOR TERMOMETRO----------------------------
#define ONE_WIRE_BUS 7 // Define pino do sensor
OneWire oneWire(ONE_WIRE_BUS); // Cria um objeto OneWire
DallasTemperature sensors(&oneWire); // Informa a referencia da biblioteca dallas temperature para Biblioteca onewire
//------------------------------------------------------------------------

//---------------------------SENSOR DE VAZÃO----------------------------
//definicao do pino do sensor e de interrupcao
const int INTERRUPCAO_SENSOR = 0; //interrupt = 0 equivale ao pino digital 2
const int PINO_SENSORVZ = 2;

//definicao de variaveis
unsigned long contador = 0;
const float FATOR_CALIBRACAO = 4.5; //
float fluxo = 0;
unsigned long tempo_antes = 0;

void contador_pulso() {
  contador++;
}
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
  pinMode(PINO_SENSORVZ, INPUT_PULLUP); //Sensor vazão
  sensors.begin(); //Inicia o termometro
  
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

  //LEITURA DO PH
  int measure = analogRead(ph_pin);
  //Serial.print("Measure: ");
  //Serial.print(measure);
  measure = measure - measure*0.15;
  double voltage = (5 / 1024.0) * measure; 
  //Serial.print("\tVoltage: ");
  //Serial.print(voltage, 3);
  float PH_fim = 3.40;
  float PH_ini = 2.75;
  float PH_step = (PH_fim - PH_ini)/3;
  float sph = 7 + ((PH_ini-voltage)/PH_step);

  //float sph = 7 + ((2.5 - voltage) / 0.18);
  //Serial.print("\tPH: ");
  //Serial.print(sph, 3);
  //Serial.println("");
  


  //LEITURA TEMPERATURA DO LIQUIDO
  sensors.requestTemperatures(); // Envia comando para realizar a conversão de temperatura
  float templiq = sensors.getTempCByIndex(0);

  //LEITURA DE VAZÃO
  if((millis() - tempo_antes) > 1000){  
  detachInterrupt(INTERRUPCAO_SENSOR); //desabilita a interrupcao para realizar a conversao do valor de pulsos
  fluxo = ((1000.0 / (millis() - tempo_antes)) * contador) / FATOR_CALIBRACAO; //conversao do valor de pulsos para L/min

  contador = 0; //reinicializacao do contador de pulsos
  tempo_antes = millis(); //atualizacao da variavel tempo_antes
  attachInterrupt(INTERRUPCAO_SENSOR, contador_pulso, FALLING); //contagem de pulsos do sensor    
  }
  
  
   //verifica se foram lidos corretamente,
   if (isnan(tempar) || isnan(umidade) || isnan(tds) || isnan(indiceUV) || isnan(sph) || isnan(templiq) || isnan(fluxo)) {
    Serial.println("Falha na leitura dos sensores!");
  } else {
    // Envia os dados para o ESP8266 via Serial
    Serial.print(" Temp. Ar: "); // Indicador de temperatura
    Serial.print(tempar);
    Serial.print(" ºC\t\tUmidade: "); // Indicador de umidade
    Serial.print(umidade);
    Serial.print(" %\t\tTDS :"); // Indicador de umidade
    Serial.print(tds);
    Serial.print(" Mg\\L\t\tUV: "); // Indicador de umidade
    Serial.print(indiceUV);
    Serial.print(" W\\M2\t\tpH: "); // Indicador de umidade
    Serial.print(sph);
    Serial.print(" ºC\t\tTemp. Liq: "); // Indicador de umidade
    Serial.print(templiq);
    Serial.print(" ºC\t\tVazao: "); // Indicador de umidade
    Serial.print(fluxo);
    Serial.print(" L\\min\n");
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
  Serial3.print(',');
  Serial3.print(sph);
  Serial3.print(',');
  Serial3.print(templiq);
  Serial3.print(',');
  Serial3.print(fluxo);
  Serial3.print('\n');

  //Espera 15 segundos para fazer o loop
  delay(5000);
}
