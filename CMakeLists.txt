cmake_minimum_required(VERSION 3.28)
project(Connect4Learning LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(main src/main.cpp)
if(WIN32)
    set_target_properties(main PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
    if(MSVC)
        target_link_options(main PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
    endif()
endif()
target_compile_features(main PRIVATE cxx_std_20)
target_link_libraries(main PRIVATE SFML::Graphics)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)


# Copia la carpeta Assets a la carpeta donde se genera el ejecutable (.exe)
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
    "$<TARGET_FILE_DIR:main>/Assets"
    VERBATIM
)