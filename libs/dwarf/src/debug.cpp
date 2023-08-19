#include "dwarf/debug.hpp"

namespace dwarf
{
    namespace details
    {
        void DebugInfoDeleter::operator()(Dwarf_Debug debug)
        {
            dwarf_finish(debug);
        }
    }

    DebugInfo OpenDebugInfo(const char* fileName)
    {
        Dwarf_Ptr errptr = 0;
        Dwarf_Debug dbg = nullptr;
        Dwarf_Error error = nullptr;
        auto res =
            dwarf_init_path(fileName, nullptr, 0, DW_GROUPNUMBER_ANY, nullptr, &errptr, &dbg, &error);

        if(res != DW_DLV_OK)
        {
            return {};
        }

        return {dbg, {}};
    }
}