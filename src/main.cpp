#include <Arduino.h>
#include <TFT_eSPI.h>
#include "panda_idle_grid.h"

namespace {
EPaper display;

struct Pose {
  int width;
  int height;
  int lift;
  bool showHeart;
  unsigned long holdMs;
};

constexpr Pose POSES[] = {
  // Bredde, højde, løft, hjerte, pause i ms
  {341, 363,  0, false, 2400}, // Sidder stille
  {372, 313,  0, false,  450}, // Gør sig klar
  {322, 371, 48, false,  300}, // Sætter af
  {331, 355, 96, true,   600}, // Toppen af hoppet
  {341, 363, 42, false,  300}, // På vej ned
  {378, 305,  0, false,  450}, // Lander
};

constexpr int POSE_COUNT = sizeof(POSES) / sizeof(POSES[0]);
constexpr int ANIMATION_WIDTH = 544;
constexpr int ANIMATION_HEIGHT = 464;

constexpr const char* HEART[] = {
  "0110110",
  "1111111",
  "1111111",
  "0111110",
  "0011100",
  "0001000",
};

int poseIndex = 0;
unsigned long poseShownAt = 0;

void drawHeart(int x, int y) {
  constexpr int cell = 6;

  for (int row = 0; row < 6; ++row) {
    for (int col = 0; col < 7; ++col) {
      if (HEART[row][col] == '1') {
        display.fillRect(x + col * cell, y + row * cell,
                         cell, cell, TFT_BLACK);
      }
    }
  }
}

void drawFrame() {
  const Pose& pose = POSES[poseIndex];
  const int centerX = display.width() / 2;
  const int areaX = centerX - ANIMATION_WIDTH / 2;
  const int areaY = display.height() - ANIMATION_HEIGHT;
  const int groundY = display.height() - 10;

  display.fillRect(areaX, areaY, ANIMATION_WIDTH,
                   ANIMATION_HEIGHT, TFT_WHITE);

  const int pandaX = centerX - pose.width / 2;
  const int pandaY = groundY - pose.lift - pose.height;

  // Tegn sammenhængende sorte pixels i én operation pr. stribe.
  for (int row = 0; row < PANDA_IDLE_ROWS; ++row) {
    const int top = pandaY + row * pose.height / PANDA_IDLE_ROWS;
    const int bottom = pandaY + (row + 1) * pose.height / PANDA_IDLE_ROWS;
    int runStart = -1;

    for (int col = 0; col <= PANDA_IDLE_COLUMNS; ++col) {
      const bool black = col < PANDA_IDLE_COLUMNS &&
                         PANDA_IDLE[row][col] == '1';

      if (black && runStart == -1) {
        runStart = col;
      } else if (!black && runStart != -1) {
        const int left = pandaX + runStart * pose.width / PANDA_IDLE_COLUMNS;
        const int right = pandaX + col * pose.width / PANDA_IDLE_COLUMNS;
        display.fillRect(left, top, right - left, bottom - top, TFT_BLACK);
        runStart = -1;
      }
    }
  }

  if (pose.showHeart) {
    drawHeart(pandaX + pose.width + 18, pandaY + 46);
  }
}
}  // namespace

void setup() {
  display.begin();
  display.fillScreen(TFT_WHITE);
  drawFrame();
  display.update();
  poseShownAt = millis();
}

void loop() {
  if (millis() - poseShownAt < POSES[poseIndex].holdMs) {
    delay(10);
    return;
  }

  poseIndex = (poseIndex + 1) % POSE_COUNT;
  drawFrame();

#ifdef USE_PARTIAL_EPAPER
  display.updataPartial((display.width() - ANIMATION_WIDTH) / 2,
                        display.height() - ANIMATION_HEIGHT,
                        ANIMATION_WIDTH, ANIMATION_HEIGHT);
#else
  display.update();
#endif

  poseShownAt = millis();
}