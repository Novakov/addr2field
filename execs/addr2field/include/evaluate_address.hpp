#pragma once
#include <cstdint>
#include <optional>
#include "dwarf/fwd.hpp"

std::optional<std::int64_t> EvaluateAddress(const dwarf::Die& die);