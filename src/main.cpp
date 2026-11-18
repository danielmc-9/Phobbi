#include <Arduino.h>
#include <TFT_eSPI.h>

namespace {
EPaper display;

constexpr char TITLE[] = "Phobbie";

// Only this part of the screen changes during the hatching animation.
constexpr int CHARACTER_AREA_WIDTH = 280;
constexpr int CHARACTER_AREA_HEIGHT = 280;
constexpr int CHARACTER_AREA_Y = 155;

constexpr int EGG_COLUMNS = 17;
constexpr int EGG_ROWS = 20;
constexpr int EGG_PIXEL_SIZE = 7;

constexpr int PANDA_COLUMNS = 31;
constexpr int PANDA_ROWS = 33;
constexpr int PANDA_PIXEL_SIZE = 7;
constexpr int PANDA_BREATH_EXPANSION = 8;
constexpr int PANDA_BREATH_LIFT = 3;
constexpr unsigned long PANDA_FRAME_INTERVAL_MS = 1300;

// Idle panda bitmap. 1 is a black pixel and 0 is white.
constexpr char PANDA_IDLE[PANDA_ROWS][PANDA_COLUMNS + 1] = {
    "0000000111100000000000111100000",
    "0000001111110111111101111110000",
    "0000001111111000000011111111000",
    "0000001111000000000001111111000",
    "0000000110000000000000111111000",
    "0000000100000000000000011110000",
    "0000001000000000000000001100000",
    "0000001001100000111000000100000",
    "0000010011100001111100000110000",
    "0000010110100001001110000010000",
    "0000010110100001001110000010000",
    "0000010111000000111110000010000",
    "0000010010111100011100000010000",
    "0000010000111000000000000110000",
    "0000001000010000000000000111000",
    "0000001000111110000000001111000",
    "0000000100000000000000111111100",
    "0000000111000000000011111111100",
    "0000001111111111111111111111110",
    "0000011111111111111111111110010",
    "0000011110000000011111111100010",
    "0000111100000000111111111100011",
    "0000111100000001111111111000001",
    "0000111000000011111111111000001",
    "0111111000000011111111110000001",
    "1111111000000001100000011000001",
    "1111111000000001011111111100001",
    "1111111000000001111111111110001",
    "1111111100000001111111111110011",
    "0111111100000001111111111110010",
    "0111111110000001111111111110100",
    "0011111111000001111111111111100",
    "0001111111111111111111111100001",
};

int characterAreaX = 0;
bool pandaIsInhaling = false;
unsigned long lastPandaFrame = 0;

bool insideEllipse(int x, int y, int centreX, int centreY,
                   int radiusX, int radiusY) {
  const long dx = x - centreX;
  const long dy = y - centreY;
  const long radiusXSquared = radiusX * radiusX;
  const long radiusYSquared = radiusY * radiusY;

  return dx * dx * radiusYSquared + dy * dy * radiusXSquared <=
         radiusXSquared * radiusYSquared;
}

void clearCharacterArea() {
  display.fillRect(characterAreaX, CHARACTER_AREA_Y,
                   CHARACTER_AREA_WIDTH, CHARACTER_AREA_HEIGHT, TFT_WHITE);
}

void refreshCharacterArea() {
  display.updataPartial(characterAreaX, CHARACTER_AREA_Y,
                        CHARACTER_AREA_WIDTH, CHARACTER_AREA_HEIGHT);
}

bool isCrackPixel(int x, int y, int crackStage) {
  if (crackStage == 0) {
    return false;
  }

  const bool firstCrack =
      (x == 8 && y == 4) || (x == 7 && y == 5) ||
      (x == 8 && y == 6) || (x == 7 && y == 7);

  if (crackStage == 1) {
    return firstCrack;
  }

  const bool secondCrack =
      (x == 8 && y == 8) || (x == 9 && y == 9) ||
      (x == 8 && y == 10) || (x == 9 && y == 11) ||
      (x == 10 && y == 12);

  return firstCrack || secondCrack;
}

bool isEggDot(int x, int y) {
  return (x == 5 && y == 7) || (x == 11 && y == 6) ||
         (x == 12 && y == 11) || (x == 5 && y == 12) ||
         (x == 8 && y == 15) || (x == 12 && y == 16);
}

void drawEgg(int horizontalOffset, int crackStage, bool pandaPeeking = false) {
  clearCharacterArea();

  const int eggWidth = EGG_COLUMNS * EGG_PIXEL_SIZE;
  const int eggHeight = EGG_ROWS * EGG_PIXEL_SIZE;
  const int startX = characterAreaX + (CHARACTER_AREA_WIDTH - eggWidth) / 2 +
                     horizontalOffset;
  const int startY = CHARACTER_AREA_Y +
                     (CHARACTER_AREA_HEIGHT - eggHeight) / 2;

  for (int y = 0; y < EGG_ROWS; ++y) {
    for (int x = 0; x < EGG_COLUMNS; ++x) {
      const bool outerEgg = insideEllipse(x, y, 8, 10, 7, 9);
      const bool innerEgg = insideEllipse(x, y, 8, 10, 6, 8);
      const bool outline = outerEgg && !innerEgg;

      bool black = outline || (innerEgg && isEggDot(x, y)) ||
                   isCrackPixel(x, y, crackStage);

      if (pandaPeeking) {
        const bool leftEar = insideEllipse(x, y, 5, 7, 2, 2);
        const bool rightEar = insideEllipse(x, y, 11, 7, 2, 2);
        const bool head = insideEllipse(x, y, 8, 10, 5, 4);
        const bool face = insideEllipse(x, y, 8, 10, 4, 3);
        const bool leftEye = insideEllipse(x, y, 6, 10, 1, 1);
        const bool rightEye = insideEllipse(x, y, 10, 10, 1, 1);

        if (leftEar || rightEar || head) black = true;
        if (face) black = false;
        if (leftEye || rightEye || (x == 8 && y == 12)) black = true;
      }

      if (black) {
        display.fillRect(startX + x * EGG_PIXEL_SIZE,
                         startY + y * EGG_PIXEL_SIZE,
                         EGG_PIXEL_SIZE, EGG_PIXEL_SIZE, TFT_BLACK);
      }
    }
  }
}

void drawPanda(bool inhaling = false) {
  clearCharacterArea();

  const int restingWidth = PANDA_COLUMNS * PANDA_PIXEL_SIZE;
  const int pandaWidth = restingWidth +
                         (inhaling ? PANDA_BREATH_EXPANSION : 0);
  const int pandaHeight = PANDA_ROWS * PANDA_PIXEL_SIZE;
  const int startX = characterAreaX +
                     (CHARACTER_AREA_WIDTH - pandaWidth) / 2;
  const int startY = CHARACTER_AREA_Y +
                     (CHARACTER_AREA_HEIGHT - pandaHeight) / 2 -
                     (inhaling ? PANDA_BREATH_LIFT : 0);

  for (int y = 0; y < PANDA_ROWS; ++y) {
    for (int x = 0; x < PANDA_COLUMNS; ++x) {
      if (PANDA_IDLE[y][x] == '1') {
        // Calculate shared cell edges so the wider inhale frame has no gaps.
        const int pixelLeft = startX + x * pandaWidth / PANDA_COLUMNS;
        const int pixelRight = startX + (x + 1) * pandaWidth / PANDA_COLUMNS;

        display.fillRect(pixelLeft,
                         startY + y * PANDA_PIXEL_SIZE,
                         pixelRight - pixelLeft, PANDA_PIXEL_SIZE, TFT_BLACK);
      }
    }
  }
}

void showEggFrame(int horizontalOffset, int crackStage,
                  bool pandaPeeking = false, unsigned long pauseMs = 350) {
  drawEgg(horizontalOffset, crackStage, pandaPeeking);
  refreshCharacterArea();
  delay(pauseMs);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  display.begin();

  characterAreaX = (display.width() - CHARACTER_AREA_WIDTH) / 2;

  display.fillScreen(TFT_WHITE);
  display.setTextColor(TFT_BLACK);
  display.setFreeFont(&FreeSansBold24pt7b);
  display.setTextSize(1);

  const int titleWidth = display.textWidth(TITLE);
  display.drawString(TITLE, (display.width() - titleWidth) / 2, 45);

  // The first frame is sent with a clean full-screen refresh.
  drawEgg(0, 0);
  display.update();
  delay(700);

  // A short ePaper-friendly hatching animation.
  showEggFrame(-7, 0);
  showEggFrame(7, 0);
  showEggFrame(-4, 1, false, 450);
  showEggFrame(4, 2, false, 500);
  showEggFrame(0, 2, true, 650);

  drawPanda();
  refreshCharacterArea();
  lastPandaFrame = millis();
}

void loop() {
  if (millis() - lastPandaFrame >= PANDA_FRAME_INTERVAL_MS) {
    pandaIsInhaling = !pandaIsInhaling;
    drawPanda(pandaIsInhaling);
    refreshCharacterArea();
    lastPandaFrame = millis();
  }

  delay(10);
}