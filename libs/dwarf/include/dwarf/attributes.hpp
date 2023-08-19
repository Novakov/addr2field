#pragma once
#include <optional>
#include <string_view>
#include "dwarf/die.hpp"
#include "dwarf/fwd.hpp"
#include "libdwarf/libdwarf.h"

namespace dwarf
{
    enum FormType : std::uint16_t
    {
        DW_FORM_strp = 0xe,
        DW_FORM_string = 0x8,
        DW_FORM_flag = 0xc,

        DW_FORM_data = 0x1005,
    };

    enum Attribute : std::uint16_t
    {
        DW_AT_NAME = 0x03,
        DW_AT_declaration = 0x3c,
        DW_AT_linkage_name = 0x6e,
        DW_AT_data_member_location = 0x38,
        DW_AT_Location = 0x02,
        DW_AT_external = 0x3f,
    };

    namespace details
    {
        template <FormType Form> struct FormDescriptor;
        template <Attribute Attribute> struct KnownAttributeDescriptor;
    }

    template <Attribute DieAttribute, FormType Form = details::KnownAttributeDescriptor<DieAttribute>::Form>
    typename std::optional<typename details::FormDescriptor<Form>::ValueType> ReadDieAttribute(const Die& die)
    {
        Dwarf_Error error;
        Dwarf_Attribute attribute;
        if(dwarf_attr(die.get(), DieAttribute, &attribute, &error))
        {
            return std::nullopt;
        }

        auto ret = details::FormDescriptor<Form>::Read(attribute);

        dwarf_dealloc_attribute(attribute);

        return ret;
    }

    std::optional<std::string_view> ReadDieName(const Die& die);

    Die ReadDieType(const Die &die);
}

#include "dwarf/attributes/forms.hpp"
#include "dwarf/attributes/wellKnown.hpp"