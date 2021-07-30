cmake_minimum_required(VERSION 3.5)

project(MrUDP)

add_definitions(-DMRUDP_NO_CORE)

if(USE_BOOST)
    include_directories(PRIVATE ${USE_BOOST})
else()
    message( FATAL_ERROR "please specify USE_BOOST=/place/where/boost/is/boost <-- not a relative path!")
endif()

if(USE_OPENSSL)
    include_directories(PRIVATE ${USE_OPENSSL})
else()
    message( FATAL_ERROR "please specify USE_OPENSSL=/place/where/openssl/openssl <-- not a relative path!")
endif()

if (WIN32)
	add_definitions(-DSYS_WINDOWS)
endif ()

if (APPLE)
	add_definitions(-DSYS_MAC)
endif ()

if(UNIX AND NOT APPLE)
	add_definitions(-DSYS_LINUX)
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

############################################################
# Create a library
############################################################

#Generate the static library from the library sources
add_library(MrUDP STATIC 
    mrudp/mrudp.cpp
    mrudp/Base.cpp
    mrudp/Connection.cpp
    mrudp/Crypto.cpp
    mrudp/Handshake.cpp
    mrudp/Packet.cpp
    mrudp/Service.cpp
    mrudp/Socket.cpp
    mrudp/Types.cpp
    mrudp/connection/Probe.cpp
    mrudp/imp/Asio.cpp
    mrudp/imp/Crypto.cpp
    mrudp/receiver/ReceiveQueue.cpp
    mrudp/receiver/Receiver.cpp
    mrudp/sender/Retrier.cpp
    mrudp/sender/Sender.cpp
    mrudp/sender/SendIDGenerator.cpp
    mrudp/sender/SendQueue.cpp
)

target_include_directories(MrUDP
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/mrudp
)


############################################################
# Create an executable
############################################################

# Add an executable with the above sources
add_executable(MrUDP-Tests 
    tests/Basics.cpp
    tests/Connections.cpp
    tests/MaximumTransferRate.cpp
    tests/PacketID.cpp
    tests/StandaloneCore.cpp
    tests/Run.cpp
)

# link the new hello_library target with the hello_binary target
target_link_libraries( MrUDP-Tests
    PRIVATE 
        MrUDP
)
