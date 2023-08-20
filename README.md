# addr2field

`addr2field` tool find exact field in global variable that corresponds to given absolute address. 

Tools like GDB or `addr2line` can give symbol and offset within it from specified address. Such information is not particular useful when symbol matches to composition root of application which can be quite large structure. In such cases `addr2field` will return full path to field, starting from global (or static) variable, through all nested structures until field that can no longer by divided (some primitive type like int or enum).

## Usage
`addr2field` is command line program which lookups one or more addresses (as hex numbers) in specified ELF file. It prints full path to field for each address:

```shell
shell$ addr2field <elf file> <address1> <address2>...
[info ] Analyzing ELF <elf file>
[info ] Building type tree
[info ] Address for lookup: <address1>
[info ] Symbol name: SomeSymbol+2912
[info ] Looking for DIE matching symbol
[debug] CU: main.cpp
[debug] Found variable die (offset=0x0005BA26) SomeSymbol+2912
[debug] Type: Sometype Tag: 0x13 Offset: 2912
[debug] [loop] Member: Member1 Offset: 0
[debug] [loop] Member: Member2 Offset: 8
... (more debug output)
<address1> - ::SomeSymbol.Member1.Member2.count
<address2> - ::SomeSymbol.Member2.Member2.count
```

Type tree (reconstruction of available types from DWARF information) must be built for each run and can take some time (dozens of seconds for large programs). After that lookups for each address are quite fast.

## Limitations
* `addr2field` is aimed for statically linked ELF files (no relocations)
* Tested only on 32-bit little-endian executables.

## This stuff is not working!
DWARF debug information (which this tool relies on) is quite complicated as well as source language used to built executable that is analyzed. I found it difficult to prepare exhaustive test suite as simple programs fails to approximate structure of information that can be encountered in real world complex applications. For example, single file programs can be analyzed much easier that executable involving multiple translation units and static libraries.

If you found use case where `addr2field` fails to resolve address correctly or crashes, please open issue with example program and address to lookup. Most edge cases are quite easy to fix, I just need to have concrete example to test against. 

## Tutorial
1. Let's start with following program:
    ```c++
    #include <cstdint>
    struct A {
        int32_t a;
        int32_t b;
        int32_t c;
    };
    
    struct B {
        A d1;
        A d2;
    };
    
    struct C {
        B elements[4]; 
    };
    
    struct Main {
        C c1;
        C c2;
    };
    
    Main Root{};
    
    int main() {
        // not really important
        return Root.c1.elements[3].d1.c;
    }
    ```

2. Compile it using bare metal ARM toolchain: `arm-none-eabi-g++ -mcpu=cortex-m3 -specs=nosys.specs -g -o test.elf main.cpp`
3. Use GDB to retrieve address to lookup (in real world example this value would come from running program and stumbling upon address in e.g. pointer variable or register):
    ```shell
    shell$ gdb ./test.elf
    (gdb) p &Root.c1.elements[3].d2.b
    $1 = (int32_t *) 0x18860 <Root+88>
    ```
    In this example address we will lookup is 0x18860
4. Run `addr2field` with discovered address
    ```shell
    shell$ addr2field test.elf 0x18860
    [info ] Analyzing ELF test.elf
    [info ] Building type tree
    [info ] Address for lookup: 0x00018860
    [info ] Symbol name: Root+88
    [info ] Looking for DIE matching symbol
    [debug] CU: main.cpp
    [debug] Found variable die (offset=0x00000469) Root+88
    [debug] Type: Main Tag: 0x13 Offset: 88
    [debug] [loop] Member: c1 Offset: 0
    [debug] [loop] Member: c2 Offset: 96
    [info ] Containing member: c1, offset 88
    [debug] Type: C Tag: 0x13 Offset: 88
    [debug] [loop] Member: elements Offset: 0
    [info ] Containing member: elements, offset 88
    [debug] Array access [3]+16
    [debug] Type: B Tag: 0x13 Offset: 16
    [debug] [loop] Member: d1 Offset: 0
    [debug] [loop] Member: d2 Offset: 12
    [info ] Containing member: d2, offset 4
    [debug] Type: A Tag: 0x13 Offset: 4
    [debug] [loop] Member: a Offset: 0
    [debug] [loop] Member: b Offset: 4
    [debug] [loop] Member: c Offset: 8
    [info ] Containing member: b, offset 0
    0x00018860 - ::Root.c1.elements[3].d2.b
    ```
   
    Last line of output shows full path to field: `::Root.c1.elements[3].d2.b`. This is exactly the same path as we used previously in GDB.

## Building
`addr2field` can be build using Conan which will pull all necessary dependencies. Customize `build_type` settings and `shared` option to your needs.

**Note:** Exact IDs in output might be different depending on your system.

### Build `libdwarf` package (temporary)
**Note:** This step is temporary. I will try to upstream `libdwarf/0.7.0` recipe to Conan Center Index.

Build `libdwarf` package using Conan:
```shell
addr2field$ cd dependencies/libdwarf
addr2field/dependencies/libdwarf $ conan create -pr:h default -pr:b default -s:h build_type=Release -o:h libdwarf:shared=False . libdwarf/0.7.0@novakov/local
(lots of output)
libdwarf/0.7.0@novakov/local: Package 'b4bd3fe2ab7a51154f326d431f57daf7b2b31fb5' created
libdwarf/0.7.0@novakov/local: Created package revision f1430b6431c49e253e041fffc5a1e73e
(more output)
libdwarf/0.7.0@novakov/local (test package): Running test()
```

### Build `addr2field`
Build `addr2field` using Conan:
```shell
addr2field$ conan install -pr:h default -pr:b default -s:h build_type=Release -s:b build_type=Release -o:h *:shared=False --install-folder build --output-folder build conanfile.py
(lots of output)
conanfile.py (addr2field/1.0.0): Generator 'CMakeDeps' calling 'generate()'
conanfile.py (addr2field/1.0.0): Generator 'VirtualBuildEnv' calling 'generate()'
conanfile.py (addr2field/1.0.0): Generator txt created conanbuildinfo.txt
conanfile.py (addr2field/1.0.0): Aggregating env generators
conanfile.py (addr2field/1.0.0): Generated conaninfo.txt
conanfile.py (addr2field/1.0.0): Generated graphinfo
addr2field$ source ./build/build/generators/conanbuild.sh # (Bash)
addr2field$ ./build/build/generators/conanbuild.ps1 # (Powershell)
addr2field$ ./build/build/generators/conanbuild.bat # (CMD)
addr2field$ cmake --preset default (Visual Studio)
addr2field$ cmake --preset release (GCC, Clang)
addr2field$ cmake --build --preset release
```
