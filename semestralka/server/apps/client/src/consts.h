#pragma once

#include <numbers>

constexpr float HEX_SIZE = 40.0F;
constexpr int BOARD_OFFSET_X = 400;
constexpr int BOARD_OFFSET_Y = 300;

constexpr float HEX_X_FACTOR = 3.0F / 2.0F;
constexpr float HEX_Y_FACTOR = std::numbers::sqrt3_v<float>;
constexpr float HEX_ANGLE_STEP = 60.0F * (std::numbers::pi_v<float> / 180.0F);
constexpr int HEX_SIDES = 6;

constexpr int TEXT_FONT_SIZE = 32;
constexpr int TEXT_FONT_SPACING = 5;
