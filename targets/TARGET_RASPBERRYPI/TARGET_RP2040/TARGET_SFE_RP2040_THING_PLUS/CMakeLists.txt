# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

set(LINKER_FILE ../TOOLCHAIN_GCC_ARM/memmap_default.ld)

add_library(mbed-sfe-rp2040-thing-plus INTERFACE)

target_sources(mbed-sfe-rp2040-thing-plus
    INTERFACE
        board.c
        bs2_default_padded_checksummed.S
)

target_include_directories(mbed-sfe-rp2040-thing-plus
    INTERFACE
        .
)

mbed_set_linker_script(mbed-sfe-rp2040-thing-plus ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_FILE})

target_link_libraries(mbed-sfe-rp2040-thing-plus INTERFACE mbed-rp2040)
