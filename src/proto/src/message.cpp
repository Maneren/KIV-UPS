#include <proto/message.h>

namespace proto {

TakeResult take_frame(std::string &buffer) {
  const auto nl = buffer.find('\n');
  if (nl == std::string::npos) {
    if (buffer.size() > MAX_LINE) {
      buffer.clear();
      return DroppedFrame{};
    }
    return IncompleteFrame{};
  }

  const auto total = nl + 1;
  if (total > MAX_LINE) {
    buffer.erase(0, total);
    return DroppedFrame{};
  }

  auto frame = buffer.substr(0, total);
  buffer.erase(0, total);
  return Frame{.frame = std::move(frame)};
}

} // namespace proto
