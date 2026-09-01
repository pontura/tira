#include "MainMenu.h"
#include <math.h>
#include <NonBlockingRtttl.h>  // rtttl::tone/noTone: mismo canal LEDC que usa SoundManager

MainMenu::MainMenu(U8G2* display, const char** names, int count, CRGB* leds, int numLeds)
  : display(display), names(names), count(count),
    selected(0), scrollOffset(0), lastScroll(0), swWasPressed(false), xWasActive(false),
    leds(leds), numLeds(numLeds),
    ledSegAnimFrom(0), ledSegAnimTo(0), ledAnimStart(0), lastLedUpdate(0),
    buzzerOffAt(0) {}

void MainMenu::begin() {
  pinMode(MENU_VRY_PIN, INPUT);
  pinMode(MENU_VRX_PIN, INPUT);
  pinMode(MENU_SW_PIN, INPUT_PULLUP);
  selected     = 0;
  scrollOffset = 0;
  swWasPressed = false;
  xWasActive   = false;
  buzzerOffAt  = 0;

  // Arranca directo en la posición del juego 0, sin animar.
  float start        = segmentStart(selected);
  ledSegAnimFrom      = start;
  ledSegAnimTo        = start;
  ledAnimStart        = millis();
  lastLedUpdate       = 0;

  render();
  updateLeds();
}

extern bool displayOK;

void MainMenu::render() {
  if (!displayOK) return;
  display->clearBuffer();

  // Título
  display->setFont(u8g2_font_5x7_tr);
  display->drawStr(2, 8, "TIRA GAMES");
  display->drawHLine(0, 11, 128);

  // Lista de juegos (ventana de 3 items visibles)
  static const int VISIBLE = 3;
  display->setFont(u8g2_font_7x14B_tr);
  for (int vi = 0; vi < VISIBLE && (scrollOffset + vi) < count; vi++) {
    int i = scrollOffset + vi;
    int y = 27 + vi * 16;
    if (i == selected) {
      display->setDrawColor(1);
      display->drawBox(0, y - 14, 128, 16);
      display->setDrawColor(0);
      display->drawStr(4, y, names[i]);
      display->drawStr(118, y, ">");
      display->setDrawColor(1);
    } else {
      display->drawStr(4, y, names[i]);
    }
  }

  // Indicadores de scroll (triángulos arriba/abajo)
  if (scrollOffset > 0)
    display->drawStr(122, 20, "^");
  if (scrollOffset + VISIBLE < count)
    display->drawStr(122, 59, "v");

  display->sendBuffer();
}

// LED donde arranca el segmento del juego `index` (0=arriba de la tira),
// descendiendo a medida que crece el índice. El último segmento absorbe
// el resto si numLeds no es múltiplo exacto de count.
float MainMenu::segmentStart(int index) const {
  if (count <= 0) return 0.0f;
  float sliceSize = (float)numLeds / (float)count;
  if (index >= count - 1) return 0.0f;
  return (float)numLeds - (float)(index + 1) * sliceSize;
}

void MainMenu::updateLeds() {
  if (leds == nullptr || numLeds <= 0 || count <= 0) return;

  unsigned long now = millis();
  if (now - lastLedUpdate < MENU_LED_UPDATE_MS) return;
  lastLedUpdate = now;

  float progress = constrain((float)(now - ledAnimStart) / (float)MENU_LED_ANIM_MS, 0.0f, 1.0f);
  float eased    = progress * progress * (3.0f - 2.0f * progress);  // ease-in-out (smoothstep)
  float start    = ledSegAnimFrom + (ledSegAnimTo - ledSegAnimFrom) * eased;

  float sliceSize = (float)numLeds / (float)count;
  int   lo        = constrain((int)roundf(start), 0, numLeds - 1);
  int   hi        = constrain((int)roundf(start + sliceSize) - 1, 0, numLeds - 1);

  FastLED.clear();
  for (int i = lo; i <= hi; i++) leds[i] = CRGB::White;
  FastLED.show();
}

int MainMenu::update() {
  unsigned long now = millis();

  // Corta el beep de cambio de juego manualmente. Usa rtttl::tone/noTone (no
  // el tone()/noTone() global) para no pelearse con el canal LEDC que ya
  // reclamó la librería RTTTL — ver NonBlockingRtttl.h.
  if (buzzerOffAt && now >= buzzerOffAt) { rtttl::noTone(BUZZER_PIN); buzzerOffAt = 0; }

  // Scroll por eje Y del joystick
  int vry = analogRead(MENU_VRY_PIN);
  int dev = vry - MENU_CENTER;

  if (abs(dev) > MENU_DEADZONE) {
    int absdev   = abs(dev) - MENU_DEADZONE;
    int maxdev   = MENU_CENTER - MENU_DEADZONE;
    int interval = (int)map(absdev, 0, maxdev, MENU_SCROLL_SLOW, MENU_SCROLL_FAST);

    if (now - lastScroll >= (unsigned long)interval) {
      lastScroll = now;
      int prevSelected = selected;
      // Sin wrap-around: el primer y el último juego son los extremos del scroll.
      if (dev > 0) selected = min(selected + 1, count - 1);
      else         selected = max(selected - 1, 0);

      if (selected != prevSelected) {
        // Beep bien cortito y grave al pasar de un juego a otro
        rtttl::tone(BUZZER_PIN, MENU_SELECT_BEEP_FREQ);
        buzzerOffAt = now + MENU_SELECT_BEEP_MS;

        // Ajustar ventana para mantener selected visible
        static const int VISIBLE = 3;
        if (selected < scrollOffset)            scrollOffset = selected;
        if (selected >= scrollOffset + VISIBLE) scrollOffset = selected - VISIBLE + 1;
        scrollOffset = constrain(scrollOffset, 0, max(0, count - VISIBLE));

        render();

        // Nueva animación de desplazamiento de LEDs hacia el segmento del juego seleccionado
        unsigned long ledNow = millis();
        float animProgress = constrain((float)(ledNow - ledAnimStart) / (float)MENU_LED_ANIM_MS, 0.0f, 1.0f);
        float animEased    = animProgress * animProgress * (3.0f - 2.0f * animProgress);
        ledSegAnimFrom      = ledSegAnimFrom + (ledSegAnimTo - ledSegAnimFrom) * animEased;
        ledSegAnimTo        = segmentStart(selected);
        ledAnimStart        = ledNow;
      }
    }
  }

  updateLeds();

  // Joystick derecha (VRX) → entrar al juego seleccionado
  int vrx     = analogRead(MENU_VRX_PIN);
  bool xRight = (vrx - MENU_CENTER) > MENU_DEADZONE;
  if (xRight && !xWasActive) {
    xWasActive = true;
    return selected;
  }
  if (!xRight) xWasActive = false;

  return -1;
}
