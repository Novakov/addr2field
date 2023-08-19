#pragma once

#include <memory>
#include <cstdint>
#include "dwarf/fwd.hpp"
#include "libdwarf/libdwarf.h"

namespace dwarf
{
    enum DieTag : std::uint16_t
    {
        DW_TAG_namespace = 0x39,
        DW_TAG_class_type = 0x02,
        DW_TAG_structure_type = 0x13,
        DW_TAG_union_type = 0x17,
        DW_TAG_variable = 0x34,
        DW_TAG_member = 0x0d,
        DW_TAG_typedef = 0x16,
        DW_TAG_array_type = 0x01,
        DW_TAG_base_type = 0x24,
        DW_TAG_pointer_type = 0x0f,
        DW_TAG_inheritance = 0x1c,
        DW_TAG_enumerator = 0x04,
    };

    namespace details
    {
        class DieDeleter
        {
        public:
            DieDeleter() = default;
            DieDeleter(const DieDeleter&) = default;

            DieDeleter(const DebugInfo& owner, bool isInfo);

            const DebugInfo& Owner() const
            {
                return *_owner;
            }

            bool IsInfo() const
            {
                return _isInfo;
            }

            void operator()(Dwarf_Die die);

        private:
            const DebugInfo* _owner = nullptr;
            bool _isInfo;
        };
    }

    Die Sibling(const Die& die);
    Die Child(const Die& die);
    Die GetCompilationUnitTopDie(const DebugInfo& debug, bool isInfo);
    Die GetCompilationUnitTopDie(const DebugInfo& debug, bool isInfo, const Die& descendantDie);
    Die GetDieByOffset(const DebugInfo& debug, bool isInfo, Dwarf_Off offset);

    Dwarf_Off GetDieOffset(const Die& die);

    DieTag GetTag(const Die& die);

    const char* GetTagName(DieTag tag);

    [[deprecated("Use wrapping function")]] Die Wrap(Dwarf_Die die);
}