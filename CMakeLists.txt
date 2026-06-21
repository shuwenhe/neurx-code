cmake_minimum_required(VERSION 3.21.1)

option(LINK_INSIGHT "Link Qt Insight Tracker library" ON)
# Qt Design Studio components pull a remote repository during configure.
# Keep them opt-in so ordinary local builds do not fail when offline or
# behind a restricted network.
option(BUILD_QDS_COMPONENTS "Build design studio components" OFF)
option(NEURX_ALLOW_QDS_FETCH "Allow fetching Qt Design Studio components from the network" OFF)

project(neurx-codeApp LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt Creator's single-config builds need an explicit build type.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Qml Quick Network Concurrent Widgets)
find_package(Qt6 OPTIONAL_COMPONENTS WebSockets)

find_path(SQLITE3_INCLUDE_DIR
    NAMES sqlite3.h
    HINTS
        "C:/Users/Public/qt/Tools/mingw1310_64/opt/include"
        "C:/Program Files/msys64/mingw64/include"
)

find_library(SQLITE3_LIBRARY
    NAMES sqlite3 libsqlite3
    HINTS
        "C:/Users/Public/qt/Tools/mingw1310_64/opt/lib"
        "C:/Program Files/msys64/mingw64/lib"
)

if(NOT SQLITE3_INCLUDE_DIR OR NOT SQLITE3_LIBRARY)
    message(FATAL_ERROR
        "[neurx] SQLite3 development files were not found. "
        "Expected sqlite3.h and a sqlite3 library in a configured toolchain path.")
endif()

add_library(sqlite3 UNKNOWN IMPORTED)
set_target_properties(sqlite3 PROPERTIES
    IMPORTED_LOCATION "${SQLITE3_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE_DIR}"
)

if (Qt6_VERSION VERSION_GREATER_EQUAL 6.3)
    qt_standard_project_setup()
endif()

# ── Phase 1: Framework Sources (neurx_core) ───────────────────────────────────
# Prevent MOC from processing conflicting plugin headers to avoid type redefinition
# Must be done BEFORE add_library for CMake to honor SKIP_MOC during MOC phase
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/PluginMetadata.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/PluginInterface.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/PluginTypes.h"
    PROPERTIES SKIP_MOC ON
)

# Use GLOB_RECURSE to find all .cpp sources from src/ (headers are for MOC, not direct compilation)
file(GLOB_RECURSE NEURX_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp"
)

# Remove main.cpp (compiled separately in executable)
list(FILTER NEURX_CORE_SOURCES EXCLUDE REGEX "/src/main\\.cpp$")

# Remove Moc-generated files (automatically generated)
list(FILTER NEURX_CORE_SOURCES EXCLUDE REGEX "Moc\\.cpp$")

# Remove test files (have their own main() or are for testing only)
list(FILTER NEURX_CORE_SOURCES EXCLUDE REGEX "/src/test/.*\\.cpp$")
list(FILTER NEURX_CORE_SOURCES EXCLUDE REGEX "Test\\.cpp$")

# Remove duplicate/support files that define symbols elsewhere
list(FILTER NEURX_CORE_SOURCES EXCLUDE REGEX "SkillSystemSupport\\.cpp$")

add_library(neurx_core STATIC
    ${NEURX_CORE_SOURCES}
)


target_sources(neurx_core PRIVATE
    src/filesystem/ExecutorFileSystem.h
    src/goals/GoalManager.h
    src/skills/SkillManager.h
    src/state/StateManager.h
    src/config/ConfigManager.h
    src/plugins/PluginManager.h
    src/llm/LLMExtensions.h
    src/memory/MemoryManager.h
    src/execution/ExecutionEngine.h
    src/integration/IntegrationOrchestrator.h
    src/tools/ToolRegistry.h
    src/skills/AnthropicManagers.h
)

target_include_directories(neurx_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(neurx_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::Concurrent
    Qt6::Widgets
    sqlite3
)

set_target_properties(neurx_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Reduce Debug info volume to keep very large translation units from being
# killed by the macOS linker/compiler when memory pressure spikes.
target_compile_options(neurx_core PRIVATE
    $<$<CONFIG:Debug>:-g1>
)

# ── Phase 2: Collect All Remaining Sources ──────────────────────────────────
# All sources were already collected and added to neurx_core in Phase 1.
# No need for redundant globbing and filtering that might exclude necessary files.

qt_add_executable(neurx-codeApp
    src/main.cpp
)

target_link_libraries(neurx-codeApp PRIVATE
    neurx_core
    content
    NeurXCode
)

if(APPLE)
    # neurx_core is a large moc-heavy static library; force loading avoids
    # archive member selection issues on macOS when Qt meta-object references
    # only become visible after the first scan.
    target_link_options(neurx-codeApp PRIVATE
        "LINKER:-force_load,$<TARGET_FILE:neurx_core>"
    )
endif()

qt_add_resources(neurx-codeApp "configuration"
    PREFIX "/"
    FILES
        qtquickcontrols2.conf
)

target_link_libraries(neurx-codeApp PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::Concurrent
    Qt6::Widgets
)

# Optional WebSockets support
if(Qt6WebSockets_FOUND)
    target_link_libraries(neurx-codeApp PRIVATE Qt6::WebSockets)
    target_compile_definitions(neurx-codeApp PRIVATE NEURX_HAS_WEBSOCKETS)
endif()

# ── Platform-specific packaging ───────────────────────────────────────────────
if(APPLE)
    # Set macOS bundle properties
    set_target_properties(neurx-codeApp PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "NeurX Code"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER "ai.neurx.code"
        MACOSX_BUNDLE_ICON_FILE "AppIcon"
    )

    # Copy app icon into the bundle if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/AppIcon.icns")
        set_source_files_properties(
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/AppIcon.icns"
            PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources
        )
        target_sources(neurx-codeApp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/assets/AppIcon.icns")
    endif()

    # Embed Qt plugins into the .app bundle
    qt_generate_deploy_app_script(
        TARGET neurx-codeApp
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
elseif(WIN32)
    set_target_properties(neurx-codeApp PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.rc")
        target_sources(neurx-codeApp PRIVATE assets/app.rc)
    endif()
    # Windows deploy via windeployqt (called from build-windows.bat)
endif()

# ── Codex CLI (Rust) ──────────────────────────────────────────────────────────
# Builds the bundled Codex CLI Rust workspace and copies the resulting binary
# into the app output directory so CodexTool can invoke it at runtime.
#
# Disable with:  cmake ... -DBUILD_CODEX_CLI=OFF
# ─────────────────────────────────────────────────────────────────────────────
option(BUILD_CODEX_CLI "Build the Codex CLI from the bundled Rust workspace" ON)

set(CODEX_RS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/codex/codex-rs")

if(BUILD_CODEX_CLI)
    find_program(CARGO_EXECUTABLE cargo
        HINTS
            "$ENV{HOME}/.cargo/bin"
            "/opt/homebrew/bin"
            "/usr/local/bin"
            "$ENV{CARGO_HOME}/bin"
        DOC "Rust Cargo build tool (https://rustup.rs)")

    if(NOT CARGO_EXECUTABLE)
        message(WARNING
            "[neurx] BUILD_CODEX_CLI=ON but 'cargo' was not found. "
            "Install Rust via https://rustup.rs, or pass -DBUILD_CODEX_CLI=OFF to skip. "
            "Disabling Codex CLI build for this configuration.")
        set(BUILD_CODEX_CLI OFF CACHE BOOL "" FORCE)
    endif()
endif()

if(BUILD_CODEX_CLI)
    # Map CMake build type → Cargo profile.
    set(_codex_cargo_args "build" "-p" "codex-cli")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CODEX_CARGO_PROFILE "debug")
    else()
        set(CODEX_CARGO_PROFILE "release")
        list(APPEND _codex_cargo_args "--release")
    endif()

    set(CODEX_CLI_BINARY
        "${CODEX_RS_DIR}/target/${CODEX_CARGO_PROFILE}/codex${CMAKE_EXECUTABLE_SUFFIX}")

    # Cargo is incremental; always invoke it — it will only recompile changed crates.
    # Force RUSTUP_TOOLCHAIN=stable so rustup does not attempt to sync/install
    # the pinned 1.95.0 toolchain from rust-toolchain.toml on every build.
    # stable (1.96.0) satisfies the 1.95.0 minimum requirement.
    add_custom_target(codex_cli ALL
        COMMAND ${CMAKE_COMMAND} -E env RUSTUP_TOOLCHAIN=stable
            "${CARGO_EXECUTABLE}" ${_codex_cargo_args}
        BYPRODUCTS "${CODEX_CLI_BINARY}"
        WORKING_DIRECTORY "${CODEX_RS_DIR}"
        COMMENT "[neurx] Building Codex CLI (Rust, profile=${CODEX_CARGO_PROFILE})…"
        USES_TERMINAL
        VERBATIM
    )

    # neurx_core must wait for codex_cli and receives the binary path as a macro.
    add_dependencies(neurx_core codex_cli)
    target_compile_definitions(neurx_core PRIVATE
        CODEX_CLI_BINARY_PATH="${CODEX_CLI_BINARY}"
    )

    # ── Copy the binary into the runtime output tree ──────────────────────────
    if(APPLE)
        # macOS: place inside the .app bundle's Contents/MacOS/
        add_custom_command(TARGET neurx-codeApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CODEX_CLI_BINARY}"
                "$<TARGET_BUNDLE_DIR:neurx-codeApp>/Contents/MacOS/codex"
            COMMENT "[neurx] Copying Codex CLI into .app bundle…"
            VERBATIM
        )
        install(PROGRAMS "${CODEX_CLI_BINARY}"
                DESTINATION "neurx-codeApp.app/Contents/MacOS"
                COMPONENT Runtime)
    elseif(WIN32)
        add_custom_command(TARGET neurx-codeApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CODEX_CLI_BINARY}"
                "$<TARGET_FILE_DIR:neurx-codeApp>/codex.exe"
            COMMENT "[neurx] Copying Codex CLI next to executable…"
            VERBATIM
        )
        install(PROGRAMS "${CODEX_CLI_BINARY}"
                DESTINATION "."
                COMPONENT Runtime)
    else()
        # Linux: place in the same directory as the app binary.
        add_custom_command(TARGET neurx-codeApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CODEX_CLI_BINARY}"
                "$<TARGET_FILE_DIR:neurx-codeApp>/codex"
            COMMENT "[neurx] Copying Codex CLI next to executable…"
            VERBATIM
        )
        install(PROGRAMS "${CODEX_CLI_BINARY}"
                DESTINATION "bin"
                COMPONENT Runtime)
    endif()

    message(STATUS "[neurx] Codex CLI: cargo=${CARGO_EXECUTABLE}, profile=${CODEX_CARGO_PROFILE}")
    message(STATUS "[neurx] Codex CLI binary: ${CODEX_CLI_BINARY}")
endif()

# ── Gemini CLI (C++) ──────────────────────────────────────────────────────────
# Builds the bundled Gemini CLI C++ workspace and copies the resulting binary
# into the app output directory so GeminiTool can invoke it at runtime.
#
# Disable with:  cmake ... -DBUILD_GEMINI_CLI=OFF
# ─────────────────────────────────────────────────────────────────────────────
# Note: gemini-cli directory does not exist in this workspace, so disable by default
option(BUILD_GEMINI_CLI "Build the Gemini CLI from the bundled C++ workspace" OFF)
set(GEMINI_CLI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gemini-cli")
if(BUILD_GEMINI_CLI)
    # Map CMake build type → Gemini profile.
    set(_gemini_cli_args "build" "-p" "gemini-cli")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(GEMINI_CLI_PROFILE "debug")
    else()
        set(GEMINI_CLI_PROFILE "release")
        list(APPEND _gemini_cli_args "--release")
    endif()

    set(GEMINI_CLI_BINARY
        "${GEMINI_CLI_DIR}/target/${GEMINI_CLI_PROFILE}/gemini${CMAKE_EXECUTABLE_SUFFIX}")

    set(GEMINI_COPY_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/CopyGeminiCliIfExists.cmake")
    file(WRITE "${GEMINI_COPY_SCRIPT}" [=[
if(EXISTS "${src}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${src}" "${dst}"
        RESULT_VARIABLE copy_result
    )
    if(NOT copy_result EQUAL 0)
        message(FATAL_ERROR "[neurx] Failed to copy Gemini CLI from ${src} to ${dst}")
    endif()
else()
    message(STATUS "[neurx] Gemini CLI binary not found at ${src}; skipping bundle copy.")
endif()
]=])

    # ── Copy the binary into the runtime output tree ──────────────────────────
    if(APPLE)
        # macOS: place inside the .app bundle's Contents/MacOS/
        add_custom_command(TARGET neurx-codeApp POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -Dsrc="${GEMINI_CLI_BINARY}"
                -Ddst="$<TARGET_BUNDLE_DIR:neurx-codeApp>/Contents/MacOS/gemini"
                -P "${GEMINI_COPY_SCRIPT}"
            COMMENT "[neurx] Copying Gemini CLI into .app bundle…"
            VERBATIM
        )
        install(PROGRAMS "${GEMINI_CLI_BINARY}"
                DESTINATION "neurx-codeApp.app/Contents/MacOS"
                COMPONENT Runtime)
    elseif(WIN32)
        add_custom_command(TARGET neurx-codeApp POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -Dsrc="${GEMINI_CLI_BINARY}"
                -Ddst="$<TARGET_FILE_DIR:neurx-codeApp>/gemini.exe"
                -P "${GEMINI_COPY_SCRIPT}"
            COMMENT "[neurx] Copying Gemini CLI next to executable…"
            VERBATIM
        )
        install(PROGRAMS "${GEMINI_CLI_BINARY}"
                DESTINATION "."
                COMPONENT Runtime)
    else()
        # Linux: place in the same directory as the app binary.
        add_custom_command(TARGET neurx-codeApp POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -Dsrc="${GEMINI_CLI_BINARY}"
                -Ddst="$<TARGET_FILE_DIR:neurx-codeApp>/gemini"
                -P "${GEMINI_COPY_SCRIPT}"
            COMMENT "[neurx] Copying Gemini CLI next to executable…"
            VERBATIM
        )
        install(PROGRAMS "${GEMINI_CLI_BINARY}"
                DESTINATION "bin"
                COMPONENT Runtime)
    endif()
    message(STATUS "[neurx] Gemini CLI binary: ${GEMINI_CLI_BINARY}")
endif()

set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)
set(QML_IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    CACHE STRING "Import paths for Qt Creator's code model"
    FORCE
)

if (BUILD_QDS_COMPONENTS AND NOT NEURX_ALLOW_QDS_FETCH)
    message(WARNING
        "[neurx] BUILD_QDS_COMPONENTS is ON but NEURX_ALLOW_QDS_FETCH is OFF. "
        "Skipping Qt Design Studio components to avoid configure failures when network access is unavailable. "
        "Enable both options explicitly if you want CMake to fetch them.")
    set(BUILD_QDS_COMPONENTS OFF CACHE BOOL "" FORCE)
endif()

if (BUILD_QDS_COMPONENTS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/qmlcomponents)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/qmlmodules)

if (LINK_INSIGHT)
    include(${CMAKE_CURRENT_SOURCE_DIR}/insight)
endif ()

# Enable testing
enable_testing()
add_subdirectory(tests)

include(GNUInstallDirs)
install(TARGETS neurx-codeApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# make IDEs aware of the QML import path
set(QML_IMPORT_PATH ${PROJECT_BINARY_DIR}/qml CACHE PATH
    "Path to the custom QML components defined by the project")
