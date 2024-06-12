CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(PROJ_NAME VelcroFramework)

#调试版本增加后缀
SET(CMAKE_DEBUG_POSTFIX "_debug")

#设置编译输出目录
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Binaries)
SET(LIBRARY_OUTPUT_PATH    ${PROJECT_SOURCE_DIR}/Binaries)

#设置编译输出目录为默认库搜索路径
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/Binaries)
#设置头文件搜索目录
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/core)


add_subdirectory(core)
add_subdirectory(version)

