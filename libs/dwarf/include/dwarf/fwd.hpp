#pragma once
#include <memory>
#include "libdwarf/libdwarf.h"

namespace dwarf
{
    namespace details
    {
        struct DieDeleter;
        struct DebugInfoDeleter;
    }

    using Die = std::unique_ptr<Dwarf_Die_s, details::DieDeleter>;
    using DebugInfo = std::unique_ptr<Dwarf_Debug_s, details::DebugInfoDeleter>;
}