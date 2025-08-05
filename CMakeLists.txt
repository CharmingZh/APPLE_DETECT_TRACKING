cmake_minimum_required(VERSION 3.16)
project(Apple_RGBD)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(OPENCV_INCLUDE_DIR ${THIRD_PARTY_DIR}/opencv/include)
set(OPENCV_LIB_DIR     ${THIRD_PARTY_DIR}/opencv/lib)
set(OPENCV_BIN_DIR     ${THIRD_PARTY_DIR}/opencv/bin)

find_package(Threads REQUIRED)

file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp")
list(APPEND PROJECT_SOURCES "${THIRD_PARTY_DIR}/hungarian/Hungarian.cpp")
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES}
        src/ImageTracker.cpp
        src/ImageTracker.h
        src/SimpleSerial.h)

target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${OPENCV_INCLUDE_DIR}"
        "${THIRD_PARTY_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        "${OPENCV_LIB_DIR}/opencv_core4130.lib"
        "${OPENCV_LIB_DIR}/opencv_imgproc4130.lib"
        "${OPENCV_LIB_DIR}/opencv_highgui4130.lib"
        "${OPENCV_LIB_DIR}/opencv_imgcodecs4130.lib"
        "${OPENCV_LIB_DIR}/opencv_videoio4130.lib"
        Threads::Threads
)

file(GLOB OPENCV_DLLS "${OPENCV_BIN_DIR}/*.dll")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OPENCV_DLLS}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying OpenCV DLLs for ${PROJECT_NAME}..."
)