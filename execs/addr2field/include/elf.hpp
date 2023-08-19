#pragma once
#include <cstdint>
#include <optional>
#include <string>

std::optional<std::tuple<std::string, std::uint64_t>>
    LookupSymbolByAddress(const char* elfFile, std::uint32_t address);
