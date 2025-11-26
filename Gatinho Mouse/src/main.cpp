#include <Arduino.h>
#include <TFT_eSPI.h>
#include <BleMouse.h>
#include <AnimatedGIF.h> 
#include "gif_file.h"     

// ==========================================
// --- CONFIGURAÇÕES DO USUÁRIO ---
// ==========================================
#define GIF_DELAY_MS  100 

// TEMPO MÁXIMO DESCONECTADO (em milissegundos)
// Se ficar 30 segundos sem conectar, ele reinicia para tentar forçar a conexão
#define RESTART_TIMEOUT_MS 30000 

// TAMANHO DO GIF
#define GIF_W 240
#define GIF_H 101

// ==========================================
// --- CONFIGURAÇÕES STEALTH ---
// ==========================================
#define MAX_MOVE_RANGE 15       
#define MOUSE_SPEED_FACTOR 0.05 
#define MIN_WAIT_TIME 2000      
#define MAX_WAIT_TIME 15000     

// Cores (Formato RGB565)
// Opção 1: Azul Bebê (Salmão no código devido a inversão da tela)
#define BT_BLUE_COLOR 0xF651
#define BT_GREY_COLOR 0xCE79   
#define BG_COLOR      TFT_WHITE 

// --- OBJETOS ---
BleMouse bleMouse("Logitech MX Master 3", "Logitech", 100);
TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;

// --- VARIÁVEIS DE CONTROLE ---
int xOffset = 0;
int yOffset = 0;
bool lastConnectionState = false; 
bool firstRun = true;             
unsigned long disconnectStartTime = 0; // Para contar o tempo desconectado

struct MousePhysics {
  float currentX = 0; float currentY = 0;
  float targetX = 0;  float targetY = 0;
  float velocityX = 0; float velocityY = 0;
  float residueX = 0; float residueY = 0;
  bool isMoving = false;
  unsigned long nextActionTime = 0;
} mouse;

// ==========================================
// --- FUNÇÃO: WATCHDOG DE CONEXÃO (NOVA) ---
// ==========================================
void checkConnectionWatchdog() {
  bool isConnected = bleMouse.isConnected();

  if (isConnected) {
    // Se está conectado, zera o cronômetro
    disconnectStartTime = 0;
  } else {
    // Se está desconectado
    if (disconnectStartTime == 0) {
      // Começa a contar agora
      disconnectStartTime = millis();
    } else {
      // Já está contando. Verifica se estourou o tempo.
      if (millis() - disconnectStartTime > RESTART_TIMEOUT_MS) {
        Serial.println("Tempo limite de desconexão atingido. Reiniciando para reconectar...");
        tft.fillScreen(TFT_BLACK); // Apaga a tela para não piscar feio
        delay(100);
        ESP.restart(); // <--- O PULO DO GATO: Reinicia o ESP32 totalmente
      }
    }
  }
}

// ==========================================
// --- DRAW BADGE ---
// ==========================================
void drawBluetoothBadge(bool connected) {
  int w = 26; 
  int h = 26; 
  int r = 8;  
  
  int x = tft.width() - w - 10; 
  int y = 10;

  uint16_t bgColor = connected ? BT_BLUE_COLOR : BT_GREY_COLOR;
  
  tft.fillRect(x, y, w, h, BG_COLOR); 
  tft.fillRoundRect(x, y, w, h, r, bgColor);

  int cx = x + (w/2); 
  int cy = y + (h/2); 
  
  tft.drawLine(cx, cy - 6, cx, cy + 6, TFT_WHITE);
  tft.drawLine(cx, cy - 6, cx + 4, cy - 2, TFT_WHITE); 
  tft.drawLine(cx + 4, cy - 2, cx - 3, cy + 4, TFT_WHITE); 
  tft.drawLine(cx, cy + 6, cx + 4, cy + 2, TFT_WHITE); 
  tft.drawLine(cx + 4, cy + 2, cx - 3, cy - 4, TFT_WHITE); 
}

// ==========================================
// --- MOUSE LOGIC ---
// ==========================================
void humanMouseLogic(unsigned long now) {
  if (!bleMouse.isConnected()) return;

  if (!mouse.isMoving) {
    if (now > mouse.nextActionTime) {
      float angle = random(0, 628) / 100.0;
      float dist = (random(1, 100) + random(1, 100)) / 200.0 * MAX_MOVE_RANGE; 
      mouse.targetX = cos(angle) * dist;
      mouse.targetY = sin(angle) * dist;
      mouse.isMoving = true;
      mouse.nextActionTime = now + random(50, 200); 
    }
  } 
  else {
    float dx = mouse.targetX - mouse.currentX;
    float dy = mouse.targetY - mouse.currentY;
    
    mouse.velocityX = (dx * MOUSE_SPEED_FACTOR);
    mouse.velocityY = (dy * MOUSE_SPEED_FACTOR);

    if (abs(dx) > 2 || abs(dy) > 2) {
       mouse.velocityX += (random(-10, 11) / 50.0);
       mouse.velocityY += (random(-10, 11) / 50.0);
    }

    mouse.currentX += mouse.velocityX;
    mouse.currentY += mouse.velocityY;

    mouse.residueX += mouse.velocityX;
    mouse.residueY += mouse.velocityY;
    int moveX = (int)mouse.residueX;
    int moveY = (int)mouse.residueY;

    if (moveX != 0 || moveY != 0) {
      bleMouse.move(moveX, moveY);
      mouse.residueX -= moveX;
      mouse.residueY -= moveY;
    }

    if (abs(dx) < 0.5 && abs(dy) < 0.5) {
      mouse.isMoving = false;
      mouse.nextActionTime = now + random(MIN_WAIT_TIME, MAX_WAIT_TIME);
    }
  }
}

// ==========================================
// --- GIF DRAW ---
// ==========================================
void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > tft.width()) iWidth = tft.width();

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; 
  s = pDraw->pPixels;
  
  if (pDraw->ucDisposalMethod == 2) { 
    pDraw->ucHasTransparency = 1; 
    pDraw->ucTransparent = 255;
  }
  
  if (pDraw->ucHasTransparency) { 
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + iWidth;
    x = 0; iCount = 0;
    while(s < pEnd) {
      c = *s++;
      if (c == ucTransparent) {
        if (iCount > 0) {
          tft.pushImage(pDraw->iX + x + xOffset, y + yOffset, iCount, 1, usTemp);
          x += iCount; iCount = 0;
        }
        x++;
      } else {
        usTemp[iCount++] = usPalette[c];
      }
    }
    if (iCount > 0) tft.pushImage(pDraw->iX + x + xOffset, y + yOffset, iCount, 1, usTemp);
  } else { 
    s = pDraw->pPixels;
    for (x=0; x<iWidth; x++) usTemp[x] = usPalette[*s++];
    tft.pushImage(pDraw->iX + xOffset, y + yOffset, iWidth, 1, usTemp);
  }
}

// ==========================================
// --- SETUP ---
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // --- CONFIGURAÇÃO DE BRILHO CORRETA PARA S3 (PINO 38) ---
  pinMode(38, OUTPUT);
  analogWrite(38, 50); // Brilho suave (0-255)
  
  tft.init();
  tft.setRotation(1); 
  tft.fillScreen(BG_COLOR);
  tft.setSwapBytes(true); 

  xOffset = (tft.width() - GIF_W) / 2;
  yOffset = (tft.height() - GIF_H) / 2;

  gif.begin(LITTLE_ENDIAN_PIXELS);
  
  Serial.println("Iniciando Mouse Stealth S3...");
  bleMouse.begin();
  
  randomSeed(analogRead(1) + micros());
  
  drawBluetoothBadge(false);
}

// ==========================================
// --- LOOP ---
// ==========================================
void loop() {
  // 1. WATCHDOG: Verifica se precisa reiniciar para reconectar
  checkConnectionWatchdog();

  // 2. STATUS DO ÍCONE
  bool isConnected = bleMouse.isConnected();
  if (isConnected != lastConnectionState || firstRun) {
    drawBluetoothBadge(isConnected);
    lastConnectionState = isConnected;
    firstRun = false;
  }

  // 3. ANIMAÇÃO E MOUSE
  if (gif.open((uint8_t *)bongo_gif, sizeof(bongo_gif), GIFDraw)) {
    while (gif.playFrame(false, NULL)) {
       unsigned long startWait = millis();
       while (millis() - startWait < GIF_DELAY_MS) {
          humanMouseLogic(millis());
          // Precisamos verificar o Watchdog dentro do loop do GIF também
          // para garantir que ele conte o tempo mesmo durante a animação
          checkConnectionWatchdog(); 
          delay(2); 
       }
    }
    gif.close();
  }
}