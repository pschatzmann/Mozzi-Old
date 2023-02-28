#
# CMake file which builds Mozzi as library or executable in linux or os/X based on the BUILD_EXAMPLE option.
# You must have pulse audio installed.
#
cmake_minimum_required (VERSION 3.11)
project (mozzi)
set (CMAKE_CXX_STANDARD 11)
include(FetchContent)

# Defines the location of the Arduino Sketch
set (ARDUINO_SKETCH ${CMAKE_CURRENT_SOURCE_DIR}/extras/linux/example.cpp )

# Supported Options
option(BUILD_LINUX "Build for Linux" ON)
option(BUILD_EXAMPLE "Build executable for Arduino sketch" OFF)
option(BUILD_SHARED_LIBS "Build with shared libraries" ON)

# Collect all Mozzi CPP files
file(GLOB MOZZI_SRC ${PROJECT_SOURCE_DIR}/*.cpp)

if (BUILD_EXAMPLE)
    # We generate an executable
    add_definitions(-DARDUINO=183 -DNO_LIB)
    add_executable (mozzi ${MOZZI_SRC} ${ARDUINO_SKETCH} ${CMAKE_CURRENT_SOURCE_DIR}/extras/linux/main.cpp )
    install(TARGETS mozzi )

else()
    # We generate an executable
    add_definitions(-DARDUINO=183)
    add_library (mozzi ${MOZZI_SRC})
    install(TARGETS mozzi DESTINATION "lib")
endif()

if(BUILD_LINUX)
    # Build with Linux Arduino Emulator
    FetchContent_Declare(arduino_emulator GIT_REPOSITORY "https://github.com/pschatzmann/Arduino-Emulator.git" GIT_TAG main )
    FetchContent_GetProperties(arduino_emulator)
    if(NOT arduino_emulator_POPULATED)
        FetchContent_Populate(arduino_emulator)
        add_subdirectory(${arduino_emulator_SOURCE_DIR})
    endif()

    # defined includes for Arduino
    include_directories(${CMAKE_CURRENT_SOURCE_DIR} )

    # Add Pulse Audio
    find_library(PULSEAUDIO_LIBRARY NAMES pulse DOC "The PulseAudio library")
    target_link_libraries(mozzi pulse-simple pulse arduino_emulator)

    # Installation target
    install(TARGETS mozzi arduino_emulator DESTINATION "lib")

    # Install Header files
    if(NOT BUILD_EXAMPLE)
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/" # source directory
            DESTINATION "include/mozzi" # target directory
            FILES_MATCHING # install only matched files
            PATTERN "*.h" # select header files
        )

    endif()

else()
    # No linux integration
    include_directories(${CMAKE_CURRENT_SOURCE_DIR} )
    install(TARGETS mozzi DESTINATION "lib")
    # Install Header files
    if(NOT BUILD_EXAMPLE)
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/" # source directory
            DESTINATION "include/mozzi" # target directory
            FILES_MATCHING # install only matched files
            PATTERN "*.h" # select header files
)
endif()

endif()

# Make sure the compiler can find include files 
target_include_directories (mozzi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


