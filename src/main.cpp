#include <Arduino.h>
#include <TFT_eSPI.h>
#include "panda_idle_grid.h"
#include "panda_sleep_bitmap.h"

namespace {
EPaper display;

constexpr int PANDA_SCALE_PERCENT = 70;
constexpr int PANDA_WIDTH = 341;
constexpr int PANDA_HEIGHT = 363;
constexpr int GROUND_MARGIN = 10;
constexpr int BLANK_SCREEN_DELAY_MS = 2500;
constexpr int TITLE_SCREEN_DELAY_MS = 2000;
constexpr int REVEAL_FRAME_DELAY_MS = 180;
constexpr int BLINK_BEFORE_DELAY_MS = 500;
constexpr int BLINK_CLOSED_DELAY_MS = 350;
constexpr int SLEEP_FRAME_DELAY_MS = 550;
constexpr int WAKE_EYES_OPEN_DELAY_MS = 700;
constexpr int BUTTON_DEBOUNCE_MS = 40;
constexpr int TITLE_TEXT_SIZE = 8;
constexpr const char* TITLE = "Phubbi";
constexpr int SLEEP_BUTTON_PINS[] = {2, 3, 5};

// Each value is the number of bitmap rows visible above and below the centre.
// The final frame reveals the complete panda.
constexpr int REVEAL_HALF_ROWS[] = {2, 5, 9, 13, PANDA_IDLE_ROWS};

bool sleepingPandaShown = false;
bool sleepButtonLatched = false;

struct Rect {
  int x;
  int y;
  int width;
  int height;
};

int scaled(int value) {
  return value * PANDA_SCALE_PERCENT / 100;
}

Rect pandaBounds() {
  const int width = scaled(PANDA_WIDTH);
  const int height = scaled(PANDA_HEIGHT);
  const int x = display.width() / 2 - width / 2;
  const int y = display.height() - GROUND_MARGIN - height;
  return {x, y, width, height};
}

Rect pandaGridRect(const Rect& bounds, int firstColumn, int firstRow,
                   int lastColumn, int lastRow) {
  const int left = bounds.x + firstColumn * bounds.width / PANDA_IDLE_COLUMNS;
  const int top = bounds.y + firstRow * bounds.height / PANDA_IDLE_ROWS;
  const int right = bounds.x + lastColumn * bounds.width / PANDA_IDLE_COLUMNS;
  const int bottom = bounds.y + lastRow * bounds.height / PANDA_IDLE_ROWS;
  return {left, top, right - left, bottom - top};
}

void drawPandaRows(const Rect& bounds, int firstRow, int lastRow) {
  firstRow = max(0, firstRow);
  lastRow = min(PANDA_IDLE_ROWS, lastRow);

  for (int row = firstRow; row < lastRow; ++row) {
    const int top = bounds.y + row * bounds.height / PANDA_IDLE_ROWS;
    const int bottom = bounds.y + (row + 1) * bounds.height / PANDA_IDLE_ROWS;
    int runStart = -1;

    // Draw every connected horizontal run as one rectangle.
    for (int col = 0; col <= PANDA_IDLE_COLUMNS; ++col) {
      const bool black = col < PANDA_IDLE_COLUMNS &&
                         PANDA_IDLE[row][col] == '1';

      if (black && runStart == -1) {
        runStart = col;
      } else if (!black && runStart != -1) {
        const int left = bounds.x + runStart * bounds.width / PANDA_IDLE_COLUMNS;
        const int right = bounds.x + col * bounds.width / PANDA_IDLE_COLUMNS;
        display.fillRect(left, top, right - left, bottom - top, TFT_BLACK);
        runStart = -1;
      }
    }
  }
}

void refresh(const Rect& area) {
#ifdef USE_PARTIAL_EPAPER
  display.updataPartial(area.x, area.y, area.width, area.height);
#else
  display.update();
#endif
}

void showTitleCard() {
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  display.setTextSize(TITLE_TEXT_SIZE);

  const int titleWidth = display.textWidth(TITLE);
  const int titleHeight = display.fontHeight();
  display.setCursor((display.width() - titleWidth) / 2,
                    (display.height() - titleHeight) / 2);
  display.print(TITLE);
  display.update();
  delay(TITLE_SCREEN_DELAY_MS);
}

void drawThickLine(int x1, int y1, int x2, int y2, int thickness,
                   uint16_t colour) {
  const int offset = thickness / 2;
  for (int i = -offset; i <= offset; ++i) {
    display.drawLine(x1, y1 + i, x2, y2 + i, colour);
  }
}

void blinkOnce(const Rect& bounds) {
  constexpr int eyeFirstRow = 7;
  constexpr int eyeLastRow = 12;
  const Rect leftEye = pandaGridRect(bounds, 7, eyeFirstRow, 11, eyeLastRow);
  const Rect rightEye = pandaGridRect(bounds, 15, eyeFirstRow, 21, eyeLastRow);
  const Rect eyeArea = pandaGridRect(bounds, 7, eyeFirstRow, 21, eyeLastRow);
  const Rect leftPupil = pandaGridRect(bounds, 9, 9, 10, 11);
  const Rect rightPupil = pandaGridRect(bounds, 16, 9, 18, 11);

  delay(BLINK_BEFORE_DELAY_MS);

  // Keep the panda's rounded black eye patches. Cover only the pupils, then
  // cut a soft white eyelid curve into each patch for a gentler blink.
  display.fillRect(leftPupil.x, leftPupil.y, leftPupil.width,
                   leftPupil.height, TFT_BLACK);
  display.fillRect(rightPupil.x, rightPupil.y, rightPupil.width,
                   rightPupil.height, TFT_BLACK);

  const int leftMidY = leftEye.y + leftEye.height / 2;
  const int rightMidY = rightEye.y + rightEye.height / 2;
  drawThickLine(leftEye.x + 4, leftMidY,
                leftEye.x + leftEye.width / 2, leftMidY + 4, 2, TFT_WHITE);
  drawThickLine(leftEye.x + leftEye.width / 2, leftMidY + 4,
                leftEye.x + leftEye.width - 4, leftMidY, 2, TFT_WHITE);
  drawThickLine(rightEye.x + 4, rightMidY,
                rightEye.x + rightEye.width / 2, rightMidY + 4, 2, TFT_WHITE);
  drawThickLine(rightEye.x + rightEye.width / 2, rightMidY + 4,
                rightEye.x + rightEye.width - 4, rightMidY, 2, TFT_WHITE);
  refresh(eyeArea);
  delay(BLINK_CLOSED_DELAY_MS);

  // Restore the original eye pixels, leaving the complete panda as the final
  // permanent e-paper image.
  display.fillRect(leftEye.x, leftEye.y, leftEye.width, leftEye.height, TFT_WHITE);
  display.fillRect(rightEye.x, rightEye.y, rightEye.width, rightEye.height,
                   TFT_WHITE);
  drawPandaRows(bounds, eyeFirstRow, eyeLastRow);
  refresh(eyeArea);
}

void drawSleepingPanda() {
  const int x = (display.width() - PANDA_SLEEP_WIDTH) / 2;
  const int y = display.height() - GROUND_MARGIN - PANDA_SLEEP_HEIGHT;
  display.drawBitmap(x, y, PANDA_SLEEP_BITMAP,
                     PANDA_SLEEP_WIDTH, PANDA_SLEEP_HEIGHT, TFT_BLACK);
}

void drawIdlePanda() {
  const Rect bounds = pandaBounds();
  display.fillScreen(TFT_WHITE);
  drawPandaRows(bounds, 0, PANDA_IDLE_ROWS);
  display.update();
}

Rect drawPixelZ(int x, int y, int cellSize) {
  const int width = 5 * cellSize;
  const int height = 6 * cellSize;
  display.fillRect(x, y, width, cellSize, TFT_BLACK);
  for (int step = 0; step < 4; ++step) {
    display.fillRect(x + (3 - step) * cellSize,
                     y + (step + 1) * cellSize,
                     2 * cellSize, cellSize, TFT_BLACK);
  }
  display.fillRect(x, y + 5 * cellSize, width, cellSize, TFT_BLACK);
  return {x, y, width, height};
}

void showSleepingPanda() {
  display.fillScreen(TFT_WHITE);
  drawSleepingPanda();
  display.update();
  delay(SLEEP_FRAME_DELAY_MS);

  // The Zs accumulate one at a time and the third frame remains permanently.
  Rect zArea = drawPixelZ(270, 306, 2);
  refresh(zArea);
  delay(SLEEP_FRAME_DELAY_MS);

  zArea = drawPixelZ(246, 278, 3);
  refresh(zArea);
  delay(SLEEP_FRAME_DELAY_MS);

  zArea = drawPixelZ(216, 243, 4);
  refresh(zArea);
}

void drawIdleStyleSleepingEyes(int pandaX, int pandaY) {
  constexpr int cellSize = 7;
  constexpr int eyeRows = 5;
  constexpr int leftFirstColumn = 7;
  constexpr int leftLastColumn = 12;
  constexpr int rightFirstColumn = 15;
  constexpr int rightLastColumn = 22;
  constexpr int sourceFirstColumn = leftFirstColumn;
  constexpr int eyesX = 52;
  constexpr int eyesY = 77;

  // Copy the eye pixels directly from the idle panda's face. The untouched
  // white cells become the same small pupil gaps used by the idle artwork.
  for (int row = 0; row < eyeRows; ++row) {
    for (int col = leftFirstColumn; col < leftLastColumn; ++col) {
      if (PANDA_IDLE[7 + row][col] == '1') {
        display.fillRect(pandaX + eyesX +
                             (col - sourceFirstColumn) * cellSize,
                         pandaY + eyesY + row * cellSize,
                         cellSize, cellSize, TFT_BLACK);
      }
    }

    for (int col = rightFirstColumn; col < rightLastColumn; ++col) {
      if (PANDA_IDLE[7 + row][col] == '1') {
        display.fillRect(pandaX + eyesX +
                             (col - sourceFirstColumn) * cellSize,
                         pandaY + eyesY + row * cellSize,
                         cellSize, cellSize, TFT_BLACK);
      }
    }
  }
}

void wakeSleepingPanda() {
  const int pandaX = (display.width() - PANDA_SLEEP_WIDTH) / 2;
  const int pandaY = display.height() - GROUND_MARGIN - PANDA_SLEEP_HEIGHT;
  const Rect wakeArea = {216, 243, pandaX + 159 - 216, pandaY + 117 - 243};

  // Remove the three sleep symbols as the panda wakes up.
  display.fillRect(270, 306, 10, 12, TFT_WHITE);
  display.fillRect(246, 278, 15, 18, TFT_WHITE);
  display.fillRect(216, 243, 20, 24, TFT_WHITE);

  // Clear the sleeping facial marks while preserving the head outline. This
  // larger canvas fits the idle eyes at their real seven-pixel cell size.
  display.fillRect(pandaX + 55, pandaY + 77, 104, 35, TFT_WHITE);

  drawIdleStyleSleepingEyes(pandaX, pandaY);

  // Only the Z and face area changed, so avoid a full-screen e-paper flash.
  display.updataPartial(wakeArea.x, wakeArea.y,
                        wakeArea.width, wakeArea.height);
  delay(WAKE_EYES_OPEN_DELAY_MS);

  drawIdlePanda();
}

bool sleepButtonPressed() {
  for (const int pin : SLEEP_BUTTON_PINS) {
    if (digitalRead(pin) == LOW) {
      return true;
    }
  }
  return false;
}

void playStartupAnimation() {
  const Rect bounds = pandaBounds();
  const int centreRow = PANDA_IDLE_ROWS / 2;
  bool firstFrame = true;

  for (const int halfRows : REVEAL_HALF_ROWS) {
    // Every frame expands the ink reveal outward from the panda's centre.
    if (firstFrame) {
      // Clear the complete title card as the panda reveal begins.
      display.fillScreen(TFT_WHITE);
    } else {
      display.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, TFT_WHITE);
    }

    drawPandaRows(bounds, centreRow - halfRows, centreRow + halfRows + 1);

    if (firstFrame) {
      display.update();
      firstFrame = false;
    } else {
      refresh(bounds);
    }

    delay(REVEAL_FRAME_DELAY_MS);
  }

  blinkOnce(bounds);
}
}  // namespace

void setup() {
  for (const int pin : SLEEP_BUTTON_PINS) {
    pinMode(pin, INPUT_PULLUP);
  }

  display.begin();
  display.fillScreen(TFT_WHITE);
  display.update();
  delay(BLANK_SCREEN_DELAY_MS);
  showTitleCard();
  playStartupAnimation();
}

void loop() {
  const bool buttonPressed = sleepButtonPressed();

  if (buttonPressed && !sleepButtonLatched) {
    delay(BUTTON_DEBOUNCE_MS);
    if (sleepButtonPressed()) {
      sleepButtonLatched = true;

      if (sleepingPandaShown) {
        wakeSleepingPanda();
        sleepingPandaShown = false;
      } else {
        showSleepingPanda();
        sleepingPandaShown = true;
      }
    }
  } else if (!buttonPressed) {
    // Require a release before another transition can start. Holding a button
    // through an e-paper refresh therefore cannot immediately toggle back.
    sleepButtonLatched = false;
  }

  // E-paper retains whichever state was drawn most recently.
  delay(10);
}
