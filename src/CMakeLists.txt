file(GLOB_RECURSE FALCON_CORE_HEADERS
  ${CMAKE_SOURCE_DIR}/include/falcon/core/*.h
  ${CMAKE_SOURCE_DIR}/include/falcon/core/*.hpp
  ${CMAKE_SOURCE_DIR}/include/falcon/util/*.h
  ${CMAKE_SOURCE_DIR}/include/falcon/util/*.hpp
)

set(FALCON_CORE_SOURCES
  core/Image.cpp
  core/Binarize.cpp
  core/Segment.cpp
  core/Normalize.cpp
  core/Features.cpp
  core/Classifier.cpp
  core/GlyphDB.cpp
  util/Timer.cpp
  util/String.cpp
  ocr/Pipeline.cpp
)

add_library(falcon_core ${FALCON_CORE_SOURCES} ${FALCON_CORE_HEADERS})
target_include_directories(falcon_core
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(falcon_core PUBLIC cxx_std_17)

set(FALCON_APP_SOURCES
  app/WinMain.cpp
  app/MainWindow.cpp
)

add_executable(falcon_app ${FALCON_APP_SOURCES})
target_include_directories(falcon_app
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(falcon_app PRIVATE falcon_core)

if(WIN32)
  target_compile_definitions(falcon_app PRIVATE UNICODE)
  target_link_libraries(falcon_app PRIVATE user32 gdi32 comdlg32)
endif()
