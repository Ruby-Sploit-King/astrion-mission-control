cmake_minimum_required(VERSION 3.21)

project(AstrionMissionControl
    VERSION 1.0.0
    DESCRIPTION "Aerospace-inspired telemetry dashboard"
    LANGUAGES CXX 
  )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Network Test)

enable_testing()

add_library(astrion_core 
    src/telemetry.cpp 
    src/telemetry.hpp 
    src/tle.cpp
    src/tle.hpp
    src/sgp4.cpp
    src/sgp4.hpp
    src/astro.cpp
    src/astro.hpp
    src/tlesource.cpp
    src/tlesource.hpp
    src/missioncontroller.cpp
    src/missioncontroller.hpp
)

target_include_directories(astrion_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(astrion_core
    PUBLIC Qt6::Widgets Qt6::Network
)

add_executable(astrion
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.hpp
    src/plotwidget.cpp
    src/plotwidget.hpp
    src/orbitwidget.cpp
    src/orbitwidget.hpp
    data/data.qrc
)

target_link_libraries(astrion
    PRIVATE
        astrion_core
        Qt6::Widgets
        Qt6::Network
)

add_executable(astrion_tests
    tests/test_telemetry.cpp
)

target_link_libraries(astrion_tests
    PRIVATE
        astrion_core
        Qt6::Test
)

add_test(
    NAME astrion_telemetry_tests
    COMMAND astrion_tests
)

set_tests_properties(
    astrion_telemetry_tests
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(astrion_tle_tests
    tests/test_tle.cpp
)

target_link_libraries(astrion_tle_tests
    PRIVATE
        astrion_core
        Qt6::Test
)

add_test(
    NAME astrion_tle_tests
    COMMAND astrion_tle_tests
)

set_tests_properties(
    astrion_tle_tests
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(astrion_sgp4_tests
    tests/test_sgp4.cpp
)

target_link_libraries(astrion_sgp4_tests
    PRIVATE
        astrion_core
        Qt6::Test
)

add_test(
    NAME astrion_sgp4_tests
    COMMAND astrion_sgp4_tests
)

set_tests_properties(
    astrion_sgp4_tests
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
