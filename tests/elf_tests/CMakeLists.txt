function(add_elf_test ELF ADDRESS FIELD)
    get_filename_component(ELF_FULL ${ELF} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME "${ELF}/${FIELD}" COMMAND Python3::Interpreter
        ${CMAKE_CURRENT_SOURCE_DIR}/check_field_resolution.py
        --addr2field $<TARGET_FILE:addr2field>
        --elf ${ELF_FULL}
        --address ${ADDRESS}
        --field ${FIELD}
    )
endfunction()

add_elf_test(elfs/pwsat.elf 0x88018e38 ::Main.Storage._storage.Device._device.obj_bucket[116].count)
add_elf_test(elfs/pwsat.elf 0x880f6d0f ::i2cTransfer[0].offset)
