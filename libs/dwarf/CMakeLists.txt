set(NAME dwarf)

add_library(${NAME} STATIC)

target_include_directories(${NAME} PUBLIC include)

target_sources(${NAME} PRIVATE
    src/die.cpp
    src/attributes.cpp
    src/compilationUnit.cpp
    src/debug.cpp
)

target_compile_features(${NAME} PUBLIC cxx_std_17)
#target_compile_definitions(${NAME} PRIVATE LIBDWARF_BUILD)

target_link_libraries(${NAME} PUBLIC
    Microsoft.GSL::GSL
    libdwarf::libdwarf
)