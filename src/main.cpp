#include <Arduino.h>
#include <TFT_eSPI.h>
#include "panda_phone.h"

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
    {341, 363,  0, false, 240},  // Sidder stille
    {372, 313,  0, false,  45},  // Gør sig klar
    {322, 371, 48, false,  30},  // Sætter af
    {331, 355, 96, true,   60},  // Toppen af hoppet
    {341, 363, 42, false,  30},  // På vej ned
    {378, 305,  0, false,  45},  // Lander
};

constexpr int POSE_COUNT = sizeof(POSES) / sizeof(POSES[0]);
constexpr int ANIMATION_WIDTH = 544;
constexpr int ANIMATION_HEIGHT = 464;
constexpr int BITMAP_ROW_BYTES = (PANDA_PHONE_WIDTH + 7) / 8;

constexpr const char* HEART[] = {
    "0110110",
    "1111111",
    "1111111",
    "0011100",
    "0001000",
};
    
int poseIndex = 0;
unsigned long poseShownAt = 0;

void drawHeart(int x, int y) {
  constexpr int cell = 6;
  constexpr int rows = sizeof(HEART) / sizeof(HEART[0]);

  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < 7; ++col) {
      if (HEART[row][col] == '1') {
        display.fillRect(x + col * cell, y + row * cell,
                         cell, cell, TFT_BLACK);
      }
    }
  }
}

void drawPhonePanda(int x, int y, int width, int height) {
  for (int row = 0; row < PANDA_PHONE_HEIGHT; ++row) {
    const int top = y + row * height / PANDA_PHONE_HEIGHT;
    const int bottom = y + (row + 1) * height / PANDA_PHONE_HEIGHT;
    const uint8_t* bitmapRow = PANDA_PHONE_BITMAP + row * BITMAP_ROW_BYTES;

    // Tegn hver sammenhængende stribe af sorte pixels som ét rektangel.
    int runStart = -1;
    uint8_t packedPixels = 0;

    for (int col = 0; col <= PANDA_PHONE_WIDTH; ++col) {
      if (col < PANDA_PHONE_WIDTH && col % 8 == 0) {
        packedPixels = pgm_read_byte(bitmapRow + col / 8);
      }

      const bool black = col < PANDA_PHONE_WIDTH &&
                         (packedPixels & (0x80 >> (col % 8))) != 0;

      if (black && runStart == -1) {
        runStart = col;
      } else if (!black && runStart != -1) {
        const int left = x + runStart * width / PANDA_PHONE_WIDTH;
        const int right = x + col * width / PANDA_PHONE_WIDTH;
        display.fillRect(left, top, right - left, bottom - top, TFT_BLACK);
        runStart = -1;
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

  drawPhonePanda(pandaX, pandaY, pose.width, pose.height);

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