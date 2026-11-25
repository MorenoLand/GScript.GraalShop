cmake_minimum_required(VERSION 3.24)
project(GraalShop LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.20
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(SDL3)
FetchContent_Declare(stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG 6b5799bae8a1fe3792691ef4b72ae26434c3e799
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(stb)

add_library(gani_model src/gani_parser.cpp)
target_include_directories(gani_model PUBLIC include)
target_compile_features(gani_model PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(gani_model PRIVATE /W4)
else()
  target_compile_options(gani_model PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(GraalShop src/main.cpp src/sheets.cpp src/canvas.cpp src/editor.cpp src/font.cpp)
target_link_libraries(GraalShop PRIVATE gani_model SDL3::SDL3)
if(WIN32)
  target_link_libraries(GraalShop PRIVATE dwmapi)
  if(NOT EMSCRIPTEN)
    set_target_properties(GraalShop PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
      target_link_options(GraalShop PRIVATE "/ENTRY:mainCRTStartup")
    endif()
    target_sources(GraalShop PRIVATE src/app.rc)
  endif()
endif()
if(EMSCRIPTEN)
  set(WASM_PRELOAD "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets@assets")
  set_target_properties(GraalShop PROPERTIES SUFFIX ".html")
  target_link_options(GraalShop PRIVATE ${WASM_PRELOAD}
    "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/wasm/shell.html"
    "-sALLOW_MEMORY_GROWTH=1" "-sASYNCIFY=1" "-sEXIT_RUNTIME=0" "-sFORCE_FILESYSTEM=1")
endif()
target_include_directories(GraalShop PRIVATE "${stb_SOURCE_DIR}")
if(MSVC)
  target_compile_options(GraalShop PRIVATE /W4 $<$<CONFIG:Release>:/O2 /GL /Gy /Gw /Z0>)
  target_link_options(GraalShop PRIVATE $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO /DEBUG:NONE>)
else()
  target_compile_options(GraalShop PRIVATE -Wall -Wextra -Wpedantic)
endif()
set_target_properties(GraalShop PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
foreach(configuration DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set_target_properties(GraalShop PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${configuration} "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endforeach()
add_custom_command(TARGET GraalShop POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:GraalShop>/assets")

add_executable(gani_tests tests/gani_tests.cpp)
target_link_libraries(gani_tests PRIVATE gani_model)
if(MSVC)
  target_compile_options(gani_tests PRIVATE /W4)
else()
  target_compile_options(gani_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME gani_model_tests COMMAND $<TARGET_FILE:gani_tests>)
