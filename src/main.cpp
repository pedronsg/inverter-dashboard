#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <ModbusRTU.h>
#include <EEPROM.h>

// Configurações padrão (usadas se não houver configuração salva)
const char* DEFAULT_SSID = "SEU_WIFI_SSID";
const char* DEFAULT_PASSWORD = "SUA_WIFI_PASSWORD";
const char* DEFAULT_AUTH_TOKEN = "inverter_2024_secure_token_xyz789";

// Configurações Modbus padrão
#define MODBUS_SERIAL Serial
#define DEFAULT_MODBUS_BAUD 9600
#define DEFAULT_INVERTER_ADDRESS 0x01

// Estrutura de configuração salva na EEPROM
struct Config {
  char ssid[32];
  char password[64];
  char auth_token[64];
  uint16_t modbus_baud;
  uint8_t inverter_address;
  uint16_t registers[10]; // Até 10 registros configuráveis
  uint8_t register_count;
  uint8_t checksum;
};

Config config;
bool configLoaded = false;

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

// Funções de configuração
void loadConfig() {
  EEPROM.begin(512);
  EEPROM.get(0, config);
  
  // Verificar checksum
  uint8_t checksum = 0;
  for (int i = 0; i < sizeof(config) - 1; i++) {
    checksum ^= ((uint8_t*)&config)[i];
  }
  
  if (checksum != config.checksum) {
    // Configuração inválida, usar padrões
    strcpy(config.ssid, DEFAULT_SSID);
    strcpy(config.password, DEFAULT_PASSWORD);
    strcpy(config.auth_token, DEFAULT_AUTH_TOKEN);
    config.modbus_baud = DEFAULT_MODBUS_BAUD;
    config.inverter_address = DEFAULT_INVERTER_ADDRESS;
    config.register_count = 6;
    config.registers[0] = 4067; // PV Power
    config.registers[1] = 5401; // Grid Power
    config.registers[2] = 10008; // House Consumption
    config.registers[3] = 10022; // Battery Power
    config.registers[4] = 10023; // Battery Level
    config.registers[5] = 10024; // Battery Health
    config.checksum = 0;
    for (int i = 0; i < sizeof(config) - 1; i++) {
      config.checksum ^= ((uint8_t*)&config)[i];
    }
    saveConfig();
  }
  configLoaded = true;
  Serial.println("Configuration loaded");
}

void saveConfig() {
  // Recalcular checksum
  config.checksum = 0;
  for (int i = 0; i < sizeof(config) - 1; i++) {
    config.checksum ^= ((uint8_t*)&config)[i];
  }
  
  EEPROM.put(0, config);
  EEPROM.commit();
  Serial.println("Configuration saved");
}

// Função para validar token de autenticação
bool validateToken() {
  if (!configLoaded) return false;
  
  if (server.hasHeader("Authorization")) {
    String auth = server.header("Authorization");
    if (auth.equals("Bearer " + String(config.auth_token))) {
      return true;
    }
  }
  
  // Verificar token via query parameter (para facilitar testes)
  if (server.hasArg("token")) {
    if (server.arg("token").equals(config.auth_token)) {
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
  if (!configLoaded) {
    Serial.println("Configuration not loaded!");
    return;
  }
  
  WiFi.begin(config.ssid, config.password);
  Serial.print("Connecting to WiFi: ");
  Serial.println(config.ssid);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
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
  if (!configLoaded) {
    Serial.println("Configuration not loaded!");
    return false;
  }
  
  Serial.println("Reading inverter data...");
  
  // Ler registros configurados
  uint16_t values[10];
  bool success = true;
  
  for (int i = 0; i < config.register_count; i++) {
    if (mb.readHreg(config.inverter_address, config.registers[i], &values[i], 1)) {
      Serial.println("Register " + String(config.registers[i]) + ": " + String(values[i]));
    } else {
      Serial.println("Error reading register " + String(config.registers[i]));
      success = false;
    }
  }
  
  if (success) {
    // Mapear valores para estrutura (assumindo ordem padrão)
    if (config.register_count >= 6) {
      inverterData.solar_production = values[0];
      inverterData.grid_power = (int16_t)values[1];
      inverterData.house_consumption = values[2];
      inverterData.battery_power = (int16_t)values[3];
      inverterData.battery_level = values[4];
      inverterData.battery_health = values[5];
    }
    inverterData.timestamp = millis();
  }
  
  return success;
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

// Handler para página de configuração
void handleConfig() {
  String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuration</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input, select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
        button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background: #0056b3; }
        .register-group { display: flex; gap: 10px; align-items: center; margin-bottom: 10px; }
        .register-group input { flex: 1; }
        .register-group button { flex: 0 0 auto; background: #dc3545; }
        .status { padding: 10px; margin: 10px 0; border-radius: 4px; }
        .success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 Inverter Configuration</h1>
        
        <form id="configForm">
            <h2>WiFi Settings</h2>
            <div class="form-group">
                <label for="ssid">WiFi SSID:</label>
                <input type="text" id="ssid" name="ssid" required>
            </div>
            <div class="form-group">
                <label for="password">WiFi Password:</label>
                <input type="password" id="password" name="password" required>
            </div>
            
            <h2>Security</h2>
            <div class="form-group">
                <label for="auth_token">Auth Token:</label>
                <input type="text" id="auth_token" name="auth_token" required>
            </div>
            
            <h2>Modbus Settings</h2>
            <div class="form-group">
                <label for="modbus_baud">Baud Rate:</label>
                <select id="modbus_baud" name="modbus_baud">
                    <option value="9600">9600</option>
                    <option value="19200">19200</option>
                    <option value="38400">38400</option>
                    <option value="57600">57600</option>
                    <option value="115200">115200</option>
                </select>
            </div>
            <div class="form-group">
                <label for="inverter_address">Inverter Address:</label>
                <input type="number" id="inverter_address" name="inverter_address" min="1" max="255" required>
            </div>
            
            <h2>Modbus Registers</h2>
            <div id="registers">
                <!-- Registers will be added here -->
            </div>
            <button type="button" onclick="addRegister()">+ Add Register</button>
            
            <div style="margin-top: 30px;">
                <button type="submit">💾 Save Configuration</button>
                <button type="button" onclick="resetConfig()" style="background: #dc3545; margin-left: 10px;">🔄 Reset to Defaults</button>
            </div>
        </form>
        
        <div id="status"></div>
    </div>
    
    <script>
        // Load current configuration
        fetch('/api/config')
            .then(response => response.json())
            .then(data => {
                document.getElementById('ssid').value = data.ssid || '';
                document.getElementById('password').value = data.password || '';
                document.getElementById('auth_token').value = data.auth_token || '';
                document.getElementById('modbus_baud').value = data.modbus_baud || 9600;
                document.getElementById('inverter_address').value = data.inverter_address || 1;
                
                // Load registers
                const registersDiv = document.getElementById('registers');
                registersDiv.innerHTML = '';
                if (data.registers) {
                    data.registers.forEach((reg, index) => {
                        addRegister(reg);
                    });
                }
            })
            .catch(error => {
                showStatus('Error loading configuration: ' + error, 'error');
            });
        
        function addRegister(value = '') {
            const registersDiv = document.getElementById('registers');
            const div = document.createElement('div');
            div.className = 'register-group';
            div.innerHTML = `
                <input type="number" placeholder="Register number" value="${value}" min="0" max="65535">
                <button type="button" onclick="removeRegister(this)">Remove</button>
            `;
            registersDiv.appendChild(div);
        }
        
        function removeRegister(button) {
            button.parentElement.remove();
        }
        
        function showStatus(message, type) {
            const statusDiv = document.getElementById('status');
            statusDiv.innerHTML = `<div class="status ${type}">${message}</div>`;
            setTimeout(() => statusDiv.innerHTML = '', 5000);
        }
        
        document.getElementById('configForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const formData = new FormData(this);
            const config = {
                ssid: formData.get('ssid'),
                password: formData.get('password'),
                auth_token: formData.get('auth_token'),
                modbus_baud: parseInt(formData.get('modbus_baud')),
                inverter_address: parseInt(formData.get('inverter_address')),
                registers: []
            };
            
            // Collect registers
            const registerInputs = document.querySelectorAll('#registers input');
            registerInputs.forEach(input => {
                if (input.value) {
                    config.registers.push(parseInt(input.value));
                }
            });
            
            fetch('/api/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(config)
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showStatus('Configuration saved successfully! Device will restart.', 'success');
                    setTimeout(() => location.reload(), 2000);
                } else {
                    showStatus('Error saving configuration: ' + data.error, 'error');
                }
            })
            .catch(error => {
                showStatus('Error saving configuration: ' + error, 'error');
            });
        });
        
        function resetConfig() {
            if (confirm('Reset to default configuration? This will clear all settings.')) {
                fetch('/api/config/reset', { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            showStatus('Configuration reset! Device will restart.', 'success');
                            setTimeout(() => location.reload(), 2000);
                        } else {
                            showStatus('Error resetting configuration: ' + data.error, 'error');
                        }
                    })
                    .catch(error => {
                        showStatus('Error resetting configuration: ' + error, 'error');
                    });
            }
        }
    </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}

// Handler para API de configuração
void handleConfigAPI() {
  if (server.method() == HTTP_GET) {
    // Retornar configuração atual
    StaticJsonDocument<512> doc;
    doc["ssid"] = config.ssid;
    doc["password"] = "***"; // Não enviar senha
    doc["auth_token"] = config.auth_token;
    doc["modbus_baud"] = config.modbus_baud;
    doc["inverter_address"] = config.inverter_address;
    
    JsonArray registers = doc.createNestedArray("registers");
    for (int i = 0; i < config.register_count; i++) {
      registers.add(config.registers[i]);
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
    
  } else if (server.method() == HTTP_POST) {
    // Salvar nova configuração
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (error) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
      return;
    }
    
    // Atualizar configuração
    strncpy(config.ssid, doc["ssid"], sizeof(config.ssid) - 1);
    strncpy(config.password, doc["password"], sizeof(config.password) - 1);
    strncpy(config.auth_token, doc["auth_token"], sizeof(config.auth_token) - 1);
    config.modbus_baud = doc["modbus_baud"];
    config.inverter_address = doc["inverter_address"];
    
    // Atualizar registros
    config.register_count = 0;
    if (doc["registers"].is<JsonArray>()) {
      for (JsonVariant reg : doc["registers"].as<JsonArray>()) {
        if (config.register_count < 10) {
          config.registers[config.register_count] = reg;
          config.register_count++;
        }
      }
    }
    
    saveConfig();
    server.send(200, "application/json", "{\"success\":true}");
    
    // Reiniciar após 2 segundos
    delay(2000);
    ESP.restart();
  }
}

// Handler para reset de configuração
void handleConfigReset() {
  if (server.method() == HTTP_POST) {
    // Limpar EEPROM
    for (int i = 0; i < 512; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    
    server.send(200, "application/json", "{\"success\":true}");
    
    // Reiniciar após 2 segundos
    delay(2000);
    ESP.restart();
  }
}

// Configurar rotas do servidor web
void setupWebServer() {
  server.on("/", handleDashboard);
  server.on("/data.json", handleDataJson);
  server.on("/index.html", handleDashboard);
  server.on("/style.css", handleStaticFile);
  server.on("/script.js", handleStaticFile);
  server.on("/config", handleConfig);
  server.on("/api/config", handleConfigAPI);
  server.on("/api/config/reset", handleConfigReset);
  
  server.begin();
  Serial.println("Web server started on port 80");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Inverter Modbus ESP8266...");
  
  // Carregar configuração
  loadConfig();
  
  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Error initializing SPIFFS");
    return;
  }
  
  // Configurar Modbus
  MODBUS_SERIAL.begin(config.modbus_baud, SERIAL_8N1);
  mb.begin(&MODBUS_SERIAL);
  mb.master();
  mb.setBaudrate(config.modbus_baud);
  mb.setBaudTimeout(1000);
  
  // Conectar WiFi
  connectWiFi();
  
  // Configurar OTA
  setupOTA();
  
  // Configurar servidor web
  setupWebServer();
  
  Serial.println("System started successfully!");
  Serial.println("Configuration page: http://" + WiFi.localIP().toString() + "/config");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  
  // Read inverter data every 5 seconds
  static unsigned long lastRead = 0;
  if (millis() - lastRead > 5000) {
    if (readInverterData()) {
      Serial.println("Data updated successfully");
    } else {
      Serial.println("Error reading inverter data");
    }
    lastRead = millis();
  }
  
  delay(100);
}
