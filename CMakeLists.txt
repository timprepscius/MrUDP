cmake_minimum_required(VERSION 3.5)

project(MrUDP)

add_definitions(-DMRUDP_NO_CORE)

if(USE_CRYPTO)
	find_package(OpenSSL REQUIRED)
	target_link_libraries(untitled OpenSSL::SSL)
else()

endif()

if(BOOST_ROOT)
    include_directories(PRIVATE ${BOOST_ROOT})
else()
    find_package(Boost)
    if(Boost_FOUND)
       include_directories(${Boost_INCLUDE_DIRS})       
    else()
       message( FATAL_ERROR "please specify BOOST_ROOT=/place/where/boost/is/boost <-- not a relative path!")
    endif()
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
    mrudp/Handshake.cpp
    mrudp/NetworkPath.cpp
    mrudp/Packet.cpp
    mrudp/Scheduler.cpp
    mrudp/Service.cpp
    mrudp/Socket.cpp
    mrudp/Statistics.cpp
    mrudp/Types.cpp
    mrudp/connection/Probe.cpp
    mrudp/imp/Asio.cpp
    mrudp/receiver/ReceiveQueue.cpp
    mrudp/receiver/Receiver.cpp
    mrudp/sender/Retrier.cpp
    mrudp/sender/Sender.cpp
    mrudp/sender/SendQueue.cpp
    mrudp/receiver/UnreliableReceiveQueue.cpp
)

if(USE_CRYPTO)
	target_sources(MrUDP PRIVATE
		mrudp/Crypto.cpp
		mrudp/imp/CryptoOpenSSL.cpp
	)
else()

endif()

target_include_directories(MrUDP
    PUBLIC 
        ${PROJECT_SOURCE_DIR}
)


############################################################
# Create an executable
############################################################

# Add an executable with the above sources
add_executable(MrUDP-Tests 
    tests/Basics.cpp
    tests/Connections.cpp
    tests/ConnectionTimesOutAtBeginning.cpp
    tests/MaximumTransferRate.cpp
    tests/NetworkPathChange.cpp
    tests/PacketID.cpp
    tests/StandaloneCore.cpp
    tests/Streams.cpp
    tests/Run.cpp
)

if(USE_CRYPTO)
	target_sources(MrUDP-Tests PRIVATE
		tests/Crypto.cpp
	)
else()
endif()


# Add an executable with the above sources
add_executable(MrUDP-Examples
    examples/Run.cpp
    examples/EchoClient.cpp
    examples/EchoClient.c
    examples/EchoServer.cpp
    examples/EchoServer.c
)

if(USE_CRYPTO)
	target_sources(MrUDP-Examples PRIVATE
		tests/Crypto.cpp
	)
else()
endif()

# link the new hello_library target with the hello_binary target
target_link_libraries( MrUDP-Tests
    PRIVATE 
        MrUDP
)

# link the new hello_library target with the hello_binary target
target_link_libraries( MrUDP-Examples
    PRIVATE
        MrUDP
)
