# ===================================================================
# CMake 基础配置
# ===================================================================
cmake_minimum_required(VERSION 3.16)
project(Apple_RGBD)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===================================================================
# 1. 定义第三方库的路径
# ===================================================================
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(OPENCV_INCLUDE_DIR ${THIRD_PARTY_DIR}/opencv/include)
set(OPENCV_LIB_DIR     ${THIRD_PARTY_DIR}/opencv/lib)
set(OPENCV_BIN_DIR     ${THIRD_PARTY_DIR}/opencv/bin)

# ===================================================================
# 2. 配置并查找依赖
# ===================================================================
find_package(Threads REQUIRED)

# ===================================================================
# 3. 定义主项目的源文件和可执行文件
# ===================================================================
# 自动查找 src 目录下的所有 .cpp 源文件
file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp")
# 将 Hungarian.cpp 手动添加到要编译的源文件列表中
list(APPEND PROJECT_SOURCES "${THIRD_PARTY_DIR}/hungarian/Hungarian.cpp")
# 创建可执行文件
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# ===================================================================
# 4. 设置主项目的包含目录和链接库
# ===================================================================
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
        "${OPENCV_LIB_DIR}/opencv_videoio4130.lib" # <--- (新增) 添加 videoio 模块
        Threads::Threads
)

# ===================================================================
# 5. 主项目的构建后 DLL 拷贝命令
# ===================================================================
file(GLOB OPENCV_DLLS "${OPENCV_BIN_DIR}/*.dll")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OPENCV_DLLS}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying OpenCV DLLs for ${PROJECT_NAME}..."
)