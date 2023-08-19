#pragma once
#include <string_view>
#include "dwarf/attributes.hpp"

namespace dwarf
{
    namespace details
    {
        template <> struct FormDescriptor<FormType::DW_FORM_strp>
        {
            using ValueType = std::string_view;

            static ValueType Read(Dwarf_Attribute attribute);
        };

        template <> struct FormDescriptor<FormType::DW_FORM_string>
        {
            using ValueType = std::string_view;

            static ValueType Read(Dwarf_Attribute attribute);
        };

        template <> struct FormDescriptor<FormType::DW_FORM_flag>
        {
            using ValueType = bool;

            static ValueType Read(Dwarf_Attribute attribute);
        };

        template <> struct FormDescriptor<FormType::DW_FORM_data>
        {
            using ValueType = std::int64_t;

            static ValueType Read(Dwarf_Attribute attribute);
        };
    }
}
