cmake_minimum_required(VERSION 3.22)
project(AtomicButton VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── JUCE (fetched automatically) ─────────────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ── App target ───────────────────────────────────────────
juce_add_gui_app(AtomicButton
    PRODUCT_NAME        "Atomic Button"
    VERSION             "1.0.0"
    COMPANY_NAME        "AtomicButton"
    BUNDLE_ID           "com.atomicbutton.atomicbutton"
    ICON_BIG            ""
)

juce_generate_juce_header(AtomicButton)

# Explicitly expose the generated JuceHeader.h to our own source files.
# juce_generate_juce_header uses INTERFACE visibility, which doesn't cover
# the target's own sources — add PRIVATE so our .cpp/.h files can find it.
target_include_directories(AtomicButton PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/JuceLibraryCode"
)

target_sources(AtomicButton PRIVATE
    Source/Main.cpp
    Source/AudioEngine.cpp
    Source/EditClipComponent.cpp
    Source/KeyboardComponent.cpp
    Source/VUMeterComponent.cpp
    Source/WaveformDisplay.cpp
    Source/MainComponent.cpp
)

if(WIN32)
    set(ASIO_SDK_DIR "" CACHE PATH "Path to Steinberg ASIO SDK (optional)")
    if(NOT "${ASIO_SDK_DIR}" STREQUAL "")
        target_include_directories(AtomicButton PRIVATE "${ASIO_SDK_DIR}/common")
        target_compile_definitions(AtomicButton PRIVATE JUCE_ASIO=1)
        message(STATUS "ASIO enabled: ${ASIO_SDK_DIR}")
    else()
        # Explicitly disable ASIO so the juce_audio_devices module does not try
        # to compile iasiodrv.h (which is absent on CI and non-ASIO machines).
        target_compile_definitions(AtomicButton PRIVATE JUCE_ASIO=0)
        message(STATUS "ASIO disabled (set ASIO_SDK_DIR to enable)")
    endif()
else()
    target_compile_definitions(AtomicButton PRIVATE JUCE_ASIO=0)
endif()

target_compile_definitions(AtomicButton PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_USE_FLAC=1
    JUCE_USE_OGGVORBIS=1
    JUCE_USE_MP3AUDIOFORMAT=0     # Set to 1 if you have a valid MP3 licence
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:AtomicButton,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:AtomicButton,JUCE_VERSION>"
)

target_link_libraries(AtomicButton PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
