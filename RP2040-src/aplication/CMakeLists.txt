cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

#  Node End-poind -> Send data
# add_subdirectory(LoRaSender)

# Node Gateway -> Receive data
add_subdirectory(LoRaReceiver)
