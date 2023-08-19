#include "dwarf/compilationUnit.hpp"
#include "dwarf/debug.hpp"

namespace dwarf
{
    bool NavigateToNextCompilationUnit(const DebugInfo& debug)
    {
        Dwarf_Error error;

        Dwarf_Unsigned cuHeaderLength = 0;
        Dwarf_Half versionStamp = 0;
        Dwarf_Off abbrevOffset = 0;
        Dwarf_Half addressSize = 0;
        Dwarf_Half lengthSize = 0;
        Dwarf_Half extensionSize = 0;
        Dwarf_Sig8 typeSignature;
        Dwarf_Unsigned typeOffset = 0;
        Dwarf_Unsigned nextHeaderOffset = 0;
        Dwarf_Half headerCuType = 0;

        auto res = dwarf_next_cu_header_d(
            debug.get(),
            true,
            &cuHeaderLength,
            &versionStamp,
            &abbrevOffset,
            &addressSize,
            &lengthSize,
            &extensionSize,
            &typeSignature,
            &typeOffset,
            &nextHeaderOffset,
            &headerCuType,
            &error);

        return res == DW_DLV_OK;
    }
}