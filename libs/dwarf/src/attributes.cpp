#include "dwarf/attributes.hpp"
#include "dwarf/attributes/forms.hpp"
#include "dwarf/debug.hpp"

namespace dwarf
{
    namespace details
    {
        std::string_view FormDescriptor<FormType::DW_FORM_strp>::Read(Dwarf_Attribute attribute)
        {
            char* r;
            Dwarf_Error error;
            dwarf_formstring(attribute, &r, &error);
            return std::string_view{r};
        }

        std::string_view FormDescriptor<FormType::DW_FORM_string>::Read(Dwarf_Attribute attribute)
        {
            char* r;
            Dwarf_Error error;
            dwarf_formstring(attribute, &r, &error);
            return std::string_view{r};
        }

        bool FormDescriptor<FormType::DW_FORM_flag>::Read(Dwarf_Attribute attribute)
        {
            Dwarf_Bool b;
            Dwarf_Error error;
            dwarf_formflag(attribute, &b, &error);
            return b;
        }

        std::int64_t FormDescriptor<FormType::DW_FORM_data>::Read(Dwarf_Attribute attribute)
        {
            Dwarf_Unsigned b;
            Dwarf_Error error;
            dwarf_formudata(attribute, &b, &error);
            return b;
        }
    }

    std::optional<std::string_view> ReadDieName(const Die& die)
    {
        return ReadDieAttribute<Attribute::DW_AT_NAME>(die);
    }

    Die ReadDieType(const Die& die)
    {
        Dwarf_Error error;
        Dwarf_Off offset;
        Dwarf_Bool dwIsInfo;

        if(dwarf_dietype_offset(die.get(), &offset, &dwIsInfo, &error) != DW_DLV_OK)
        {
            return {};
        }

        Dwarf_Die typeDie;
        const auto& owner = die.get_deleter();
        if(dwarf_offdie_b(owner.Owner().get(), offset, owner.IsInfo(), &typeDie, &error) != DW_DLV_OK)
        {
            return {};
        }

        return {typeDie, die.get_deleter()};
    }
}