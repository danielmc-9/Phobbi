#pragma once

// 7.5-inch monochrome 800x480 panel using the UC8179 controller.
#ifndef BOARD_SCREEN_COMBO
  #define BOARD_SCREEN_COMBO 502
#endif

// The ESP32-S3 Plus display board used by the 7.5-inch kit. EE04 and EE05
// have the same display pinout; EE04 is the name used by Seeed's cookbook.
#ifndef USE_XIAO_EPAPER_DISPLAY_BOARD_EE04
  #define USE_XIAO_EPAPER_DISPLAY_BOARD_EE04
#endif
