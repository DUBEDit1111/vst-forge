cmake_minimum_required(VERSION 3.25)
project(ToneDrive VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static MSVC runtime (Windows): the plugin carries its own C++ runtime, so it loads on a PC
# without the Visual C++ Redistributable installed.
# Check a build with: dumpbin /dependents <...>.vst3\Contents\x86_64-win\<name>.vst3
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
    set(PLUGIN_FORMATS VST3 AU Standalone)
else()
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

# Offline / faster configure: -DFETCHCONTENT_SOURCE_DIR_JUCE=<path to a JUCE 9 checkout of the same tag>
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.5 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Codes: manufacturer 4 chars, first upper-case; plugin code 4 chars, exactly one upper-case.
# Both are permanent once released (hosts identify the plugin by them).
juce_add_plugin(ToneDrive
    PRODUCT_NAME "ToneDrive"
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Ycmp
    PLUGIN_CODE Tndr
    FORMATS ${PLUGIN_FORMATS}
    AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    VST3_CATEGORIES Fx Distortion
    VST3_AUTO_MANIFEST TRUE)

juce_generate_juce_header(ToneDrive)
target_sources(ToneDrive PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(ToneDrive PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MODAL_LOOPS_PERMITTED=0)
target_link_libraries(ToneDrive
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(ToneDrive PRIVATE /permissive- /utf-8 /MP)
endif()

# Engine oracle: dependency-free console app (no JUCE), the fastest loop there is.
add_executable(EngineTests Tests/EngineTests.cpp)
if(MSVC)
    target_compile_options(EngineTests PRIVATE /permissive- /utf-8 /W4)
else()
    target_compile_options(EngineTests PRIVATE -Wall -Wextra)
endif()

# Host-abuse battery: whole processor + editor under rude-host conditions.
juce_add_console_app(ProcessorTests PRODUCT_NAME "ProcessorTests")
juce_generate_juce_header(ProcessorTests)
target_sources(ProcessorTests PRIVATE Tests/ProcessorTests.cpp)
target_compile_definitions(ProcessorTests PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STANDALONE_APPLICATION=0)
target_link_libraries(ProcessorTests PRIVATE
    ToneDrive
    juce::juce_audio_utils juce::juce_dsp
    juce::juce_recommended_config_flags)
if(MSVC)
    target_compile_options(ProcessorTests PRIVATE /permissive- /utf-8 /MP)
endif()
