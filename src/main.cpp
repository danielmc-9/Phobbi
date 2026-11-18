#include <Arduino.h>
#include <TFT_eSPI.h>

namespace {
EPaper display;

constexpr char NAMES[][9] = {"Jennie", "Kristine", "Johan"};
constexpr int NAME_COUNT = sizeof(NAMES) / sizeof(NAMES[0]);
constexpr char TITLE_SUFFIX[] = " er smuk";
constexpr unsigned long NAME_CHANGE_INTERVAL_MS = 1000;

// An 11 x 9 pixel heart. Each 1-bit is drawn as one large square pixel.
constexpr char HEART[] =
    "00110001100"
    "01111011110"
    "11111111111"
    "11111111111"
    "01111111110"
    "00111111100"
    "00011111000"
    "00001110000"
    "00000100000";

constexpr int HEART_COLUMNS = 11;
constexpr int HEART_ROWS = 9;
constexpr int HEART_PIXEL_SIZE = 14;
constexpr int TITLE_HEART_GAP = 32;

int nameX = 0;
int nameY = 0;
int nameAreaWidth = 0;
int nameAreaHeight = 0;
int currentNameIndex = 0;
unsigned long lastNameChange = 0;

void drawPixelHeart(int x, int y) {
  for (int row = 0; row < HEART_ROWS; ++row) {
    for (int column = 0; column < HEART_COLUMNS; ++column) {
      if (HEART[row * HEART_COLUMNS + column] == '1') {
        display.fillRect(x + column * HEART_PIXEL_SIZE,
                         y + row * HEART_PIXEL_SIZE,
                         HEART_PIXEL_SIZE,
                         HEART_PIXEL_SIZE,
                         TFT_BLACK);
      }
    }
  }
}

void drawName() {
  display.fillRect(nameX, nameY, nameAreaWidth, nameAreaHeight, TFT_WHITE);
  display.drawString(NAMES[currentNameIndex], nameX, nameY);
}
}  // namespace

void setup() {
  Serial.begin(115200);

  display.begin();

  display.fillScreen(TFT_WHITE);
  display.setTextColor(TFT_BLACK);
  display.setFreeFont(&FreeSansBold24pt7b);
  display.setTextSize(1);

  const int heartWidth = HEART_COLUMNS * HEART_PIXEL_SIZE;
  const int heartHeight = HEART_ROWS * HEART_PIXEL_SIZE;
  nameAreaWidth = max(display.textWidth(NAMES[0]), display.textWidth(NAMES[1]));
  nameAreaHeight = display.fontHeight();
  const int suffixWidth = display.textWidth(TITLE_SUFFIX);
  const int titleWidth = nameAreaWidth + suffixWidth;
  const int groupWidth = titleWidth + TITLE_HEART_GAP + heartWidth;
  const int groupX = (display.width() - groupWidth) / 2;
  const int centreY = display.height() / 2;

  nameX = groupX;
  nameY = centreY - nameAreaHeight / 2;

  drawName();
  display.drawString(TITLE_SUFFIX, nameX + nameAreaWidth, nameY);
  drawPixelHeart(groupX + titleWidth + TITLE_HEART_GAP,
                 centreY - heartHeight / 2);

  display.update();
  lastNameChange = millis();
}

void loop() {
  if (millis() - lastNameChange >= NAME_CHANGE_INTERVAL_MS) {
    currentNameIndex = (currentNameIndex + 1) % NAME_COUNT;
    drawName();
    display.updataPartial(nameX, nameY, nameAreaWidth, nameAreaHeight);
    lastNameChange = millis();
  }

  delay(10);
}