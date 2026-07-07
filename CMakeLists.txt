cmake_minimum_required(VERSION 3.25)

# ---- macOS: universal binary + deployment target (MUST be set before project()) ----
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build a universal (arm64 + x86_64) binary")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum supported macOS version")
endif()

project(DetailForge VERSION 0.12.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Faust engine (portable, Faust-free build) ----
# The generated C++ engine is COMMITTED at Source/generated/ClipEngine.h and the handful of Faust
# architecture headers it needs are vendored under ThirdParty/faust. So neither a normal build nor
# CI depends on a Faust install. Developers who edit Source/dsp/clip.dsp regenerate the committed
# header with the manual target `regen_faust` (requires `faust` on PATH):
#     cmake --build --preset default --target regen_faust
set(CLIP_DSP       "${CMAKE_CURRENT_SOURCE_DIR}/Source/dsp/clip.dsp")
set(CLIP_ENGINE    "${CMAKE_CURRENT_SOURCE_DIR}/Source/generated/ClipEngine.h")  # committed artifact
set(FAUST_ARCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/faust")                # vendored headers

find_program(FAUST_EXECUTABLE faust)
if(FAUST_EXECUTABLE)
    add_custom_target(regen_faust
        COMMAND "${FAUST_EXECUTABLE}" -lang cpp -cn ClipEngine "${CLIP_DSP}" -o "${CLIP_ENGINE}"
        BYPRODUCTS "${CLIP_ENGINE}"
        COMMENT "Faust: regenerating committed Source/generated/ClipEngine.h from clip.dsp"
        VERBATIM)
endif()

# ---- Dependencies (via CMake FetchContent) ----
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---- The plugin target ----
# AU is built by JUCE on macOS only; VST3 + Standalone build on every platform.
# WebView2 is a Windows-only concern — inject NEEDS_WEBVIEW2 only there (macOS uses WKWebView).
set(DF_WEBVIEW2_ARG "")
if(WIN32)
    set(DF_WEBVIEW2_ARG NEEDS_WEBVIEW2 TRUE)
endif()

juce_add_plugin(DetailForge
    PRODUCT_NAME "Detail Forge"
    COMPANY_NAME "Fayella"
    BUNDLE_ID com.fayella.detailforge
    PLUGIN_MANUFACTURER_CODE Fyla
    PLUGIN_CODE Dtfg
    FORMATS AU VST3 Standalone
    VST3_CATEGORIES Fx Dynamics
    AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    ${DF_WEBVIEW2_ARG}
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(DetailForge PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# ---- Embed the WebView UI + the JUCE frontend JS as binary data ----
set(JUCE_JS_DIR "${juce_SOURCE_DIR}/modules/juce_gui_extra/native/javascript")
configure_file("${JUCE_JS_DIR}/index.js"
               "${CMAKE_CURRENT_BINARY_DIR}/ui/juce_index.js" COPYONLY)
configure_file("${JUCE_JS_DIR}/check_native_interop.js"
               "${CMAKE_CURRENT_BINARY_DIR}/ui/juce_check_native_interop.js" COPYONLY)

juce_add_binary_data(DetailForgeData SOURCES
    Source/ui/index.html
    "${CMAKE_CURRENT_BINARY_DIR}/ui/juce_index.js"
    "${CMAKE_CURRENT_BINARY_DIR}/ui/juce_check_native_interop.js")

# Portable defs (both platforms use the WebView UI).
target_compile_definitions(DetailForge PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Windows-only: use the statically-linked WebView2 loader (pairs with NEEDS_WEBVIEW2).
if(WIN32)
    target_compile_definitions(DetailForge PUBLIC
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    )
endif()

target_include_directories(DetailForge PRIVATE
    Source
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/generated"
    "${FAUST_ARCH_DIR}")

target_link_libraries(DetailForge PRIVATE
    DetailForgeData
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ---- JUCE-free measurement runner (drives the real ClipEngine offline) ----
add_executable(engine_runner Tests/engine_runner.cpp)
target_include_directories(engine_runner PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/generated"
    "${FAUST_ARCH_DIR}")
set_target_properties(engine_runner PROPERTIES CXX_STANDARD 20)

# ---- JUCE-free M2 limiter gate (drives the real TruePeakLimiter offline) ----
add_executable(limiter_runner Tests/limiter_runner.cpp)
target_include_directories(limiter_runner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set_target_properties(limiter_runner PROPERTIES CXX_STANDARD 20)

# ---- Automated fidelity gates (ctest) ----
enable_testing()
add_test(NAME m0_gate
         COMMAND ${CMAKE_COMMAND} -E env PYTHON=python
                 python "${CMAKE_CURRENT_SOURCE_DIR}/scripts/m0_gate.py"
                 --runner $<TARGET_FILE:engine_runner>)
set_tests_properties(m0_gate PROPERTIES LABELS "measurement")

# M2: the limiter's true-peak ceiling must hold on an inter-sample signal (self-contained, no args).
add_test(NAME m2_gate COMMAND limiter_runner)
set_tests_properties(m2_gate PROPERTIES LABELS "measurement")
