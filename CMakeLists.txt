cmake_minimum_required(VERSION 3.22)
project(SlikBezzleVocalMaster VERSION 1.3.0)

if(DEFINED JUCE_DIR)
  add_subdirectory(${JUCE_DIR} JUCE)
else()
  # No local JUCE given: download it automatically.
  include(FetchContent)
  FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(SlikBezzleVocalMaster
  COMPANY_NAME "Slik Bezzle"
  COMPANY_WEBSITE "https://juce.com/"
  COMPANY_EMAIL ""
  PLUGIN_MANUFACTURER_CODE Slbz
  PLUGIN_CODE Vmst
  FORMATS VST3 Standalone
  PRODUCT_NAME "SLIK BEZZLE VOCAL MASTER"
  DESCRIPTION "All-in-one vocal processor by Slik Bezzle"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
)

# The sources include <JuceHeader.h>, which CMake builds only when asked.
juce_generate_juce_header(SlikBezzleVocalMaster)

target_sources(SlikBezzleVocalMaster PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h
)

target_compile_features(SlikBezzleVocalMaster PRIVATE cxx_std_17)
target_compile_definitions(SlikBezzleVocalMaster PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(SlikBezzleVocalMaster PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
)
