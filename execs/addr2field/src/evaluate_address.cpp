#include "evaluate_address.hpp"
#include "dwarf/attributes.hpp"
#include "dwarf/die.hpp"

std::optional<std::int64_t> EvaluateAddress(const dwarf::Die& die)
{
    Dwarf_Error error;

    Dwarf_Attribute locationAttribute;
    {
        if(dwarf_attr(die.get(), dwarf::Attribute::DW_AT_Location, &locationAttribute, &error) != DW_DLV_OK)
        {
            return std::nullopt;
        }
    }

    Dwarf_Loc_Head_c loclistHead = nullptr;
    Dwarf_Unsigned entryCount = 0;
    dwarf_get_loclist_c(locationAttribute, &loclistHead, &entryCount, &error);

    Dwarf_Half attributeForm;
    if(dwarf_whatform(locationAttribute, &attributeForm, &error) != DW_DLV_OK)
    {
        return std::nullopt;
    }

    if(attributeForm == 0x17) // DW_FORM_sec_offset
    {
        return std::nullopt;
    }

    if(loclistHead == nullptr)
    {
        return std::nullopt;
    }

    for(Dwarf_Unsigned i = 0; i < entryCount; i++)
    {
        Dwarf_Small loclist_lkind = 0;
        Dwarf_Small lle_value = 0;
        Dwarf_Unsigned rawval1 = 0;
        Dwarf_Unsigned rawval2 = 0;
        Dwarf_Bool debug_addr_unavailable = false;
        Dwarf_Addr lopc = 0;
        Dwarf_Addr hipc = 0;
        Dwarf_Unsigned loclist_expr_op_count = 0;
        Dwarf_Locdesc_c locdesc_entry = 0;
        Dwarf_Unsigned expression_offset = 0;
        Dwarf_Unsigned locdesc_offset = 0;

        dwarf_get_locdesc_entry_d(
            loclistHead,
            i,
            &lle_value,
            &rawval1,
            &rawval2,
            &debug_addr_unavailable,
            &lopc,
            &hipc,
            &loclist_expr_op_count,
            &locdesc_entry,
            &loclist_lkind,
            &expression_offset,
            &locdesc_offset,
            &error);

        //        printf("Variable location %d/%d locationsL op count=%d\n", i + 1, entryCount, loclist_expr_op_count);

        for(Dwarf_Unsigned j = 0; j < loclist_expr_op_count; j++)
        {
            Dwarf_Unsigned opd1 = 0;
            Dwarf_Unsigned opd2 = 0;
            Dwarf_Unsigned opd3 = 0;
            Dwarf_Unsigned offsetforbranch = 0;
            Dwarf_Small op;

            dwarf_get_location_op_value_c(
                locdesc_entry, j, &op, &opd1, &opd2, &opd3, &offsetforbranch, &error);

            //            printf(
            //                "Variable location %d/%d locationsL op %d/%d opd=(0x%X,0x%X,0x%X)
            //                op=%d\n", i + 1, entryCount, j + 1, loclist_expr_op_count, opd1, opd2,
            //                opd3,
            //                op);

            if(op == 0x03) // DW_OP_addr
            {
                return opd1;
            }
        }
    }

    dwarf_dealloc_loc_head_c(loclistHead);

    return std::nullopt;
}