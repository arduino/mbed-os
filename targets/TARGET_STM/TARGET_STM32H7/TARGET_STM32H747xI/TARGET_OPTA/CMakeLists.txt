# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(COMPONENT_WHD EXCLUDE_FROM_ALL)

add_library(mbed-opta INTERFACE)

target_sources(mbed-opta
    INTERFACE
        PeripheralPins.c
        system_clock_override.c
        opta_power.cpp
)

target_include_directories(mbed-opta
    INTERFACE
        .
)

target_link_libraries(mbed-opta INTERFACE mbed-stm32h747xi-cm7)
