#pragma once

#include <Color.hpp>
#include <Font.hpp>
#include <Text.hpp>

namespace {

constexpr float DEFAULT_TEXT_SIZE = 10.F;

}

namespace rayutils {

inline float spacing(float font_size) { return font_size / DEFAULT_TEXT_SIZE; }

inline raylib::Text empty_text(
    float font_size = DEFAULT_TEXT_SIZE,
    raylib::Color color = raylib::Color::DarkGray(),
    ::Font font = raylib::Font{}
) {
  return raylib::Text{{}, font_size, color, font, spacing(font_size)};
}

inline raylib::Text text(
    const std::string &text,
    float font_size = DEFAULT_TEXT_SIZE,
    raylib::Color color = raylib::Color::DarkGray(),
    ::Font font = raylib::Font{}
) {
  return raylib::Text{text, font_size, color, font, spacing(font_size)};
}

} // namespace rayutils
