#include "MainMenu.h"

MainMenu::MainMenu(U8G2* display, const char** names, int count)
  : display(display), names(names), count(count),
    selected(0), scrollOffset(0), lastScroll(0), swWasPressed(false), xWasActive(false) {}

void MainMenu::begin() {
  pinMode(MENU_VRY_PIN, INPUT);
  pinMode(MENU_VRX_PIN, INPUT);
  pinMode(MENU_SW_PIN, INPUT_PULLUP);
  selected     = 0;
  scrollOffset = 0;
  swWasPressed = false;
  xWasActive   = false;
  render();
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

int MainMenu::update() {
  unsigned long now = millis();

  // Scroll por eje Y del joystick
  int vry = analogRead(MENU_VRY_PIN);
  int dev = vry - MENU_CENTER;

  if (abs(dev) > MENU_DEADZONE) {
    int absdev   = abs(dev) - MENU_DEADZONE;
    int maxdev   = MENU_CENTER - MENU_DEADZONE;
    int interval = (int)map(absdev, 0, maxdev, MENU_SCROLL_SLOW, MENU_SCROLL_FAST);

    if (now - lastScroll >= (unsigned long)interval) {
      lastScroll = now;
      if (dev > 0) selected = (selected + 1) % count;
      else         selected = (selected - 1 + count) % count;

      // Ajustar ventana para mantener selected visible
      static const int VISIBLE = 3;
      if (selected < scrollOffset)                   scrollOffset = selected;
      if (selected >= scrollOffset + VISIBLE)        scrollOffset = selected - VISIBLE + 1;
      // Wrap-around: si saltó al final volviendo hacia arriba
      if (selected == count - 1 && dev < 0)         scrollOffset = count - VISIBLE;
      if (selected == 0 && dev > 0)                  scrollOffset = 0;
      scrollOffset = constrain(scrollOffset, 0, max(0, count - VISIBLE));

      render();
    }
  }

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
