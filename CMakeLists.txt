CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

#SET(PROJ_NAME VelcroFramework)

#调试版本增加后缀
SET(CMAKE_DEBUG_POSTFIX "_debug")

SET(LOCAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/core)
SET(LOCAL_TEST_SOURCE_DIR ${PROJECT_SOURCE_DIR}/velcro-test)
SET(THIRDPARTY_DIR ${PROJECT_SOURCE_DIR}/3dparty)


#设置编译输出目录
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Binaries)
SET(LIBRARY_OUTPUT_PATH    ${PROJECT_SOURCE_DIR}/Binaries)

#设置编译输出目录为默认库搜索路径
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/Binaries)
#设置头文件搜索目录
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/core)

# ThreadSanitizer
set(CMAKE_C_FLAGS_TSAN
	"-fsanitize=thread -g -O1"
	CACHE STRING "Flags used by the C compiler during ThreadSanitizer builds."
	FORCE)
set(CMAKE_CXX_FLAGS_TSAN
	"-fsanitize=thread -g -O1"
	CACHE STRING "Flags used by the C++ compiler during ThreadSanitizer builds."
	FORCE)

# AddressSanitize
set(CMAKE_C_FLAGS_ASAN
	"-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g -O1"
	CACHE STRING "Flags used by the C compiler during AddressSanitizer builds."
	FORCE)
set(CMAKE_CXX_FLAGS_ASAN
	"-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g -O1"
	CACHE STRING "Flags used by the C++ compiler during AddressSanitizer builds."
	FORCE)

# LeakSanitizer
set(CMAKE_C_FLAGS_LSAN
	"-fsanitize=leak -fno-omit-frame-pointer -g -O1"
	CACHE STRING "Flags used by the C compiler during LeakSanitizer builds."
	FORCE)
set(CMAKE_CXX_FLAGS_LSAN
	"-fsanitize=leak -fno-omit-frame-pointer -g -O1"
	CACHE STRING "Flags used by the C++ compiler during LeakSanitizer builds."
	FORCE)


# Remember to switch lib: https://stackoverflow.com/questions/20617788/using-memory-sanitizer-with-libstdc/20784130#20784130
# use ENABLE_LIBCXX
# MemorySanitizer
set(CMAKE_C_FLAGS_MSAN
	"-fsanitize=memory -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O2"
	CACHE STRING "Flags used by the C compiler during MemorySanitizer builds."
	FORCE)
set(CMAKE_CXX_FLAGS_MSAN
	"-fsanitize=memory -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O2"
	CACHE STRING "Flags used by the C++ compiler during MemorySanitizer builds."
	FORCE)

# UndefinedBehaviour
set(CMAKE_C_FLAGS_UBSAN
	"-fsanitize=undefined"
	CACHE STRING "Flags used by the C compiler during UndefinedBehaviourSanitizer builds."
	FORCE)
set(CMAKE_CXX_FLAGS_UBSAN
	"-fsanitize=undefined"
	CACHE STRING "Flags used by the C++ compiler during UndefinedBehaviourSanitizer builds."
	FORCE)

    
add_subdirectory(core)
add_subdirectory(version)

