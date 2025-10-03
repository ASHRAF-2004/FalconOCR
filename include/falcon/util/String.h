#pragma once

#include <string>
#include <string_view>

namespace falcon::util {

std::string ToUtf8(std::u32string_view input);

}  // namespace falcon::util
