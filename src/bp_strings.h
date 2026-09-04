#pragma once

#include <cstdint>

namespace bp {

struct LocalizedString {
    const char* id;
    const char* english;
    const char* koreanUtf8;
};

void SetTextLanguageEnglish(bool english) noexcept;
bool TextLanguageEnglish() noexcept;
const char* LocalizedText(const char* english) noexcept;
const LocalizedString* RuntimeStrings(std::uint16_t& count) noexcept;

}  // namespace bp
