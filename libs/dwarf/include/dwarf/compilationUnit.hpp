#pragma once
#include "dwarf/fwd.hpp"
#include "libdwarf/libdwarf.h"

namespace dwarf
{
    bool NavigateToNextCompilationUnit(const DebugInfo& debug);
}