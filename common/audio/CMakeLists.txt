add_library(audio STATIC
  AudioCapturePulse.cxx
  AudioCaptureWin.cxx
)

target_include_directories(audio PUBLIC ${CMAKE_SOURCE_DIR}/common)

if(UNIX AND NOT APPLE)
  find_package(PkgConfig QUIET)
  pkg_check_modules(PULSEAUDIO libpulse-simple)
  if(PULSEAUDIO_FOUND)
    target_compile_definitions(audio PRIVATE HAVE_PULSEAUDIO)
    target_include_directories(audio SYSTEM PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
    target_link_libraries(audio ${PULSEAUDIO_LIBRARIES})
  endif()
endif()

if(WIN32)
  target_link_libraries(audio ole32.lib winmm.lib)
endif()
