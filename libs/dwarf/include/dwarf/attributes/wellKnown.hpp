#pragma once
#include "dwarf/attributes.hpp"

namespace dwarf
{
    namespace details
    {
        template <> struct KnownAttributeDescriptor<Attribute::DW_AT_NAME>
        {
            static constexpr auto Form = FormType::DW_FORM_strp;
        };

        template <> struct KnownAttributeDescriptor<Attribute::DW_AT_linkage_name>
        {
            static constexpr auto Form = FormType::DW_FORM_string;
        };

        template <> struct KnownAttributeDescriptor<Attribute::DW_AT_declaration>
        {
            static constexpr auto Form = FormType::DW_FORM_flag;
        };

        template <> struct KnownAttributeDescriptor<Attribute::DW_AT_data_member_location>
        {
            static constexpr auto Form = FormType::DW_FORM_data;
        };

        template <> struct KnownAttributeDescriptor<Attribute::DW_AT_external>
        {
            static constexpr auto Form = FormType::DW_FORM_flag;
        };
    }
}