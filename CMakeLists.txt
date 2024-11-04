cmake_minimum_required(VERSION 3.22.1)
project(scheduler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic -fsanitize=address")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_library(scheduler INTERFACE)

target_include_directories(scheduler INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

include(GNUInstallDirs)
install(TARGETS scheduler EXPORT SchedulerConfig)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT SchedulerConfig
    NAMESPACE scheduler::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Scheduler
)

add_executable(test_executable test_executable.cc)
target_link_libraries(test_executable PRIVATE scheduler)
