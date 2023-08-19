#include "dwarf/die.hpp"
#include "dwarf/debug.hpp"
#include "gsl/assert"

namespace dwarf
{
    namespace details
    {
        DieDeleter::DieDeleter(const DebugInfo& owner, bool isInfo) :
            _owner{&owner},
            _isInfo{isInfo}
        {
        }

        void DieDeleter::operator()(Dwarf_Die die)
        {
            Expects(_owner != nullptr);
            dwarf_dealloc_die(die);
        }
    }

    Die Wrap(Dwarf_Die die)
    {
        return Die{die, {}};
    }

    Die Child(const Die& die)
    {
        Dwarf_Error error;
        Dwarf_Die ptr = nullptr;
        if(dwarf_child(die.get(), &ptr, &error) == DW_DLV_OK)
        {
            return Die{ptr, {die.get_deleter()}};
        }
        else
        {
            return Die{nullptr, {}};
        }
    }

    Die Sibling(const Die& die)
    {
        Dwarf_Die ptr = nullptr;
        Dwarf_Error error;
        const auto& owner = die.get_deleter();
        if(dwarf_siblingof_b(owner.Owner().get(), die.get(), owner.IsInfo(), &ptr, &error) == DW_DLV_OK)
        {
            return Die{ptr, die.get_deleter()};
        }
        else
        {
            return Die{nullptr, {}};
        }
    }

    Die GetCompilationUnitTopDie(const DebugInfo& debug, bool isInfo)
    {
        Dwarf_Die ptr = nullptr;
        Dwarf_Error error;
        if(dwarf_siblingof_b(debug.get(), nullptr, isInfo, &ptr, &error) == DW_DLV_OK)
        {
            return Die{ptr, {debug, isInfo}};
        }
        else
        {
            return Die{nullptr, {}};
        }
    }

    DieTag GetTag(const Die& die)
    {
        Dwarf_Error error;
        Dwarf_Half tag;
        dwarf_tag(die.get(), &tag, &error);
        return static_cast<DieTag>(tag);
    }

    const char* GetTagName(DieTag tag)
    {
        const char* s;
        dwarf_get_TAG_name(tag, &s);
        return s;
    }

    Dwarf_Off GetDieOffset(const Die& die)
    {
        Dwarf_Off offset;
        Dwarf_Error error;
        dwarf_dieoffset(die.get(), &offset, &error);
        return offset;
    }

    Die GetCompilationUnitTopDie(const DebugInfo& debug, bool isInfo, const Die& descendantDie)
    {
        Dwarf_Error error;
        Dwarf_Off offset;
        dwarf_CU_dieoffset_given_die(descendantDie.get(), &offset, &error);
        return GetDieByOffset(debug, isInfo, offset);
    }

    Die GetDieByOffset(const DebugInfo& debug, bool isInfo, Dwarf_Off offset)
    {
        Dwarf_Error error;
        Dwarf_Die die;
        if(dwarf_offdie_b(debug.get(), offset, isInfo, &die, &error) != DW_DLV_OK)
        {
            return {};
        }
        return {die, {debug, isInfo}};
    }
}