cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

if (TARGET tinyusb_device)
    project(LoRaGateway)

    add_executable(LoRaGateway
        LoRaGateway.cpp
    )

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    target_include_directories(LoRaGateway PUBLIC ../../src)

    target_link_directories(LoRaGateway PUBLIC ../../src)

    target_link_libraries(LoRaGateway pico_stdlib LoRa_pico_lib)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(LoRaGateway 1)
    pico_enable_stdio_uart(LoRaGateway 0)

    # create map/bin/hex/uf2 file in addition to ELF.
    pico_add_extra_outputs(LoRaGateway)

    # example_auto_set_url(LoRaGateway)

    elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()
