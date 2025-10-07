# 🚀 Como Ver o Site a Correr

## 📋 Pré-requisitos

1. **ESP-01 (ESP8266)** conectado ao computador
2. **Cabo USB-Serial** (CP2102, CH340, etc.)
3. **PlatformIO** instalado no VS Code
4. **Rede WiFi** disponível

## 🔧 Passo 1: Compilar e Carregar o Firmware

### 1.1 Conectar o ESP-01
```
ESP-01 → USB-Serial
VCC    → 3.3V
GND    → GND
TX     → RX (do cabo)
RX     → TX (do cabo)
CH_PD  → 3.3V (ou 10kΩ para 3.3V)
GPIO0  → GND (apenas durante upload)
```

### 1.2 Configurar a Porta Serial
1. Abrir **Device Manager** (Windows)
2. Encontrar a porta COM (ex: COM3, COM4)
3. Editar `platformio.ini`:
```ini
upload_port = COM3    ; Substituir pela tua porta
monitor_port = COM3   ; Substituir pela tua porta
```

### 1.3 Compilar e Carregar
```bash
# No terminal do PlatformIO
pio run --target upload
```

## 🌐 Passo 2: Aceder ao Site

### 2.1 Ver o IP do ESP-01
1. Abrir **Serial Monitor** (PlatformIO)
2. Ver mensagem: `WiFi connected! IP: 192.168.1.100`
3. **Anotar o IP** (ex: 192.168.1.100)

### 2.2 Aceder ao Dashboard
```
http://192.168.1.100/
```

### 2.3 Aceder à Configuração
```
http://192.168.1.100/config
```

## 🔧 Passo 3: Configurar WiFi e Modbus

### 3.1 Primeira Configuração
1. Ir para `/config`
2. **WiFi Settings**:
   - SSID: Nome da tua rede WiFi
   - Password: Senha da rede
3. **Security**:
   - Auth Token: `inverter_2024_secure_token_xyz789`
4. **Modbus Settings**:
   - Baud Rate: `9600`
   - Inverter Address: `1`
5. **Modbus Registers**:
   - 4067 (PV Power)
   - 5401 (Grid Power)
   - 10008 (House Consumption)
   - 10022 (Battery Power)
   - 10023 (Battery Level)
   - 10024 (Battery Health)
6. Clicar **"Save Configuration"**

### 3.2 Reinicialização
- O ESP-01 reinicia automaticamente
- Aguardar 30 segundos
- Verificar novo IP no Serial Monitor

## 📱 Passo 4: Usar o Dashboard

### 4.1 Dashboard Principal
- **URL**: `http://IP_DO_ESP/`
- **Funcionalidades**:
  - Visualização em tempo real
  - Fluxo de energia animado
  - Dados do inversor
  - Responsivo para mobile

### 4.2 Modo Teste
- Se não houver inversor conectado
- Usa dados simulados
- Permite testar a interface

## 🔍 Troubleshooting

### Problema: "WiFi connection failed"
**Solução**:
1. Verificar SSID e senha
2. Verificar se a rede está 2.4GHz
3. Verificar distância do router

### Problema: "Error reading inverter data"
**Solução**:
1. Verificar ligações Modbus
2. Verificar endereço do inversor
3. Verificar registros Modbus

### Problema: Site não carrega
**Solução**:
1. Verificar IP correto
2. Verificar se ESP-01 está ligado
3. Tentar reiniciar o ESP-01

### Problema: "Token inválido"
**Solução**:
1. Verificar token na configuração
2. Usar token correto no dashboard
3. Reiniciar após alterar token

## 📊 Estrutura do Site

```
http://IP_DO_ESP/
├── /                    → Dashboard principal
├── /config             → Página de configuração
├── /data.json          → API de dados (protegida)
├── /api/config         → API de configuração
└── /api/config/reset   → Reset de configuração
```

## 🔐 Segurança

- **Token obrigatório** para `/data.json`
- **Configuração protegida** via interface web
- **Reset seguro** para configurações padrão

## 📱 Mobile

- **Design responsivo** para telemóvel
- **Interface otimizada** para touch
- **Funciona offline** (dados em cache)

## 🚀 Próximos Passos

1. **Conectar inversor** via Modbus
2. **Configurar registros** específicos
3. **Personalizar interface** se necessário
4. **Monitorizar dados** em tempo real

---

**💡 Dica**: Mantém o Serial Monitor aberto para ver logs e debug!
