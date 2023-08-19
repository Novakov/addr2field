#include "elf.hpp"
#include <memory>
#include "libelf/libelf.h"
#include <cstring>

struct FileHandleDeleter
{
    void operator()(FILE* f)
    {
        fclose(f);
    }
};

using FileHandle = std::unique_ptr<FILE, FileHandleDeleter>;

struct ElfDeleter
{
    FileHandle UnderlyingFile;

    void operator()(Elf* elf)
    {
        elf_end(elf);
    }
};

using ElfHandle = std::unique_ptr<Elf, ElfDeleter>;

static FileHandle OpenFile(const char* fileName, const char* mode)
{
    FILE* f = fopen(fileName, mode);
    if(f == nullptr)
    {
        return {};
    }
    return {f, {}};
}

static ElfHandle OpenAsElf(FileHandle handle)
{
    auto h = elf_begin(fileno(handle.get()), Elf_Cmd::ELF_C_READ, nullptr);
    return {h, {std::move(handle)}};
}

std::optional<std::tuple<std::string, std::uint64_t>>
    LookupSymbolByAddress(const char* elfFile, std::uint32_t address)
{
    auto f = OpenFile(elfFile, "rb");

    elf_version(EV_CURRENT);
    auto elf = OpenAsElf(std::move(f));
    Elf_Scn* symbolTableSection = nullptr;
    for(auto section = elf_nextscn(elf.get(), nullptr); section != nullptr;
        section = elf_nextscn(elf.get(), section))
    {
        auto header = elf32_getshdr(section);
        if(header->sh_type == SHT_SYMTAB)
        {
            symbolTableSection = section;
            break;
        }
    }

    if(symbolTableSection == nullptr)
    {
        return std::nullopt;
    }

    auto symbolSectionHeader = elf32_getshdr(symbolTableSection);
    auto stringTableSection = elf_getscn(elf.get(), symbolSectionHeader->sh_link);

    if(stringTableSection == nullptr)
    {
        return std::nullopt;
    }

    auto stringData = elf_getdata(stringTableSection, nullptr);

    auto symbolData = elf_getdata(symbolTableSection, nullptr);
    auto symbolSize = symbolSectionHeader->sh_entsize;
    auto symbolsCount = symbolData->d_size / symbolSize;

    for(int i = 0; i < symbolsCount; i++)
    {
        Elf32_Sym sym;
        memcpy(&sym, ((const std::byte*)symbolData->d_buf) + i * symbolSize, symbolSize);

        auto symbolName = static_cast<const char*>(stringData->d_buf) + sym.st_name;

        if(ELF32_ST_BIND(sym.st_info) != STB_GLOBAL)
        {
            continue;
        }

        if((sym.st_value <= address) && (address <= (sym.st_value + sym.st_size)))
        {
            return std::make_tuple(std::string{symbolName}, address - sym.st_value);
        }
    }

    return std::nullopt;
}
