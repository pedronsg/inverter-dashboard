async function atualizarDados() {
  try {
  // Simulated data to work without a server
    const data = {
      solar_production: 2500,
      battery_level: 75,
      battery_power: -500, // Negative = discharging
      house_consumption: 1200, // Lower consumption
      grid_power: -300, // Negative = exporting to grid
      timestamp: new Date().toISOString()
    };
    
  // Try to read from data.json if available (with server)
    try {
      const res = await fetch("data.json?_=" + Date.now());
      const jsonData = await res.json();
      Object.assign(data, jsonData);
    } catch (fetchError) {
    }
  
  // Update component values
    document.getElementById("solar-production").textContent = data.solar_production + " W";
    document.getElementById("battery-power").textContent = formatBatteryPower(data.battery_power) + " W";
    document.getElementById("house-consumption").textContent = data.house_consumption + " W";
    document.getElementById("grid-power").textContent = formatGridPower(data.grid_power) + " W";
    
  // Update battery icon based on level
    const batteryIcon = document.getElementById("battery-icon");
    const level = data.battery_level;
    if (level >= 90) {
      batteryIcon.textContent = "🔋"; // Full battery
    } else if (level >= 70) {
      batteryIcon.textContent = "🔋"; // High battery
    } else if (level >= 50) {
      batteryIcon.textContent = "🔋"; // Medium battery
    } else if (level >= 30) {
      batteryIcon.textContent = "🔋"; // Low battery
    } else if (level >= 10) {
      batteryIcon.textContent = "🔋"; // Very low battery
    } else {
      batteryIcon.textContent = "🔋"; // Critical battery
    }
    
  // Update timestamp
    const timestamp = data.timestamp ? new Date(data.timestamp).toLocaleTimeString() : new Date().toLocaleTimeString();
    document.getElementById("timestamp").textContent = timestamp;
    
  // Control energy flow
    controlarFluxoEnergia(data);
    
  // Update system status
    document.getElementById("system-status").textContent = "Connected";
    document.getElementById("system-status").style.color = "#22c55e";
    
    } catch (e) {
    document.getElementById("system-status").textContent = "Connection error";
    document.getElementById("system-status").style.color = "#ef4444";
  }
}

function formatPower(power) {
  if (power > 0) {
    return "+" + power;
  } else if (power < 0) {
    return power.toString();
  } else {
    return "0";
  }
}

function formatGridPower(power) {
  // Grid: + = import (◀), - = export (▶)
  if (power > 0) {
    return `◀ ${power}`; // Importa da rede (seta grossa curta)
  } else if (power < 0) {
    return `${Math.abs(power)} ▶`; // Exporta para a rede (seta grossa curta)
  } else {
    return `0`; // Sem fluxo
  }
}

function formatBatteryPower(power) {
  // Battery: + = charging (▼), - = discharging (▲)
  if (power > 0) {
    return `▼ ${power}`; // Carrega a bateria (seta para baixo)
  } else if (power < 0) {
    return `${Math.abs(power)} ▲`; // Descarrega a bateria (seta para cima)
  } else {
    return `0`; // Sem fluxo
  }
}

function controlarFluxoEnergia(data) {
  
  // Desenhar paths curvos via SVG com base nas posições atuais
  const svg = document.getElementById('flow-svg');
  if (!svg) {
    return;
  }
  
  // Redimensionar SVG para ocupar toda a área
  const container = document.querySelector('.energy-flow');
  const rect = container.getBoundingClientRect();
  svg.setAttribute('width', rect.width);
  svg.setAttribute('height', rect.height);
  svg.setAttribute('viewBox', `0 0 ${rect.width} ${rect.height}`);
  
  
  // limpar
  while (svg.firstChild) svg.removeChild(svg.firstChild);

  const solar = centerOf('.solar-panels');
  const grid = centerOf('.grid');
  const house = centerOf('.house');
  const battery = centerOf('.battery');
  
  // Debug: verificar se as posições estão corretas

  // helpers para path bezier suave (curvas para evitar cruzamento no centro)
  function curvedPath(a, b, intensity = 0.4) {
    const dx = b.x - a.x;
    const dy = b.y - a.y;
    const distance = Math.sqrt(dx*dx + dy*dy);
    
    // Control points mais inteligentes baseados na direção
    let cx1, cy1, cx2, cy2;
    
    if (Math.abs(dx) > Math.abs(dy)) {
      // Movimento mais horizontal
      cx1 = a.x + dx * intensity;
      cy1 = a.y;
      cx2 = b.x - dx * intensity;
      cy2 = b.y;
    } else {
      // Movimento mais vertical
      cx1 = a.x;
      cy1 = a.y + dy * intensity;
      cx2 = b.x;
      cy2 = b.y - dy * intensity;
    }
    
    return `M ${a.x},${a.y} C ${cx1},${cy1} ${cx2},${cy2} ${b.x},${b.y}`;
  }
  
  // Função simples para linha reta (para debug)
  function straightPath(a, b) {
    return `M ${a.x},${a.y} L ${b.x},${b.y}`;
  }

  function addPath(id, from, to, color, active) {
    
    // Calcular raio baseado no tamanho da tela
    const isMobile = window.innerWidth < 768;
    const radius = isMobile ? 40 : 60; // 80px/2 ou 120px/2
    
    const fromPoint = pointOnCircle(from, to, radius);
    const toPoint = pointOnCircle(to, from, radius);
    
    
    // Sempre criar a linha (sempre visível)
    const path = document.createElementNS('http://www.w3.org/2000/svg','path');
    path.setAttribute('id', id);
    path.setAttribute('d', curvedPath(fromPoint, toPoint));
    path.setAttribute('class', 'flow-path'); // Sempre visível
    path.setAttribute('stroke', color);
    path.setAttribute('stroke-width', '2');
    svg.appendChild(path);

    // Bolinha de fluxo - apenas se ativo
    if (active) {
      const dot = document.createElementNS('http://www.w3.org/2000/svg','circle');
      dot.setAttribute('r', '4');
      dot.setAttribute('fill', color);
      dot.setAttribute('class','flow-dot-svg');
      dot.setAttribute('opacity', '1');
      svg.appendChild(dot);
      
      // Calcular velocidade baseada na potência
      const power = getPowerForPath(id, data);
      const speed = calculateSpeed(power);
      
      // Animação da bolinha ao longo do path
      const animateMotion = document.createElementNS('http://www.w3.org/2000/svg','animateMotion');
      animateMotion.setAttribute('dur', `${speed}s`);
      animateMotion.setAttribute('repeatCount','indefinite');
      const mpath = document.createElementNS('http://www.w3.org/2000/svg','mpath');
      mpath.setAttributeNS('http://www.w3.org/1999/xlink','href',`#${id}`);
      animateMotion.appendChild(mpath);
      dot.appendChild(animateMotion);
      
    } else {
      // Criar bolinha invisível para manter a estrutura
      const dot = document.createElementNS('http://www.w3.org/2000/svg','circle');
      dot.setAttribute('r', '4');
      dot.setAttribute('fill', color);
      dot.setAttribute('class','flow-dot-svg');
      dot.setAttribute('opacity', '0'); // Invisível
      svg.appendChild(dot);
    }
  }

  // regras de ativação conforme dados - apenas fluxos diretos
  const solarToBattery = data.solar_production > 0 && data.battery_power > 0;
  const solarToHouse = data.solar_production > data.house_consumption;
  const solarToGrid = data.solar_production > data.house_consumption && data.battery_power <= 0;
  const batteryToHouse = data.battery_power < 0;
  const gridToHouse = data.grid_power < 0;

  // cores aproximadas como na imagem
  const cSolar = '#f59e0b';
  const cGrid = '#8b5cf6';
  const cBattery = '#ef4444';
  const cHouse = '#06b6d4';

  // Verificar se as posições são válidas
  if (solar.x === 0 && solar.y === 0) {
    return;
  }

  // Desenhar todas as linhas possíveis (sempre visíveis)
  
  // Linhas do Solar
  addPath('p-solar-batt', solar, battery, cSolar, solarToBattery);
  addPath('p-solar-home', solar, house, cSolar, solarToHouse);
  addPath('p-solar-grid', solar, grid, cSolar, solarToGrid);
  
  // Linhas do Grid
  addPath('p-grid-solar', grid, solar, cGrid, false); // Sempre inativo
  addPath('p-grid-home', grid, house, cGrid, gridToHouse);
  addPath('p-grid-batt', grid, battery, cGrid, false); // Sempre inativo
  
  // Linhas da Casa
  addPath('p-home-solar', house, solar, cHouse, false); // Sempre inativo
  addPath('p-home-grid', house, grid, cHouse, false); // Sempre inativo
  addPath('p-home-batt', house, battery, cHouse, false); // Sempre inativo
  
  // Linhas da Bateria
  addPath('p-batt-solar', battery, solar, cBattery, false); // Sempre inativo
  addPath('p-batt-home', battery, house, cBattery, batteryToHouse);
  addPath('p-batt-grid', battery, grid, cBattery, false); // Sempre inativo
  
  
  // helper para atualizar tamanhos do SVG
}

function centerOf(selector) {
  const el = document.querySelector(selector);
  const svgHost = document.querySelector('.energy-flow');
  if (!el || !svgHost) {
    return { x: 0, y: 0 };
  }
  
  const er = el.getBoundingClientRect();
  const hr = svgHost.getBoundingClientRect();
  
  const x = er.left + er.width/2 - hr.left;
  const y = er.top + er.height/2 - hr.top;
  
  return { x, y };
}

// Função para calcular ponto na borda do círculo
function pointOnCircle(center, target, radius) {
  const dx = target.x - center.x;
  const dy = target.y - center.y;
  const distance = Math.sqrt(dx * dx + dy * dy);
  
  if (distance === 0) return center; // Mesmo ponto
  
  // Normalizar e multiplicar pelo raio
  const normalizedX = dx / distance;
  const normalizedY = dy / distance;
  
  return {
    x: center.x + normalizedX * radius,
    y: center.y + normalizedY * radius
  };
}

// Função para obter a potência de um path específico
function getPowerForPath(pathId, data) {
  switch(pathId) {
    case 'p-solar-batt': return Math.min(data.solar_production, data.battery_power);
    case 'p-solar-home': return Math.min(data.solar_production, data.house_consumption);
    case 'p-solar-grid': return Math.max(0, data.solar_production - data.house_consumption - data.battery_power);
    case 'p-batt-home': return Math.abs(data.battery_power);
    case 'p-grid-home': return Math.abs(data.grid_power);
    default: return 0;
  }
}

// Função para calcular velocidade baseada na potência
function calculateSpeed(power) {
  // Velocidade base: 3s para 1000W
  // Mais potência = mais rápido (menos tempo)
  // Menos potência = mais lento (mais tempo)
  const basePower = 1000; // 1000W
  const baseSpeed = 3; // 3 segundos
  
  if (power <= 0) return 5; // Muito lento se sem potência
  
  // Velocidade inversamente proporcional à potência
  const speed = (basePower / power) * baseSpeed;
  
  // Limitar entre 0.5s (muito rápido) e 8s (muito lento)
  return Math.max(0.5, Math.min(8, speed));
}

// Dados editáveis para teste
let dadosTeste = {
  solar_production: 3500,
  battery_level: 75,
  battery_power: 2000,
  house_consumption: 1400,
  grid_power: -37,
  timestamp: new Date().toISOString()
};

// Modo de teste - usar dados editáveis
if (window.location.search.includes('teste=true')) {
  // Modificar a função para usar dados editáveis
  const originalAtualizarDados = atualizarDados;
  atualizarDados = function() {
    // Usar dados de teste em vez de fetch
    const data = { ...dadosTeste };
    
    // Atualizar valores dos componentes
    document.getElementById("solar-production").textContent = data.solar_production + " W";
    document.getElementById("battery-level").textContent = data.battery_level + "%";
    document.getElementById("battery-power").textContent = formatBatteryPower(data.battery_power) + " W";
    document.getElementById("house-consumption").textContent = data.house_consumption + " W";
    document.getElementById("grid-power").textContent = formatGridPower(data.grid_power) + " W";
    
    // Atualizar barra de bateria
    const batteryFill = document.getElementById("battery-fill");
    batteryFill.style.width = data.battery_level + "%";
    
    // Atualizar timestamp
    const timestamp = new Date().toLocaleTimeString();
    document.getElementById("timestamp").textContent = timestamp;
    
    // Controlar fluxo de energia visual
    controlarFluxoEnergia(data);
    
    // Atualizar status para mostrar modo teste
    document.getElementById("system-status").textContent = "Modo Teste - Dados Editáveis";
    document.getElementById("system-status").style.color = "#f59e0b";
  };
  
  // Adicionar controles de teste
  setTimeout(() => {
    const container = document.querySelector('.container');
    const controlsDiv = document.createElement('div');
    controlsDiv.innerHTML = `
      <div style="background: rgba(255,255,255,0.1); padding: 1rem; border-radius: 10px; margin-top: 1rem;">
        <h3>🎛️ Controles de Teste</h3>
        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 1rem; margin-top: 1rem;">
          <div>
            <label>Produção Solar (W): <input type="number" id="solar-input" value="${dadosTeste.solar_production}" style="width: 100px; margin-left: 5px;"></label>
          </div>
          <div>
            <label>Nível Bateria (%): <input type="number" id="battery-level-input" value="${dadosTeste.battery_level}" style="width: 100px; margin-left: 5px;"></label>
          </div>
          <div>
            <label>Potência Bateria (W): <input type="number" id="battery-power-input" value="${dadosTeste.battery_power}" style="width: 100px; margin-left: 5px;"></label>
          </div>
          <div>
            <label>Consumo Casa (W): <input type="number" id="house-input" value="${dadosTeste.house_consumption}" style="width: 100px; margin-left: 5px;"></label>
          </div>
          <div>
            <label>Potência Rede (W): <input type="number" id="grid-input" value="${dadosTeste.grid_power}" style="width: 100px; margin-left: 5px;"></label>
          </div>
          <div>
            <button onclick="atualizarDadosTeste()" style="padding: 0.5rem 1rem; background: #22c55e; color: white; border: none; border-radius: 5px; cursor: pointer;">🔄 Atualizar</button>
          </div>
        </div>
      </div>
    `;
    container.appendChild(controlsDiv);
  }, 1000);
}

// Função para atualizar dados de teste
function atualizarDadosTeste() {
  dadosTeste.solar_production = parseInt(document.getElementById('solar-input').value) || 0;
  dadosTeste.battery_level = parseInt(document.getElementById('battery-level-input').value) || 0;
  dadosTeste.battery_power = parseInt(document.getElementById('battery-power-input').value) || 0;
  dadosTeste.house_consumption = parseInt(document.getElementById('house-input').value) || 0;
  dadosTeste.grid_power = parseInt(document.getElementById('grid-input').value) || 0;
  dadosTeste.timestamp = new Date().toISOString();
  atualizarDados();
}

// Redimensionar SVG quando a janela mudar
window.addEventListener('resize', () => {
  setTimeout(atualizarDados, 100);
});

// Aguardar que a página carregue completamente
window.addEventListener('load', () => {
  setTimeout(atualizarDados, 500);
});

setInterval(atualizarDados, 2000);
  