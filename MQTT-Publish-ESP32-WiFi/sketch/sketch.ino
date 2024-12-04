// Incluir bibliotecas
#include <WiFi.h> // Biblioteca Wi-Fi
#include <PubSubClient.h> // Biblioteca MQTT
#include <ArduinoJson.h> // Biblioteca para lidar com JSON (instale via Gerenciador de Bibliotecas do Arduino)

// Configurações do Wi-Fi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Configurações do Broker MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "irrigation/control";

// Configurações do ESP32
WiFiClient espClient;
PubSubClient client(espClient);

const int ledPin = 4;              // LED conectado ao GPIO 4
const int soilMoisturePin = 34;    // Pino analógico do sensor de umidade do solo (simulado)

// Variáveis para leitura da umidade do solo
int soilMoistureValue = 0;
float soilMoisturePercentage = 0.0;

// Função para conectar ao Wi-Fi
void setup_wifi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");
}

// Função de callback para processar mensagens recebidas do MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  // Converte o payload recebido para uma string
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Comando recebido: ");
  Serial.println(message);

  // Criar um objeto JSON para analisar a mensagem
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  // Verifica se a mensagem é um JSON válido
  if (error) {
    Serial.print("Erro ao analisar JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // Extrai o comando do JSON
  const char* command = doc["command"];

  // Ligar ou desligar o LED com base no comando
  if (String(command) == "ligar") {
    Serial.println("Dispositivo ligado!");
    digitalWrite(ledPin, HIGH); // Acende o LED
  } else if (String(command) == "desligar") {
    Serial.println("Dispositivo desligado!");
    digitalWrite(ledPin, LOW); // Apaga o LED
  }
}

// Função para reconectar ao MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao Broker MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado!");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("Falha ao conectar, tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Configurar o pino do LED e do sensor
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // LED inicializado como desligado
  pinMode(soilMoisturePin, INPUT);

  // Conectar ao Wi-Fi
  setup_wifi();

  // Configurar o cliente MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Leitura do sensor de umidade do solo
  soilMoistureValue = analogRead(soilMoisturePin);

  // Normalizar o valor para 0–100%
  soilMoisturePercentage = map(soilMoistureValue, 0, 4095, 0, 100);

  Serial.print("Umidade do Solo (valor bruto): ");
  Serial.println(soilMoistureValue);
  Serial.print("Umidade do Solo (%): ");
  Serial.println(soilMoisturePercentage);

  // Publicar valor da umidade do solo em porcentagem
  StaticJsonDocument<128> soilDoc;
  soilDoc["soilMoisture"] = soilMoisturePercentage;
  char buffer[128];
  serializeJson(soilDoc, buffer);
  client.publish("sensor/soil_moisture", buffer);

  delay(2000); // Aguarda 2 segundos antes da próxima leitura
}