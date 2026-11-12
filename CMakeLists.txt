cmake_minimum_required(VERSION 3.24)
project(FridgeToTable VERSION 0.1.0 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")
if(EXISTS "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1/algorithm")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-isystem/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1>)
    # JUCE bootstraps juceaide with a nested CMake configure, outside this
    # directory's target options, so pass the Command Line Tools libc++ path
    # through the environment as well.
    set(ENV{CXXFLAGS} "$ENV{CXXFLAGS} -isystem /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1")
endif()

set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../guitar-pan-tone-harmony/external/JUCE" CACHE PATH "Path to JUCE")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found. Install JUCE 8 or configure with -DJUCE_DIR=/path/to/JUCE")
endif()
add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")

juce_add_gui_app(FridgeToTable
    PRODUCT_NAME "FridgeToTable"
    COMPANY_NAME "Swi Kitchen Lab"
    BUNDLE_ID "com.swikitchen.fridgetotable"
    VERSION "0.1.0"
    MICROPHONE_PERMISSION_ENABLED FALSE
    CAMERA_PERMISSION_ENABLED FALSE)

target_sources(FridgeToTable PRIVATE
    Source/Main.cpp
    Source/Domain.cpp
    Source/IngredientCatalog.cpp
    Source/CodexEngine.cpp
    Source/KitchenWindow.cpp
    Source/CocoaShare.mm)

target_include_directories(FridgeToTable PRIVATE Source)
target_compile_definitions(FridgeToTable PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 
    JUCE_REPORT_APP_USAGE=0 JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:FridgeToTable,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:FridgeToTable,JUCE_VERSION>")
target_link_libraries(FridgeToTable PRIVATE
    juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_custom_command(TARGET FridgeToTable POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/bolognese-hero.png"
        "$<TARGET_BUNDLE_DIR:FridgeToTable>/Contents/Resources/bolognese-hero.png")
foreach(asset studio-hero.png ingredient-atlas.png vegetables-through-chard.png)
    add_custom_command(TARGET FridgeToTable POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Resources/${asset}" "$<TARGET_BUNDLE_DIR:FridgeToTable>/Contents/Resources/${asset}")
endforeach()
set_target_properties(FridgeToTable PROPERTIES MACOSX_BUNDLE TRUE)

