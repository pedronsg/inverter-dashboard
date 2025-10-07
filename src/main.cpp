#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <ModbusRTU.h>

// Configurações WiFi
const char* ssid = "SEU_WIFI_SSID";
const char* password = "SUA_WIFI_PASSWORD";

// Configurações Modbus
// ESP-01: UART0 (TX=GPIO1, RX=GPIO3). Transceiver RS485 com auto-direção (DE/RE por hardware)
#define MODBUS_SERIAL Serial
#define MODBUS_BAUD 9600
#define INVERTER_ADDRESS 0x01

// Configurações de Segurança
#define AUTH_TOKEN "inverter_2024_secure_token_xyz789"

// Servidor Web
ESP8266WebServer server(80);

// Modbus RTU
ModbusRTU mb;

// Estrutura de dados do inversor
struct InverterData {
  uint32_t solar_production = 0;    // W - PV Total Power (4067)
  int16_t grid_power = 0;           // W - Measured Power (5401) 
  uint16_t house_consumption = 0;   // W - Total Consumption Power (10008)
  uint16_t battery_level = 0;       // % - Battery level (10023)
  int16_t battery_power = 0;        // W - Battery Power (10022)
  uint16_t battery_health = 0;      // % - Battery health (10024)
  unsigned long timestamp = 0;
};

InverterData inverterData;

// Função para validar token de autenticação
bool validateToken() {
  if (server.hasHeader("Authorization")) {
    String auth = server.header("Authorization");
    if (auth.equals("Bearer " + String(AUTH_TOKEN))) {
      return true;
    }
  }
  
  // Verificar token via query parameter (para facilitar testes)
  if (server.hasArg("token")) {
    if (server.arg("token").equals(AUTH_TOKEN)) {
      return true;
    }
  }
  
  return false;
}

// Função para enviar resposta de erro de autenticação
void sendAuthError() {
  server.sendHeader("WWW-Authenticate", "Bearer");
  server.send(401, "application/json", "{\"error\":\"Token inválido\"}");
}

// Função para conectar ao WiFi
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Falha na conexão WiFi!");
  }
}

// Função para configurar OTA
void setupOTA() {
  ArduinoOTA.setHostname("inverter-modbus-esp01");
  ArduinoOTA.setPassword("12345678");
  
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("Iniciando OTA " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA concluído");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erro OTA[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();
}

// Função para ler dados do inversor via Modbus
bool readInverterData() {
  Serial.println("Lendo dados do inversor...");
  
  // Ler PV Total Power (4067) - U_DWORD_R
  uint32_t pvPower = 0;
  if (mb.readHreg(INVERTER_ADDRESS, 4067, (uint16_t*)&pvPower, 2)) {
    inverterData.solar_production = pvPower;
    Serial.println("PV Power: " + String(inverterData.solar_production) + "W");
  } else {
    Serial.println("Erro ao ler PV Power");
    return false;
  }
  
  // Ler Measured Power (5401) - S_WORD
  int16_t measuredPower = 0;
  if (mb.readHreg(INVERTER_ADDRESS, 5401, (uint16_t*)&measuredPower, 1)) {
    inverterData.grid_power = measuredPower;
    Serial.println("Grid Power: " + String(inverterData.grid_power) + "W");
  } else {
    Serial.println("Erro ao ler Grid Power");
    return false;
  }
  
  // Ler Total Consumption Power (10008) - U_WORD
  uint16_t consumption = 0;
  if (mb.readHreg(INVERTER_ADDRESS, 10008, &consumption, 1)) {
    inverterData.house_consumption = consumption;
    Serial.println("House Consumption: " + String(inverterData.house_consumption) + "W");
  } else {
    Serial.println("Erro ao ler House Consumption");
    return false;
  }
  
  // Ler Battery level (10023) - U_WORD
  uint16_t batteryLevel = 0;
  if (mb.readHreg(INVERTER_ADDRESS, 10023, &batteryLevel, 1)) {
    inverterData.battery_level = batteryLevel;
    Serial.println("Battery Level: " + String(inverterData.battery_level) + "%");
  } else {
    Serial.println("Erro ao ler Battery Level");
    return false;
  }
  
  // Ler Battery Power (10022) - S_WORD
  int16_t batteryPower = 0;
  if (mb.readHreg(INVERTER_ADDRESS, 10022, (uint16_t*)&batteryPower, 1)) {
    inverterData.battery_power = batteryPower;
    Serial.println("Battery Power: " + String(inverterData.battery_power) + "W");
  } else {
    Serial.println("Erro ao ler Battery Power");
    return false;
  }
  
  // Ler Battery health (10024) - U_WORD
  uint16_t batteryHealth = 0;
  if (mb.readHreg(INVERTER_ADDRESS, 10024, &batteryHealth, 1)) {
    inverterData.battery_health = batteryHealth;
    Serial.println("Battery Health: " + String(inverterData.battery_health) + "%");
  } else {
    Serial.println("Erro ao ler Battery Health");
    return false;
  }
  
  inverterData.timestamp = millis();
  return true;
}

// Handler para servir data.json (PROTEGIDO)
void handleDataJson() {
  // Verificar autenticação
  if (!validateToken()) {
    sendAuthError();
    Serial.println("Tentativa de acesso não autorizada ao data.json");
    return;
  }
  
  StaticJsonDocument<512> doc;
  
  doc["solar_production"] = inverterData.solar_production;
  doc["battery_level"] = inverterData.battery_level;
  doc["battery_power"] = inverterData.battery_power;
  doc["house_consumption"] = inverterData.house_consumption;
  doc["grid_power"] = inverterData.grid_power;
  doc["timestamp"] = "2024-01-15T10:30:00Z"; // Pode ser melhorado com RTC
  
  String response;
  serializeJson(doc, response);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET");
  server.sendHeader("Content-Type", "application/json");
  server.send(200, "application/json", response);
  
  Serial.println("Data.json servido com autenticação: " + response);
}

// Handler para servir o dashboard
void handleDashboard() {
  if (SPIFFS.begin()) {
    File file = SPIFFS.open("/index.html", "r");
    if (file) {
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(404, "text/plain", "Ficheiro não encontrado");
    }
  } else {
    server.send(500, "text/plain", "Erro ao inicializar SPIFFS");
  }
}

// Handler para servir ficheiros estáticos
void handleStaticFile() {
  String path = server.uri();
  String contentType = "text/plain";
  
  if (path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js")) contentType = "application/javascript";
  else if (path.endsWith(".json")) contentType = "application/json";
  
  if (SPIFFS.begin()) {
    File file = SPIFFS.open(path, "r");
    if (file) {
      server.streamFile(file, contentType);
      file.close();
    } else {
      server.send(404, "text/plain", "Ficheiro não encontrado");
    }
  } else {
    server.send(500, "text/plain", "Erro ao inicializar SPIFFS");
  }
}

// Configurar rotas do servidor web
void setupWebServer() {
  server.on("/", handleDashboard);
  server.on("/data.json", handleDataJson);
  server.on("/index.html", handleDashboard);
  server.on("/style.css", handleStaticFile);
  server.on("/script.js", handleStaticFile);
  
  server.begin();
  Serial.println("Servidor web iniciado na porta 80");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Inversor Modbus ESP32...");
  
  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao inicializar SPIFFS");
    return;
  }
  
  // Configurar Modbus
  MODBUS_SERIAL.begin(MODBUS_BAUD, SERIAL_8N1);
  mb.begin(&MODBUS_SERIAL);
  mb.master();
  mb.setBaudrate(MODBUS_BAUD);
  mb.setBaudTimeout(1000);
  
  // Conectar WiFi
  connectWiFi();
  
  // Configurar OTA
  setupOTA();
  
  // Configurar servidor web
  setupWebServer();
  
  Serial.println("Sistema iniciado com sucesso!");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  
  // Ler dados do inversor a cada 5 segundos
  static unsigned long lastRead = 0;
  if (millis() - lastRead > 5000) {
    if (readInverterData()) {
      Serial.println("Dados atualizados com sucesso");
    } else {
      Serial.println("Erro ao ler dados do inversor");
    }
    lastRead = millis();
  }
  
  delay(100);
}
