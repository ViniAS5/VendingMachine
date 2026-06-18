/*
 * PROJETO MAQUINA DE VENDAS
 * O ESP32 cria uma rede WiFi, hospeda a pagina de pagamento e aciona a FPGA. 
 * Por: Vinícius Abreu e Breno Porcíncula
 */

#include <SPI.h>
#include <FS.h>
using namespace fs;
#include <TFT_eSPI.h> 
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <WebServer.h>
#include <qrcoderm.h>

TFT_eSPI tft = TFT_eSPI();

const char* ssid = "Maquina de Vendas";
const char* password = "";
WebServer server(80);

#define PINO_BIT_0 22
#define PINO_BIT_1 27

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass touchSpi = SPIClass(2);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

#define COR_FUNDO TFT_BLACK

struct Botao {
  int x, y, w, h;
  uint16_t corFundo, corTexto;
  String txt;
  byte codigoFPGA; 
};

Botao botoes[3] = {
  {10, 50,  300, 50, TFT_RED, TFT_BLACK, "Coca-Cola", 1},
  {10, 110, 300, 50, tft.color565(0, 180, 0), TFT_WHITE, "Guarana", 2},
  {10, 170, 300, 50, TFT_WHITE, tft.color565(0, 120, 0), "Sprite", 3}
};

bool aguardandoPagamento = false;
bool pagamentoRealizado = false;
Botao produtoPendente;

void handleRoot() {
  if (aguardandoPagamento) {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f9; padding: 30px 20px;}";
    html += "h1{color: #333;} .box{background: white; padding: 20px; border-radius: 15px; box-shadow: 0px 4px 10px rgba(0,0,0,0.1); display: inline-block; width: 90%; max-width: 400px;}";
    html += "button{background-color: #32CD32; color: white; border: none; padding: 15px; font-size: 18px; font-weight: bold; border-radius: 10px; cursor: pointer; width: 100%; margin-top: 20px;}";
    html += "button:active{background-color: #28a428;} .footer{margin-top: 40px; font-size: 14px; color: #777;}</style></head><body>";
    
    // Logo do IFSC
    html += "<svg viewBox='0 0 66 90' width='55' height='75' xmlns='http://www.w3.org/2000/svg' style='margin-bottom:15px;'><g fill='#2f9e41'><rect x='0' y='24' width='18' height='18' rx='2'/><rect x='0' y='48' width='18' height='18' rx='2'/><rect x='0' y='72' width='18' height='18' rx='2'/><rect x='24' y='0' width='18' height='18' rx='2'/><rect x='24' y='24' width='18' height='18' rx='2'/><rect x='24' y='48' width='18' height='18' rx='2'/><rect x='24' y='72' width='18' height='18' rx='2'/><rect x='48' y='0' width='18' height='18' rx='2'/><rect x='48' y='48' width='18' height='18' rx='2'/></g><circle cx='9' cy='9' r='9' fill='#cd191e'/></svg>";
    
    html += "<div class='box'><h1>Simulador PIX</h1><p>Você está comprando:</p><h2 style='color:#cd191e;'>" + produtoPendente.txt + "</h2>";
    html += "<form action='/pagar' method='POST'><button type='submit'>SIMULAR PAGAMENTO</button></form></div>";
    
    html += "<div class='footer'>Por: Vinícius Abreu e Breno Porcíncula<br>Eletrônica Digital 2 2026.1</div>";
    
    html += "</body></html>";
    server.send(200, "text/html", html);
  } else {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'></head>";
    html += "<body style='text-align:center; font-family:Arial; padding: 30px 20px; background-color: #f4f4f9;'>";
    
    html += "<svg viewBox='0 0 66 90' width='55' height='75' xmlns='http://www.w3.org/2000/svg' style='margin-bottom:15px;'><g fill='#2f9e41'><rect x='0' y='24' width='18' height='18' rx='2'/><rect x='0' y='48' width='18' height='18' rx='2'/><rect x='0' y='72' width='18' height='18' rx='2'/><rect x='24' y='0' width='18' height='18' rx='2'/><rect x='24' y='24' width='18' height='18' rx='2'/><rect x='24' y='48' width='18' height='18' rx='2'/><rect x='24' y='72' width='18' height='18' rx='2'/><rect x='48' y='0' width='18' height='18' rx='2'/><rect x='48' y='48' width='18' height='18' rx='2'/></g><circle cx='9' cy='9' r='9' fill='#cd191e'/></svg>";
    
    html += "<h2 style='color: #333; margin-top: 20px; max-width: 400px; display: inline-block; line-height: 1.4;'>Escolha um produto na tela da máquina para começar</h2>";
    
    html += "<div style='margin-top:40px; font-size:14px; color:#777;'>Por: Vinícius Abreu e Breno Porcíncula<br>Eletrônica Digital 2 2026.1</div>";
    
    html += "</body></html>";
    server.send(200, "text/html", html);
  }
}
void handlePagar() {
  pagamentoRealizado = true; 
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f9; padding: 30px 20px;}";
  html += ".box{background: white; padding: 30px; border-radius: 15px; box-shadow: 0px 4px 10px rgba(0,0,0,0.1); display: inline-block; border-top: 10px solid #32CD32;}";
  html += ".footer{margin-top: 40px; font-size: 14px; color: #777;}</style></head><body>";
  
  html += "<svg viewBox='0 0 66 90' width='55' height='75' xmlns='http://www.w3.org/2000/svg' style='margin-bottom:15px;'><g fill='#2f9e41'><rect x='0' y='24' width='18' height='18' rx='2'/><rect x='0' y='48' width='18' height='18' rx='2'/><rect x='0' y='72' width='18' height='18' rx='2'/><rect x='24' y='0' width='18' height='18' rx='2'/><rect x='24' y='24' width='18' height='18' rx='2'/><rect x='24' y='48' width='18' height='18' rx='2'/><rect x='24' y='72' width='18' height='18' rx='2'/><rect x='48' y='0' width='18' height='18' rx='2'/><rect x='48' y='48' width='18' height='18' rx='2'/></g><circle cx='9' cy='9' r='9' fill='#cd191e'/></svg>";
  
  html += "<div class='box'><h1 style='color: #32CD32;'>Pagamento Aprovado!</h1><p>O seu produto já está sendo liberado.</p></div>";
  
  html += "<div class='footer'>Por: Vinícius Abreu e Breno Porcíncula<br>Eletrônica Digital 2 2026.1</div>";
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void desenharMenu();
void gerarTelaQRCode(Botao b);
void executarLiberacao(Botao b);

void setup() {
  Serial.begin(115200);
  
  pinMode(PINO_BIT_0, OUTPUT); 
  pinMode(PINO_BIT_1, OUTPUT); 
  digitalWrite(PINO_BIT_0, LOW);
  digitalWrite(PINO_BIT_1, LOW);

  tft.init(); tft.setRotation(1); tft.invertDisplay(false); 
  touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSpi); ts.setRotation(1); 

  tft.fillScreen(COR_FUNDO);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
  tft.drawString("Iniciando Roteador...", 160, 120);
  
  WiFi.softAP(ssid, password);
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/pagar", HTTP_POST, handlePagar);
  server.begin();

  delay(1000);
  desenharMenu();
}

void loop() {
  server.handleClient();

  if (pagamentoRealizado && aguardandoPagamento) {
    executarLiberacao(produtoPendente);
    pagamentoRealizado = false;
    aguardandoPagamento = false;
  }

  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int t_x = map(p.x, 200, 3700, 0, 320);
    int t_y = map(p.y, 240, 3800, 0, 240);

    if (aguardandoPagamento) {
      aguardandoPagamento = false;
      pagamentoRealizado = false;
      desenharMenu();
      while(ts.touched()) { delay(10); } 
      return;
    }

    for (int i = 0; i < 3; i++) {
      if (t_x > botoes[i].x && t_x < (botoes[i].x + botoes[i].w) &&
          t_y > botoes[i].y && t_y < (botoes[i].y + botoes[i].h)) {
        produtoPendente = botoes[i];
        aguardandoPagamento = true;
        pagamentoRealizado = false;
        gerarTelaQRCode(produtoPendente);
        while(ts.touched()) { delay(10); } 
        break; 
      }
    }
  }
}

void executarLiberacao(Botao b) {
  tft.fillScreen(COR_FUNDO); 
  
  tft.setTextColor(TFT_GREEN); tft.setTextSize(3); tft.setTextDatum(MC_DATUM);
  tft.drawString("PAGAMENTO ACEITO!", 160, 80);
  
  tft.setTextColor(TFT_WHITE); 
  tft.drawString("Liberando:", 160, 130);
  tft.drawString(b.txt, 160, 170); 
  
  // Envio dos dados para a FPGA
  digitalWrite(PINO_BIT_0, bitRead(b.codigoFPGA, 0)); 
  digitalWrite(PINO_BIT_1, bitRead(b.codigoFPGA, 1)); 
  delay(300); 
  
  digitalWrite(PINO_BIT_0, LOW); 
  digitalWrite(PINO_BIT_1, LOW); 
  delay(2500); 
  desenharMenu(); 
}

void gerarTelaQRCode(Botao b) {
  tft.fillScreen(TFT_WHITE); 
  tft.setTextColor(TFT_BLACK); 
  tft.setTextSize(2); 
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Pague com PIX:", 160, 10);
  tft.drawString(b.txt, 160, 35);
  tft.setTextSize(1); 
  tft.drawString("Conecte no WiFi 'Maquina de Vendas'", 160, 210);
  tft.drawString("(Toque na tela para cancelar)", 160, 225);

  String urlCompleta = "http://192.168.4.1/";
  
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, urlCompleta.c_str());

  int mult = 4; 
  int offX = (320 - (qrcode.size * mult)) / 2;
  int offY = (240 - (qrcode.size * mult)) / 2 + 10;

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(offX + (x * mult), offY + (y * mult), mult, mult, TFT_BLACK);
      }
    }
  }
}

void desenharMenu() {
  tft.fillScreen(COR_FUNDO); 
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setTextDatum(TC_DATUM); 
  tft.drawString("SELECIONE O PRODUTO", 160, 10);
  for (int i = 0; i < 3; i++) {
    tft.fillRoundRect(botoes[i].x, botoes[i].y, botoes[i].w, botoes[i].h, 10, botoes[i].corFundo); 
    tft.drawRoundRect(botoes[i].x, botoes[i].y, botoes[i].w, botoes[i].h, 10, TFT_WHITE); 
    tft.setTextColor(botoes[i].corTexto); tft.setTextDatum(MC_DATUM); 
    tft.drawString(botoes[i].txt, botoes[i].x + (botoes[i].w / 2), botoes[i].y + (botoes[i].h / 2));
  }
}
