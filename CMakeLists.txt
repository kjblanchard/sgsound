cmake_minimum_required(VERSION 3.5)
option(INSTALL_THE_LIB "install include folders and stuff for local dev" ON)
option(SYSTEM_PACKAGES "Use system packages if available" ON)
option(BUILD_TEST "Test executable" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LIBRARY_TARGET_NAME sgsound)
project(${LIBRARY_TARGET_NAME} LANGUAGES C VERSION 1.0)
message(STATUS "Starting ${LIBRARY_TARGET_NAME} initialization")
set(COMPILE_OPTIONS)
set(EXTERNAL_LIBS)
# Should we show everything, or just a few.
set(CMAKE_INSTALL_MESSAGE LAZY)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libraries as static" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(NOT (APPLE AND (CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "iossimulator")) AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Emscripten"))
    message(STATUS "Enabling AddressSanitizer in Debug mode")
    set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ASAN_FLAGS}")
  endif()
endif()


# Build system Specific
if(CMAKE_GENERATOR STREQUAL "Xcode")
    message(STATUS "Setting Build to Universal Binary")
    set(CMAKE_OSX_ARCHITECTURES
        "x86_64;arm64"
        CACHE STRING "" FORCE)
endif(CMAKE_GENERATOR STREQUAL "Xcode")
if(DEBUG_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT (APPLE AND (CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "iossimulator")) AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Emscripten"))
        message(STATUS "Enabling AddressSanitizer in Debug mode")
        set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ASAN_FLAGS}")
    endif()
endif()

# Platform specific executable flags
if(EMSCRIPTEN)
    message(STATUS "Enabling Emscripten flags for sgsound ")
    set(CMAKE_EXECUTABLE_SUFFIX
        ".html"
        CACHE STRING "Use html" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_ZLIB=1  -s USE_VORBIS=1 -s USE_OGG=1 ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_ZLIB=1  -s USE_VORBIS=1 -s USE_OGG=1 ")
    set(LINK_FLAGS
        "${LINK_FLAGS} -s ALLOW_MEMORY_GROWTH=1 -s USE_VORBIS=1 -s USE_OGG=1 -s FULL_ES3=1 -s USE_WEBGL2=1 -s STACK_SIZE=1mb -s EXPORTED_RUNTIME_METHODS=cwrap -sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=2 -s 'DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=[\"$autoResumeAudioContext\",\"$dynCall\"]'"
    )
endif(EMSCRIPTEN)


if(UNIX OR APPLE)
    list(APPEND COMPILE_OPTIONS
        -Wall
        -Wextra
        -Wdeprecated
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wshadow
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
    )

endif(UNIX OR APPLE)

set(SRC_FILES
    ./src/openal.c
    ./src/openalSfx.c
    ./src/openalStream.c
    ./src/openalBgm.c
    ./src/openalMemoryStream.c
)

add_library(${LIBRARY_TARGET_NAME} ${SRC_FILES})
if(BUILD_TEST)
    add_executable(${LIBRARY_TARGET_NAME}_test ./test/main.c)
    target_link_directories(${LIBRARY_TARGET_NAME} PUBLIC /usr/local/lib) # should always include from usr/local/lib
    target_link_libraries(${LIBRARY_TARGET_NAME}_test PUBLIC ${LIBRARY_TARGET_NAME})
    target_link_libraries(${LIBRARY_TARGET_NAME}_test PUBLIC sgforge sgtools)
    target_include_directories(
        ${LIBRARY_TARGET_NAME} SYSTEM
        PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include /usr/local/include
        PRIVATE # linux builds
        ${lib_ogg_SOURCE_DIR}/include ${lib_ogg_BINARY_DIR}/include
    )
    target_include_directories(${LIBRARY_TARGET_NAME}_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include )
endif(BUILD_TEST)

set(SKIP_INSTALL_ALL
    ON
    CACHE BOOL "We don't want any of these external packages to install anything")
set(CMAKE_DEBUG_POSTFIX
    ""
    CACHE STRING " Don't use the d for debug packages" FORCE)
set(CMAKE_DEBUG_TARGET_PROPERTIES
    "DEBUG_OUTPUT" ""
    CACHE STRING "Don't use the d for debug packages" FORCE)

if(SYSTEM_PACKAGES) # If we are using system packages, then we should try to find them.
    find_library(
        LIB_OPENAL
        PATH_SUFFIXES .dylib .a .dll
        NAMES libopenal.dylib libopenal openal libopenal.a openal.dll libopenal.a
        HINTS /usr/local/lib /c/cmake/lib)

    if(LIB_OPENAL)
        list(APPEND EXTERNAL_LIBS ${LIB_OPENAL})
    endif(LIB_OPENAL)

    find_library(
        LIB_VORBIS
        PATH_SUFFIXES .a .dll
        NAMES libvorbis vorbis vorbis.a vorbis.dll libvorbis.a
        HINTS /usr/local/lib /c/cmake/lib)

    if(LIB_VORBIS)
        list(APPEND EXTERNAL_LIBS ${LIB_VORBIS})
    endif(LIB_VORBIS)

    find_library(
        LIB_VORBISFILE
        PATH_SUFFIXES .a .dll
        NAMES libvorbisfile vorbisfile vorbisfile.a vorbisfile.dll
        HINTS /usr/local/lib)

    if(LIB_VORBISFILE)
        list(APPEND EXTERNAL_LIBS ${LIB_VORBISFILE})
    endif(LIB_VORBISFILE)

    find_library(
        LIB_OGG
        PATH_SUFFIXES .a .dll
        NAMES ogg Ogg
        HINTS /usr/local/lib)

    if(LIB_OGG)
        list(APPEND EXTERNAL_LIBS ${LIB_OGG})
    endif(LIB_OGG)

endif(SYSTEM_PACKAGES)

if(NOT EMSCRIPTEN AND NOT LIB_OGG)
    message(STATUS "OGG not found. Fetching OGG...")
    set(BUILD_TESTING
        OFF
        CACHE BOOL "do not build tests")
    FetchContent_Declare(
        LIB_OGG
        GIT_REPOSITORY https://github.com/xiph/ogg
        GIT_TAG v1.3.5
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(LIB_OGG)
    get_target_property(OGG_INCLUDE_DIR ogg INTERFACE_INCLUDE_DIRECTORIES)
    set(OGG_LIBRARY ogg)
    # list(APPEND EXTERNAL_LIBS ogg)
    target_link_libraries(${LIBRARY_TARGET_NAME} PUBLIC ogg)
endif(NOT EMSCRIPTEN AND NOT LIB_OGG)

if(NOT EMSCRIPTEN AND NOT LIB_VORBIS)
    message(STATUS "VORBIS not found. Fetching VORBIS...")
    set(INSTALL_CMAKE_PACKAGE_MODULE
        OFF
        CACHE BOOL "Dont do it" FORCE)
    FetchContent_Declare(
        LIB_VORBIS
        GIT_REPOSITORY https://github.com/xiph/vorbis
        GIT_TAG v1.3.7
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(LIB_VORBIS)
    list(APPEND EXTERNAL_LIBS vorbisenc vorbisfile vorbis)
endif(NOT EMSCRIPTEN AND NOT LIB_VORBIS)

if(NOT EMSCRIPTEN AND NOT LIB_OPENAL)
    message(STATUS "openal not found. Fetching openal...")
    set(BUILD_SHARED_LIBS
        OFF
        CACHE INTERNAL "Do not use")
    set(LIBTYPE
        "STATIC"
        CACHE INTERNAL "Do not use")
    set(ALSOFT_UTILS
        OFF
        CACHE INTERNAL "")
    set(ALSOFT_EXAMPLES
        OFF
        CACHE INTERNAL "")
    set(ALSOFT_BACKEND_WAVE
        OFF
        CACHE INTERNAL "")
    set(ALSOFT_EMBED_HRTF_DATA
        ON
        CACHE INTERNAL "")
    # set(ALSOFT_REQUIRE_COREAUDIO ON CACHE INTERNAL "" )
    FetchContent_Declare(
        LIB_OPENAL
        GIT_REPOSITORY https://github.com/kcat/openal-soft
        GIT_TAG 1.24.3
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(LIB_OPENAL)
    target_link_libraries(${LIBRARY_TARGET_NAME} PUBLIC OpenAL)
endif(NOT EMSCRIPTEN AND NOT LIB_OPENAL)

target_compile_options(${LIBRARY_TARGET_NAME} PUBLIC ${COMPILE_OPTIONS})
target_link_directories(${LIBRARY_TARGET_NAME} PUBLIC /usr/local/lib) # should always include from usr/local/lib
target_link_libraries(${LIBRARY_TARGET_NAME} PUBLIC ${EXTERNAL_LIBS})

# System is for blocking out the warnings, these are mostly external libs
target_include_directories(
    ${LIBRARY_TARGET_NAME} SYSTEM
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include /usr/local/include
    PRIVATE # linux builds
    ${lib_ogg_SOURCE_DIR}/include ${lib_ogg_BINARY_DIR}/include)

target_include_directories(${LIBRARY_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(INSTALL_THE_LIB)
    install(TARGETS ${LIBRARY_TARGET_NAME} ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
    install(DIRECTORY ./include/ DESTINATION include)
endif()

