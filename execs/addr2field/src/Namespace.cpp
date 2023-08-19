#include "Namespace.hpp"
#include "dwarf/attributes.hpp"
#include "dwarf/debug.hpp"
#include "gsl/assert"
#include <cstdio>

Namespace::Namespace(std::string_view name) : _name{name}
{
}

Namespace& Namespace::GetNested(std::string_view name)
{
    auto it = _innerNamespaces.find(name);
    if(it != _innerNamespaces.end())
    {
        return it->second;
    }

    return _innerNamespaces.emplace(name, name).first->second;
}

void Namespace::RegisterType(std::string_view name, Dwarf_Off offset)
{
    _types[name] = offset;
}

void Namespace::Print(int indent) const
{
    for(int i = 0; i < indent; i++)
    {
        printf("  ");
    }
    printf("%s\n", _name.data());

    for(const auto& item: _innerNamespaces)
    {
        item.second.Print(indent + 1);
    }

    for(const auto& item: _types)
    {
        for(int i = 0; i < indent; i++)
        {
            printf("  ");
        }
        printf("Type: %s DIE: 0x%llX\n", item.first.data(), item.second);
    }
}

Dwarf_Off Namespace::GetTypeDieOffsetByName(std::string_view name)
{
    auto it = _types.find(name);
    Expects(it != _types.end());
    return it->second;
}

void BuildTypeTree(const dwarf::DebugInfo& debug, const dwarf::Die& die, Namespace& parent)
{
    for(auto child = dwarf::Child(die); static_cast<bool>(child); child = dwarf::Sibling(child))
    {
        auto tag = dwarf::GetTag(child);

        if(tag == dwarf::DieTag::DW_TAG_namespace)
        {
            auto name = dwarf::ReadDieName(child);
            if(name.has_value())
            {
                //                printf("Found namespace! %s\n", name.value().data());
                auto& nested = parent.GetNested(name.value());
                BuildTypeTree(debug, child, nested);
            }
            else
            {
                //                printf("Found anonymous namespace!\n");
                BuildTypeTree(debug, child, parent);
            }
        }
        else if((tag == dwarf::DieTag::DW_TAG_class_type) || (tag == dwarf::DieTag::DW_TAG_structure_type))
        {
            auto isDeclaration =
                dwarf::ReadDieAttribute<dwarf::Attribute::DW_AT_declaration>(child).value_or(false);

            if(isDeclaration)
            {
                continue;
            }

            auto name = dwarf::ReadDieName(child);
            if(!name.has_value())
            {
                continue;
            }
            //            printf("Found type %s\n", name.data());
            auto offset = dwarf::GetDieOffset(child);
            parent.RegisterType(name.value(), offset);

            auto& nested = parent.GetNested(name.value());
            BuildTypeTree(debug, child, nested);
        }
    }
}

dwarf::Die LookupTypeDieByPath(
    const dwarf::DebugInfo& debug,
    Namespace& container,
    const dwarf::Die& topDie,
    const dwarf::Die& typeDie)
{
    for(auto child = dwarf::Child(topDie); static_cast<bool>(child); child = dwarf::Sibling(child))
    {
        if(dwarf::GetDieOffset(child) == dwarf::GetDieOffset(typeDie))
        {
            auto name = dwarf::ReadDieName(child);
            auto type = container.GetTypeDieOffsetByName(name.value());
            return dwarf::GetDieByOffset(debug, true, type);
        }

        switch(dwarf::GetTag(child))
        {
            case dwarf::DieTag::DW_TAG_namespace:
            case dwarf::DieTag::DW_TAG_class_type:
            case dwarf::DieTag::DW_TAG_structure_type:
            {
                auto name = dwarf::ReadDieName(child);
                dwarf::Die result{};
                if(name.has_value())
                {
                    result = LookupTypeDieByPath(debug, container.GetNested(name.value()), child, typeDie);
                }
                else
                {
                    result = LookupTypeDieByPath(debug, container, child, typeDie);
                }
                if(static_cast<bool>(result))
                {
                    return result;
                }
                break;
            }
            default:
                break;
        }
    }

    return {};
}

dwarf::Die ResolveTypeFromDie(const dwarf::DebugInfo& debug, Namespace& root, const dwarf::Die& die)
{
    auto directTypeDie = dwarf::ReadDieType(die);

    Dwarf_Error error;
    Dwarf_Off globalOffset;
    Dwarf_Off localOffset;
    dwarf_die_offsets(directTypeDie.get(), &globalOffset, &localOffset, &error);

    auto typeTag = dwarf::GetTag(directTypeDie);
    if(typeTag == dwarf::DieTag::DW_TAG_typedef)
    {
        return ResolveTypeFromDie(debug, root, directTypeDie);
    }

    auto isDeclaration = dwarf::ReadDieAttribute<dwarf::Attribute::DW_AT_declaration>(directTypeDie);
    if(isDeclaration.has_value() && *isDeclaration)
    {
        auto cuTop = dwarf::GetCompilationUnitTopDie(debug, true, directTypeDie);
        return LookupTypeDieByPath(debug, root, cuTop, directTypeDie);
    }

    return directTypeDie;
}