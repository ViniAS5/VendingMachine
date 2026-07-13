# Máquina de Vendas - Integração FPGA e ESP32 (CYD)

Projeto de uma máquina de vendas automatizada para 3 tipos de refrigerantes em lata. O sistema demonstra a integração entre hardware dedicado e Internet das Coisas (IoT), utilizando uma FPGA para controle de precisão e um ESP32 como interface de usuário e servidor web local.

Projeto final desenvolvido para a disciplina de **Eletrônica Digital 2** (2026.1) do curso de **Engenharia Eletrônica** do **IFSC - Câmpus Florianópolis**.

## Hardware Utilizado
* **FPGA:** Terasic DE10-Lite (Controle lógico e acionamento PWM).
* **Microcontrolador:** ESP32 CYD - Cheap Yellow Display com tela touch de 2.8" (Interface Gráfica e Servidor Web).
* **Atuadores:** 3x Servo Motores (Para liberação física das latas).

## Arquitetura e Funcionamento

O fluxo de funcionamento do projeto une a interface física com uma simulação de pagamento via PIX:

1. **Acesso:** O ESP32 cria um ponto de acesso Wi-Fi local chamado "Máquina de Vendas".
2. **Interface Gráfica:** Pela tela touch do ESP32, o usuário seleciona o refrigerante desejado (Coca-Cola, Guaraná ou Sprite).
3. **Simulação de Pagamento:** Ao selecionar o produto, a tela exibe um QR Code. O usuário, conectado à rede Wi-Fi da máquina, escaneia o código com o celular e é direcionado para uma página web hospedada localmente no ESP32, que simula um aplicativo bancário.
4. **Comunicação 2 Fios:** Ao clicar em "PAGAR" no celular, o ESP32 envia um pulso de 300ms contendo um código binário de 2 bits para a FPGA.
5. **Acionamento:** A FPGA recebe o comando, realiza a estabilização do sinal (debounce) e altera o PWM da saída correspondente.
    * O sinal PWM opera em 50Hz (período de 20ms).
    * O ciclo do motor intercala larguras de pulso de **1.5ms** (repouso no centro), **0.8ms** (giro para soltar a lata) e **2.5ms** (retorno para engatilhar o próximo produto).

## Pinagem e Conexões

A comunicação entre o ESP32 e a FPGA é feita por um protocolo paralelo de 2 fios para enviar os comandos binários. 

**Atenção:** É obrigatório que os pinos **GND** do ESP32, da FPGA e da fonte de alimentação dos servo motores estejam **interconectados** para garantir a mesma referência de tensão.

| Componente de Origem | Pino / Porta | Destino | Função |
| :--- | :--- | :--- | :--- |
| **ESP32 (CYD)** | Pino 27 | FPGA (IO4) | Envio do Bit mais significativo (MSB) |
| **ESP32 (CYD)** | Pino 22 | FPGA (IO3) | Envio do Bit menos significativo (LSB) |
| **FPGA** | IO6 | Servo 1 | Sinal PWM - Libera Coca-Cola |
| **FPGA** | IO5 | Servo 2 | Sinal PWM - Libera Guaraná |
| **FPGA** | IO9 | Servo 3 | Sinal PWM - Libera Sprite |

## Desenvolvedores
* **Vinícius Abreu**
* **Breno Porcíncula**
