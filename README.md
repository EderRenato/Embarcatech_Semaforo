# 🚦 Projeto: Sistema Semafórico Inteligente com Modo Noturno e Acessibilidade

[![Vídeo de Demonstração](https://img.shields.io/badge/Assista%20no-Google%20Drive-red?logo=google-drive)](https://drive.google.com/file/d/1GRbC_o-FtxxcE8-dp9RSQJp5--0AmHjj/view?usp=drive_link)

## 📋 Sumário
- [🎯 Objetivo Geral](#🎯-objetivo-geral)
- [⚙️ Modos de Operação](#⚙️-modos-de-operação)
- [🧠 Lógica do Sistema](#🧠-lógica-do-sistema)
- [🔌 Uso dos Periféricos](#🔌-uso-dos-periféricos)
- [🛠️ Tecnologias e Ferramentas](#🛠️-tecnologias-e-ferramentas)
- [👨‍💻 Autoria](#👨‍💻-autoria)

---

## 🎯 Objetivo Geral

Desenvolver um **sistema semafórico inteligente** baseado em microcontrolador (BitDogLab + FreeRTOS), capaz de:

- Operar autonomamente em diferentes condições de luminosidade;
- Alternar para modo noturno visando economia de energia e redução da poluição luminosa;
- Oferecer **recursos de acessibilidade** (sinalização sonora e visual adaptada) para pedestres com deficiências visuais ou auditivas.

---

## ⚙️ Modos de Operação

### 1. 🌞 Modo Diurno (Operação Normal)
- Ciclos de semáforo tradicionais (vermelho, amarelo, verde).
- Temporizações controladas por FSM com timers via FreeRTOS.
- LEDs operando em brilho total para visibilidade diurna.

### 2. 🌙 Modo Noturno (Ativado por Botão A)
- Ativa sinalização intermitente com LED amarelo piscante.
- Redução do consumo energético e da poluição visual.
- Comportamento comum em cruzamentos de baixo tráfego durante a madrugada.

### 3. ♿ Acessibilidade
- Emissão de sinal sonoro pelo buzzer durante a travessia segura.
- Exibição de mensagens ou ícones acessíveis no display OLED.
- Integração com matriz de LEDs WS2812B para reforço visual.

---

## 🧠 Lógica do Sistema

- Cada funcionalidade é encapsulada em uma **task do FreeRTOS**.
- Controle central baseado em **máquina de estados finita (FSM)**.
- Alternância de modos via leitura digital periódica dos botões.
- Tarefas principais:
  - `vSemaforoTask()`: ciclo de sinalização dos LEDs.
  - `vUpdateDisplayTask()`: exibição de mensagens no OLED.
  - `vUpdateMatrixTask()`: feedback visual complementar com matriz WS2812B.

---

## 🔌 Uso dos Periféricos

### GPIOs Digitais
- Controle dos LEDs do semáforo (veículos e pedestres).
- Leitura dos botões de modo (noturno e acessibilidade).

### I2C
- Comunicação com display OLED SSD1306 para exibição de mensagens.

### PWM
- Controle do **buzzer** (sinal sonoro com frequência variável).

### Temporizadores (FreeRTOS)
- Utilização de `vTaskDelay` para garantir tarefas assíncronas e não-bloqueantes.

### Matriz de LEDs WS2812B
- Utilizada para reforço visual dos estados do semáforo e acessibilidade.

---

## 🛠️ Tecnologias e Ferramentas

- **Placa:** BitDogLab
- **Sistema Operacional:** FreeRTOS
- **Linguagem:** C/C++ com Pico SDK
- **Componentes:**
  - Display OLED SSD1306 (I2C)
  - Buzzer (PWM)
  - LEDs RGB WS2812B
  - Botões táteis para modos

---

## 👨‍💻 Autoria

Projeto desenvolvido por **Eder Renato**, como parte de estudos e aplicações práticas em **sistemas embarcados**, **acessibilidade urbana** e **automação semafórica inteligente**, integrando conhecimentos de engenharia elétrica com programação em tempo real.

---

Este projeto representa um passo em direção a sistemas urbanos mais inclusivos, eficientes e tecnologicamente avançados. Sua arquitetura modular permite fácil expansão para recursos de IoT, sensores de tráfego ou comunicação intercruzamentos.
