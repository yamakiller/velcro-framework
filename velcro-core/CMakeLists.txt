
CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(PROJ_NAME_CORE VelcroCore)

SET(SRC_FILES)
SET(LIB_LINKS)

#设置不同平台的编辑参数
IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    #在Windows平台下值考虑MSVC编译器
    MESSAGE(STATUS "Platform Window")

    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        MESSAGE(STATUS "Compile Tool mingw64")

        SET(CMAKE_CXX_COMPILER g++)
        SET(CMAKE_C_COMPILER gcc)
    
        SET(CMAKE_CXX_FLAGS_DEBUG	"-std=c++11 -O0 -Wall -march=pentium4 -mmmx -g2 -ggdb")
        SET(CMAKE_CXX_FLAGS_RELEASE	"-std=c++11 -Wall -o3")
    ELSE()
        MESSAGE(STATUS "Compile Tool msvc")
        #ADD_DEFINITIONS(-D_CRT_SECURE_NOWARNINGS)
        #ADD_DEFINITIONS(-D__STDC_WANT_SECURELIB__=1)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
        add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
        add_definitions(-D_SILENCE_CXX20_IS_POD_DEPRECATION_WARNING)
        #这是为UNICODE编码模式
        ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
        
        SET(CMAKE_CXX_STANDARD 20)
        SET(CMAKE_C_STANDARD 20)
        SET(CMAKE_CXX_FLAGS_DEBUG	"/D_DEBUG /MDd /Zi /Ob0 /Od /RTC1")
        SET(CMAKE_CXX_FLAGS_RELEASE	"/MD /O2 /Ob2 /D NDEBUG")

        ADD_LINK_OPTIONS(/SAFESEH:NO)

        ADD_COMPILE_OPTIONS("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
        ADD_COMPILE_OPTIONS("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

        ADD_COMPILE_OPTIONS(/Zc:preprocessor /wd5105)
        
    ENDIF()
    INCLUDE_DIRECTORIES(${LOCAL_CORE_SOURCE_DIR}/platforms/windows)
    INCLUDE(${LOCAL_CORE_SOURCE_DIR}/platforms/windows/platform_windows_files.cmake)
    LIST(APPEND SRC_FILES ${FILES})
    ADD_DEFINITIONS("-DV_NUMERICCAST_ENABLED=1")
    ADD_DEFINITIONS("-DVELCROCORE_EXPORTS=1")


ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "Platform Linux")
    MESSAGE(STATUS "Compile Tool gcc/g++")

    SET(CMAKE_CXX_COMPILER g++)
    SET(CMAKE_C_COMPILER gcc)

    SET(CMAKE_CXX_FLAGS_DEBUG	"-std=c++20 -O0 -Wall -march=pentium4 -mmmx -g2 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE	"-std=c++20 -Wall -o3")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    MESSAGE(STATUS "Platform FreeBSD")
    MESSAGE(STATUS "Compile Tool gcc/g++")

    SET(CMAKE_CXX_COMPILER g++)
    SET(CMAKE_C_COMPILER gcc)

    SET(CMAKE_CXX_FLAGS_DEBUG	"-std=c++20 -O0 -Wall -march=pentium4 -mmmx -g2 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE	"-std=c++20 -Wall -o3")
ELSE()
    # 考虑支持 Mac 系统
    MESSAGE(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Windows")

Message("-- Used Compressor is zlib")
INCLUDE_DIRECTORIES(${THIRDPARTY_DIR}/zlib/include)
IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    #LIST(APPEND LIB_LINKS "zlib")
    IF (CMAKE_CL_64)
        IF(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/zlib/lib/Win64/Debug)
        ELSE()
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/zlib/lib/Win64/Release)
        ENDIF()
    ELSE()
    IF(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/zlib/lib/Win32/Debug)
        ELSE()
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/zlib/lib/Win32/Release)
        ENDIF()
    ENDIF()
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    #LIST(APPEND LIB_LINKS "-lzlib")
ENDIF()

Message("-- Used Compressor is zstd")
INCLUDE_DIRECTORIES(${THIRDPARTY_DIR}/zstd/include)
IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    #LIST(APPEND LIB_LINKS "zstd")
    IF (CMAKE_CL_64)
        IF(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/zstd/lib/Win64/Debug)
        ELSE()
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/zstd/lib/Win64/Release)
        ENDIF()
    ELSE()
    IF(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/zstd/lib/Win32/Debug)
        ELSE()
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/zstd/lib/Win32/Release)
        ENDIF()
    ENDIF()
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    #LIST(APPEND LIB_LINKS "-lzstd")
ENDIF()




INCLUDE(${LOCAL_CORE_SOURCE_DIR}/vcore/vcore_files.cmake)
LIST(APPEND SRC_FILES ${FILES})


ADD_LIBRARY(${PROJ_NAME_CORE} SHARED ${SRC_FILES})
TARGET_LINK_LIBRARIES(${PROJ_NAME_CORE} ${LIB_LINKS})