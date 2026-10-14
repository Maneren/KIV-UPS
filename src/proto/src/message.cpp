#include <proto/message.h>

namespace proto {

TakeResult take_frame(std::string &buffer) {
  const auto nl = buffer.find('\n');
  if (nl == std::string::npos) {
    if (buffer.size() > MAX_LINE) {
      buffer.clear();
      return {.status = TakeStatus::Error, .frame = {}};
    }
    return {.status = TakeStatus::Incomplete, .frame = {}};
  }
  const auto total = nl + 1;
  auto frame = buffer.substr(0, total);
  buffer.erase(0, total);
  if (total > MAX_LINE) {
    return {.status = TakeStatus::Error, .frame = {}};
  }
  return {.status = TakeStatus::Ok, .frame = std::move(frame)};
}

} // namespace proto
