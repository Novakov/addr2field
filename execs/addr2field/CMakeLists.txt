set(NAME addr2field)

add_executable(${NAME})

target_include_directories(${NAME} PRIVATE include)

target_sources(${NAME} PRIVATE
    src/elf.cpp
    src/evaluate_address.cpp
    src/main.cpp
    src/Namespace.cpp
)

target_link_libraries(${NAME} PUBLIC 
    dwarf
    spdlog::spdlog
)

install(TARGETS ${NAME})
