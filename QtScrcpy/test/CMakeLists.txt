cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

set(TEST_NAME tst_qyuvopenglwidget)

find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS Test Widgets OpenGL)
if(QT_DESIRED_VERSION MATCHES 6)
    find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS OpenGLWidgets)
endif()

add_executable(${TEST_NAME}
    tst_qyuvopenglwidget.cpp
    ../render/qyuvopenglwidget.h
    ../render/qyuvopenglwidget.cpp
)

target_include_directories(${TEST_NAME} PRIVATE
    ../render
)

target_link_libraries(${TEST_NAME} PRIVATE
    Qt${QT_DESIRED_VERSION}::Test
    Qt${QT_DESIRED_VERSION}::Widgets
    Qt${QT_DESIRED_VERSION}::OpenGL
)

if(QT_DESIRED_VERSION MATCHES 6)
    target_link_libraries(${TEST_NAME} PRIVATE Qt${QT_DESIRED_VERSION}::OpenGLWidgets)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${TEST_NAME} PRIVATE xcb Threads::Threads)
endif()

add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
set_tests_properties(${TEST_NAME} PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# ---- device naming (pure logic, no GUI) ----
set(NAMING_TEST_NAME tst_devicenaming)

add_executable(${NAMING_TEST_NAME}
    tst_devicenaming.cpp
    ../ui/devicenaming.h
    ../ui/devicenaming.cpp
)

target_include_directories(${NAMING_TEST_NAME} PRIVATE
    ../ui
)

target_link_libraries(${NAMING_TEST_NAME} PRIVATE
    Qt${QT_DESIRED_VERSION}::Test
)

add_test(NAME ${NAMING_TEST_NAME} COMMAND ${NAMING_TEST_NAME})
