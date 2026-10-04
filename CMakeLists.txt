cmake_minimum_required(VERSION 3.24)

# Project settings
project(keeperfx-launcher-qt VERSION 0.1 LANGUAGES CXX)

# CMake settings
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows Static Release build options
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release" AND (MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
    message(STATUS "Configuring Windows static build with LTO and size optimizations")
    # Global compilation flags for Release builds
    add_compile_options(
        $<$<CONFIG:Release>:-Os>
        $<$<CONFIG:Release>:-s>
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:Release>:-ffat-lto-objects>
        $<$<CONFIG:Release>:-ffunction-sections>
        $<$<CONFIG:Release>:-fdata-sections>
    )
    # Global link options for Release builds
    add_link_options(
        $<$<CONFIG:Release>:-static>
        $<$<CONFIG:Release>:-Os>
        $<$<CONFIG:Release>:-s>
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:Release>:-fuse-linker-plugin>
        $<$<CONFIG:Release>:-Wl,--gc-sections>
    )
endif()

# Setup flags for CPM third-party dependencies
if(MSVC)
    set(DEP_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
    set(DEP_CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W0")
else()
    # Pass LTO flags to dependencies as well if building in Release
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(DEP_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -Os -flto -ffat-lto-objects -ffunction-sections -fdata-sections")
        set(DEP_CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -Os -flto -ffat-lto-objects -ffunction-sections -fdata-sections")
    else()
        set(DEP_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
        set(DEP_CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
    endif()
endif()

# Enable ccache if available
find_program(CCACHE_PATH ccache)
if(CCACHE_PATH)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PATH}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PATH}")
    message(STATUS "ccache found and enabled")
endif()

# Linker selection (Linux / non-Windows cross builds)
if(NOT WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    find_program(MOLD_PATH mold)
    if(MOLD_PATH)
        message(STATUS "Mold linker found at: ${MOLD_PATH}")
        add_link_options("-fuse-ld=mold")
    endif()
endif()

# Include package manager CPM
# https://github.com/cpm-cmake/CPM.cmake
include("cmake/CPM.cmake")

# Library: zlib
set(ZLIB_USE_STATIC_LIBS ON)
find_package(ZLIB)
if(ZLIB_FOUND)
    message(STATUS "Found host OS ZLIB package.")
else()
    message(STATUS "Host OS ZLIB not found. Downloading and building from source via CPM...")

    CPMAddPackage(
        NAME zlib
        VERSION 1.3.2
        GITHUB_REPOSITORY madler/zlib
        GIT_TAG v1.3.2
        OPTIONS
            "BUILD_SHARED_LIBS OFF"
            "CMAKE_CXX_FLAGS ${DEP_CMAKE_CXX_FLAGS}"
            "CMAKE_C_FLAGS ${DEP_CMAKE_C_FLAGS}"
    )

    # Bridge the target naming gap:
    # System ZLIB provides "ZLIB::ZLIB", but the source build provides "zlibstatic".
    # This alias allows the rest of our CMake script to always link against ZLIB::ZLIB.
    if(NOT TARGET ZLIB::ZLIB)
        if(TARGET zlibstatic)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
        elseif(TARGET zlib)
            add_library(ZLIB::ZLIB ALIAS zlib)
        endif()
    endif()
endif()

# Library: bit7z
CPMFindPackage(
    NAME bit7z
    VERSION 4.1.0
    GITHUB_REPOSITORY rikyoz/bit7z
    GIT_TAG v4.1.0
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "BIT7Z_BUILD_TESTS OFF"
        "BIT7Z_BUILD_DOCS OFF"
        "CMAKE_CXX_FLAGS ${DEP_CMAKE_CXX_FLAGS}"
        "CMAKE_C_FLAGS ${DEP_CMAKE_C_FLAGS}"
)

# Suppress the SFINAE incomplete warning specifically for bit7z
if(TARGET bit7z)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(bit7z PRIVATE -Wno-sfinae-incomplete)
    endif()
endif()

# Set custom UI directory
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui)

# Get source files
file(GLOB PROJECT_SOURCES src/*.cpp src/*.h)
file(GLOB UI_FILES ui/*.ui)

if(WIN32)
    qt_add_executable(keeperfx-launcher-qt
        WIN32
        ${PROJECT_SOURCES}
        resources.qrc
        app.rc
    )
else()
    qt_add_executable(keeperfx-launcher-qt
        ${PROJECT_SOURCES}
        resources.qrc
    )
endif()

# Link libraries
target_link_libraries(keeperfx-launcher-qt PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Gui
    ZLIB::ZLIB
    bit7z
)

# Link crypt32 on Windows
if(WIN32)
    target_link_libraries(keeperfx-launcher-qt PRIVATE crypt32)
endif()

# Set target properties
set_target_properties(keeperfx-launcher-qt PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Copy pre-compiled libraries
if (UNIX)
    file(GLOB COMPILED_LIBS "${CMAKE_SOURCE_DIR}/ext/*.so")
elseif (WIN32)
    file(GLOB COMPILED_LIBS "${CMAKE_SOURCE_DIR}/ext/*.dll")
endif()
foreach(LIB ${COMPILED_LIBS})
    add_custom_command(TARGET keeperfx-launcher-qt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${LIB}" $<TARGET_FILE_DIR:keeperfx-launcher-qt>
        VERBATIM
    )
endforeach()

# Linux Installation settings
if (UNIX)
    include(GNUInstallDirs)
    install(TARGETS keeperfx-launcher-qt
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
