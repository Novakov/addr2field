#pragma once
#include "dwarf/fwd.hpp"
#include "libdwarf/libdwarf.h"

namespace dwarf
{
    namespace details
    {
        struct DebugInfoDeleter
        {
            void operator()(Dwarf_Debug debug);
        };
    }

    DebugInfo OpenDebugInfo(const char* fileName);
}