#include <charconv>
#include <cstdio>
#include <variant>
#include <vector>
#include "Namespace.hpp"
#include "dwarf/attributes.hpp"
#include "dwarf/compilationUnit.hpp"
#include "dwarf/debug.hpp"
#include "dwarf/die.hpp"
#include "elf.hpp"
#include "evaluate_address.hpp"
#include "libdwarf/libdwarf.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <iostream>

using namespace std::string_literals;

struct MemberAccess
{
    std::string name;
    std::uint64_t offset;
};

struct GlobalVariableAccess
{
    std::string name;
    std::uint64_t offset;
};

struct ArrayAccess
{
    std::uint64_t index;
    std::uint64_t offset;
};

using AccessPathItem = std::variant<MemberAccess, ArrayAccess, GlobalVariableAccess>;

static dwarf::Die FindVariableDie(
    const dwarf::DebugInfo& debug,
    const dwarf::Die& topDie,
    std::string_view linkageName,
    std::int64_t address)
{
    for(auto child = dwarf::Child(topDie); static_cast<bool>(child); child = dwarf::Sibling(child))
    {
        auto tag = dwarf::GetTag(child);

        if(tag == dwarf::DieTag::DW_TAG_variable)
        {
            auto name = dwarf::ReadDieAttribute<dwarf::Attribute::DW_AT_linkage_name>(child);
            if(name.has_value())
            {
                if(name.value() == linkageName)
                {
                    return std::move(child);
                }
            }

            auto evaluatedAddress = EvaluateAddress(child);
            if(evaluatedAddress.has_value() && (evaluatedAddress == address))
            {
                return std::move(child);
            }
        }

        auto r = FindVariableDie(debug, child, linkageName, address);
        if(static_cast<bool>(r))
        {
            return r;
        }
    }

    return {};
}

bool ResolveAddressToPath(
    const dwarf::DebugInfo& debug,
    Namespace& root,
    const dwarf::Die& containingType,
    std::int64_t inVariableOffset,
    std::vector<AccessPathItem>& accessPath)
{
    auto typeTag = dwarf::GetTag(containingType);

    if(typeTag == dwarf::DieTag::DW_TAG_base_type)
    {
        return true;
    }

    if(typeTag == dwarf::DieTag::DW_TAG_enumerator)
    {
        return true;
    }

    if(typeTag == dwarf::DieTag::DW_TAG_pointer_type)
    {
        return true;
    }

    if(typeTag == dwarf::DieTag::DW_TAG_union_type)
    {
        // No walking through unit types, sorry
        return true;
    }

    if(typeTag == dwarf::DieTag::DW_TAG_typedef)
    {
        auto nextType = ResolveTypeFromDie(debug, root, containingType);
        return ResolveAddressToPath(debug, root, nextType, inVariableOffset, accessPath);
    }

    if(typeTag == dwarf::DieTag::DW_TAG_array_type)
    {
        auto nextType = ResolveTypeFromDie(debug, root, containingType);
        Dwarf_Error error;
        Dwarf_Off elementSize;
        if(dwarf_bytesize(nextType.get(), &elementSize, &error) != DW_DLV_OK)
        {
            spdlog::error("Failed to get byte size :(");
            exit(1);
        }

        auto elementIndex = inVariableOffset / elementSize;
        auto inElementOffset = inVariableOffset % elementSize;

        spdlog::debug("Array access [{}]+{}", elementIndex, inElementOffset);

        accessPath.push_back(ArrayAccess{elementIndex, inElementOffset});
        return ResolveAddressToPath(debug, root, nextType, inElementOffset, accessPath);
    }

    spdlog::debug(
        "Type: {} Tag: 0x{:02X} Offset: {}",
        dwarf::ReadDieName(containingType).value_or("(unnamed)"),
        static_cast<std::underlying_type_t<decltype(typeTag)>>(typeTag),
        inVariableOffset);

    std::optional<Dwarf_Off> lastEntry = 0;
    for(auto member = dwarf::Child(containingType); static_cast<bool>(member);
        member = dwarf::Sibling(member))
    {
        auto tag = dwarf::GetTag(member);

        if(tag == dwarf::DieTag::DW_TAG_member)
        {
            auto isExternal = dwarf::ReadDieAttribute<dwarf::Attribute::DW_AT_external>(member);

            if(isExternal.value_or(false))
            {
                continue;
            }

            auto location = dwarf::ReadDieAttribute<dwarf::Attribute::DW_AT_data_member_location>(member);

            spdlog::debug(
                "[loop] Member: {} Offset: {}",
                dwarf::ReadDieName(member).value_or(""),
                location.value_or(0));

            if(location.value() > inVariableOffset)
            {
                break;
            }

            lastEntry = dwarf::GetDieOffset(member);
        }
        else if(tag == dwarf::DieTag::DW_TAG_inheritance)
        {
            auto location = dwarf::ReadDieAttribute<dwarf::Attribute::DW_AT_data_member_location>(member);

            if(location.value() > inVariableOffset)
            {
                break;
            }

            lastEntry = dwarf::GetDieOffset(member);
        }
    }

    if(!lastEntry.has_value())
    {
        spdlog::error("Matching member not found");
        return false;
    }

    {
        auto containingDie = dwarf::GetDieByOffset(debug, true, lastEntry.value());
        auto tag = dwarf::GetTag(containingDie);
        auto location =
            dwarf::ReadDieAttribute<dwarf::Attribute::DW_AT_data_member_location>(containingDie);

        if(location > inVariableOffset)
        {
            spdlog::error("Dead end :/");
            return false;
        }

        if(tag == dwarf::DieTag::DW_TAG_member)
        {
            const std::string_view memberName =
                dwarf::ReadDieName(containingDie).value_or("(unnamed)");
            std::uint64_t inMemberOffset = inVariableOffset - location.value();
            spdlog::info("Containing member: {}, offset {}", memberName, inMemberOffset);

            auto memberType = ResolveTypeFromDie(debug, root, containingDie);
            accessPath.push_back(MemberAccess{std::string{memberName}, inMemberOffset});
            return ResolveAddressToPath(debug, root, memberType, inMemberOffset, accessPath);
        }
        else if(tag == dwarf::DieTag::DW_TAG_inheritance)
        {
            auto baseType = ResolveTypeFromDie(debug, root, containingDie);
            spdlog::info(
                "Base class {}, offset {}",
                dwarf::ReadDieName(baseType).value_or("(unnamed)").data(),
                inVariableOffset - location.value());

            return ResolveAddressToPath(
                debug, root, baseType, inVariableOffset - location.value(), accessPath);
        }
        else
        {
            spdlog::error("this is unexpected...");
            return false;
        }
    }
}

static std::optional<std::string> GlobalVariableName(
    const dwarf::DebugInfo& debug,
    const dwarf::Die& topDie,
    const dwarf::Die& variableDie,
    const std::string& path)
{
    for(auto child = dwarf::Child(topDie); static_cast<bool>(child); child = dwarf::Sibling(child))
    {
        if(dwarf::GetDieOffset(child) == dwarf::GetDieOffset(variableDie))
        {
            auto name = dwarf::ReadDieName(child);

            if(name.has_value())
            {
                return path + "::"s + std::string{*name};
            }
            else
            {
                return path;
            }
        }

        switch(dwarf::GetTag(child))
        {
            case dwarf::DieTag::DW_TAG_namespace:
            case dwarf::DieTag::DW_TAG_class_type:
            case dwarf::DieTag::DW_TAG_structure_type:
            {
                auto name = dwarf::ReadDieName(child);

                std::string nestedPath;
                if(name.has_value())
                {
                    nestedPath = path + "::"s + std::string{*name};
                }
                else
                {
                    nestedPath = path;
                }

                auto result = GlobalVariableName(debug, child, variableDie, nestedPath);
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
    return std::nullopt;
}

static void LookupAddress(std::uint32_t lookupAddress, const char* elfFile, dwarf::DebugInfo& dbg, Namespace& root)
{
    spdlog::info("Address for lookup: 0x{:08X}", lookupAddress);

    auto containingSymbol = LookupSymbolByAddress(elfFile, lookupAddress);
    if(!containingSymbol.has_value())
    {
        printf(
            "0x%08X - Failed to match address to symbol\n",
            lookupAddress);
    }

    auto [linkageName, inVariableOffset] = *containingSymbol;

    spdlog::info("Symbol name: {}+{}", linkageName, inVariableOffset);

    spdlog::info("Looking for DIE matching symbol");
    dwarf::Die variableDie{};
    while(dwarf::NavigateToNextCompilationUnit(dbg))
    {
        auto cuDie = dwarf::GetCompilationUnitTopDie(dbg, true);
        spdlog::debug("CU: {}", dwarf::ReadDieName(cuDie).value_or("(unnamed)"));

        variableDie = FindVariableDie(dbg, cuDie, linkageName, lookupAddress - inVariableOffset);
        if(static_cast<bool>(variableDie))
        {
            // iterate to the end of CU list
            while(dwarf::NavigateToNextCompilationUnit(dbg))
                ;
            break;
        }
    }

    if(!static_cast<bool>(variableDie))
    {
        printf(
            "0x%08X - DIE for variable not found. Symbol: %s+%lu\n",
            lookupAddress,
            linkageName.c_str(),
            inVariableOffset);
        return;
    }

    spdlog::debug(
        "Found variable die (offset=0x{:08X}) {}+{}", dwarf::GetDieOffset(variableDie), dwarf::ReadDieName(variableDie).value_or("(unnamed)"), inVariableOffset);

    auto typeDie = ResolveTypeFromDie(dbg, root, variableDie);

    if(!static_cast<bool>(typeDie))
    {
        printf("DIE offset: %llu\n", dwarf::GetDieOffset(variableDie));
        printf(
            "0x%08X - Failed to resolve type DIE for variable. Symbol: %s+%lu\n",
            lookupAddress,
            linkageName.c_str(),
            inVariableOffset);
        return;
    }

    std::vector<AccessPathItem> accessPath;

    auto fullName = GlobalVariableName(
        dbg, dwarf::GetCompilationUnitTopDie(dbg, true, variableDie), variableDie, "");

    accessPath.push_back(GlobalVariableAccess{fullName.value_or("(unnamed)"), inVariableOffset});

    ResolveAddressToPath(dbg, root, typeDie, inVariableOffset, accessPath);

    printf("0x%08X - ", lookupAddress);

    for(const auto& pathItem: accessPath)
    {
        if(std::holds_alternative<GlobalVariableAccess>(pathItem))
        {
            auto& item = std::get<GlobalVariableAccess>(pathItem);

            printf("%s", item.name.c_str());
        }
        else if(std::holds_alternative<MemberAccess>(pathItem))
        {
            auto& item = std::get<MemberAccess>(pathItem);

            printf(".%s", item.name.c_str());
        }
        else if(std::holds_alternative<ArrayAccess>(pathItem))
        {
            auto& item = std::get<ArrayAccess>(pathItem);
            printf("[%ld]", item.index);
        }
    }
    printf("\n");
}

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        fprintf(stderr, "%s <elf file> 0x<address in hex> [0x<address in hex>...]\n", argv[0]);
        fprintf(stderr, "\tResolve addresses given on command line\n");
        fprintf(stderr, "%s <elf file> 0x<address in hex> - \n", argv[0]);
        fprintf(stderr, "\tAccept addresses one-by-one from standard input and resolve them\n");
        return 1;
    }

    auto logger = spdlog::stderr_color_st("root");
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("[%-5l] %v");
    spdlog::set_default_logger(logger);

    spdlog::info("Analyzing ELF {}", argv[1]);

    auto dbg = dwarf::OpenDebugInfo(argv[1]);

    Namespace root{""};

    spdlog::info("Building type tree");
    while(dwarf::NavigateToNextCompilationUnit(dbg))
    {
        auto cuDie = dwarf::GetCompilationUnitTopDie(dbg, true);

        BuildTypeTree(dbg, cuDie, root);
    }

    if (argc == 3 && std::string_view{argv[2]} == "-")
    {
        // Read from stdin
        std::string line;
        while(std::getline(std::cin, line))
        {
            if (line.empty())
            {
                break;
            }
            std::uint32_t lookupAddress = 0;
            {
                std::string_view s{line};
                if(auto [_, ec] = std::from_chars(s.data() + 2, s.data() + s.size(), lookupAddress, 16); ec != std::errc{})
                {
                    printf("Invalid address: %s\n", line.c_str());
                    continue;
                }
            }

            LookupAddress(lookupAddress, argv[1], dbg, root);
        }
    }
    else
    {
        for(auto arg = 2; arg < argc; arg++)
        {
            std::uint32_t lookupAddress = 0;
            {
                std::string_view s{argv[arg]};
                std::from_chars(s.data() + 2, s.data() + s.size(), lookupAddress, 16);
            }

            LookupAddress(lookupAddress, argv[1], dbg, root);
        }
    }
}
