cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

# project(LoRa_pico_lib)

add_library(LoRa_pico_lib
 			LoRa-RP2040.cpp
 			LoRa-RP2040.h
 			bmp280_i2c.cpp
 			bmp280_i2c.h
 			Print.h
 			Print.cpp
 			ssd1306_i2c.h
 			ssd1306_i2c.c
 			)
 			
add_library(LoRa_print Print.h Print.cpp)


target_link_libraries(LoRa_pico_lib 
	pico_stdlib
	hardware_spi			## for spi port
	hardware_i2c			## for i2c port		
	hardware_interp			## for interuptions
	LoRa_print				## for loRa
	FreeRTOS-Kernel-Heap4   ## for freeRTOS 
	)

 # enable usb output, disable uart output
 pico_enable_stdio_usb(LoRa_pico_lib 1)
 pico_enable_stdio_uart(LoRa_pico_lib 0)
