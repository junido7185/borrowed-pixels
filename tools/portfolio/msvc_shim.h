// GCC/Clang shim for the MSVC-only CRT calls used by tests/bp_render_font_ui_tests.cpp
// (fopen_s, _dupenv_s). Portfolio tooling only; the shipped build never includes it.
//
//   g++ -std=c++20 -O2 -include tools/portfolio/msvc_shim.h -DBP_TEST=1 -DBP_ENABLE_ASSERTS=1 -Isrc \
//       tests/*.cpp $(ls src/*.cpp | grep -v bp_win32.cpp) -o bp_test
//   env -u BP_T03_EVIDENCE_DIR ./bp_test
//
// Leave BP_T03_EVIDENCE_DIR unset to match the MSVC check counts in docs/evidence/T14:
// when it is set, T03 also writes 8 evidence BMPs and counts 8 more checks.
#pragma once
#ifndef _MSC_VER
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

inline int fopen_s(std::FILE** file, const char* name, const char* mode) {
    *file = std::fopen(name, mode);
    return *file ? 0 : errno;
}

inline int _dupenv_s(char** buffer, std::size_t* length, const char* name) {
    const char* value = std::getenv(name);
    if (!value) {
        *buffer = nullptr;
        if (length) *length = 0;
        return 0;
    }
    const std::size_t size = std::strlen(value) + 1;
    *buffer = static_cast<char*>(std::malloc(size));
    if (!*buffer) return ENOMEM;
    std::memcpy(*buffer, value, size);
    if (length) *length = size;
    return 0;
}
#endif
