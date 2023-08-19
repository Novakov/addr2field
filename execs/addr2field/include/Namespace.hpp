#pragma once
#include <map>
#include <string_view>
#include "dwarf/die.hpp"
#include "libdwarf/libdwarf.h"

class Namespace
{
public:
    Namespace(const Namespace&) = delete;

    Namespace(std::string_view name);

    Namespace& GetNested(std::string_view name);
    Dwarf_Off GetTypeDieOffsetByName(std::string_view name);

    void RegisterType(std::string_view name, Dwarf_Off offset);

    void Print(int indent = 0) const;

private:
    std::map<std::string_view, Namespace> _innerNamespaces;
    std::map<std::string_view, Dwarf_Off> _types;
    std::string_view _name;
};

void BuildTypeTree(const dwarf::DebugInfo& debug, const dwarf::Die& die, Namespace& parent);

dwarf::Die LookupTypeDieByPath(
    const dwarf::DebugInfo& debug,
    Namespace& container,
    const dwarf::Die& topDie,
    const dwarf::Die& typeDie);

dwarf::Die ResolveTypeFromDie(const dwarf::DebugInfo& debug, Namespace& root, const dwarf::Die& die);