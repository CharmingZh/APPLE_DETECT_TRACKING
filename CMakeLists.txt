cmake_minimum_required(VERSION 3.16)
project(Apple_RGBD)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 路径定义 ---
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# OpenCV 路径
set(OPENCV_INCLUDE_DIR ${THIRD_PARTY_DIR}/opencv/include)
set(OPENCV_LIB_DIR     ${THIRD_PARTY_DIR}/opencv/lib)
set(OPENCV_BIN_DIR     ${THIRD_PARTY_DIR}/opencv/bin)

# [新增] Azure Kinect SDK 本地路径
set(KINECT_SDK_DIR           ${THIRD_PARTY_DIR}/kinect_sdk)
set(KINECT_SDK_INCLUDE_DIR   ${KINECT_SDK_DIR}/include)
set(KINECT_SDK_LIB_DIR       ${KINECT_SDK_DIR}/lib)
set(KINECT_SDK_BIN_DIR       ${KINECT_SDK_DIR}/bin)

find_package(Threads REQUIRED)

# --- 查找所有源文件 ---
file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp")
list(APPEND PROJECT_SOURCES "${THIRD_PARTY_DIR}/hungarian/Hungarian.cpp")

# --- 创建可执行文件 ---
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# --- 添加头文件目录 ---
target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${OPENCV_INCLUDE_DIR}"
        "${KINECT_SDK_INCLUDE_DIR}" # [新增] 添加 Kinect 头文件目录
        "${THIRD_PARTY_DIR}"
)

# --- 链接库文件 ---
target_link_libraries(${PROJECT_NAME} PRIVATE
        # OpenCV 库
        "${OPENCV_LIB_DIR}/opencv_core4130.lib"
        "${OPENCV_LIB_DIR}/opencv_imgproc4130.lib"
        "${OPENCV_LIB_DIR}/opencv_highgui4130.lib"
        "${OPENCV_LIB_DIR}/opencv_imgcodecs4130.lib"
        "${OPENCV_LIB_DIR}/opencv_videoio4130.lib"

        # [新增] Kinect 库
        "${KINECT_SDK_LIB_DIR}/k4a.lib"

        # 线程库
        Threads::Threads
)

# --- 复制 DLL 文件到输出目录 ---
# 复制 OpenCV DLLs
file(GLOB OPENCV_DLLS "${OPENCV_BIN_DIR}/*.dll")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OPENCV_DLLS}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying OpenCV DLLs for ${PROJECT_NAME}..."
)

# [新增] 复制 Kinect DLLs
file(GLOB KINECT_DLLS "${KINECT_SDK_BIN_DIR}/*.dll")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${KINECT_DLLS}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying Kinect DLLs for ${PROJECT_NAME}..."
)