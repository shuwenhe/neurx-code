cmake_minimum_required(VERSION 3.21)

# Find Qt6 Test module
find_package(Qt6 COMPONENTS Test Core Gui Qml Quick REQUIRED)

# Enable automatic MOC for test files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Get all test files (unit tests and integration tests)
file(GLOB TEST_SOURCES "tst_*.cpp")

# Create executable for each test
foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    
    add_executable(${TEST_NAME} ${TEST_FILE})
    
    target_link_libraries(${TEST_NAME}
        Qt6::Test
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        neurx_core
    )

    if(APPLE)
        target_link_options(${TEST_NAME} PRIVATE
            "LINKER:-force_load,$<TARGET_FILE:neurx_core>"
        )
    endif()
    
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/agent
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/approvals
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/integration
    )
    
    # Register the test with CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    # Set test properties
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "high-value-plugins"
        TIMEOUT 300  # 5 minutes timeout
    )
endforeach()

# Create test groups for organization
if(TEST_SOURCES)
    list(LENGTH TEST_SOURCES TEST_COUNT)
    message(STATUS "Configured ${TEST_COUNT} tests for High-Value Plugins")
endif()

# Summary
message(STATUS "Tests configured: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Unit Tests: CodeReviewEngine, CommitCommandManager, FeatureDevelopmentWorkflow, SecurityAnalyzer, HookifyManager, PRReviewAgents")
