#pragma once

#include <cstddef>
#include <stdexcept>

namespace hive {

class HiveError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class IllegalMoveError : public HiveError {
public:
  using HiveError::HiveError;
};

class EmptyTileError : public HiveError {
public:
  using HiveError::HiveError;
};

constexpr std::size_t MAX_STACK_IGNORE = 2;

} // namespace hive
