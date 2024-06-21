CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
SET(PROJ_NAME_CORE VelcroTest)

SET(LIB_LINKS)

IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    #在Windows平台下值考虑MSVC编译器
    MESSAGE(STATUS "Platform Window")

    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        MESSAGE(STATUS "Compile Tool mingw64")

        SET(CMAKE_CXX_COMPILER g++)
        SET(CMAKE_C_COMPILER gcc)
    
        SET(CMAKE_CXX_FLAGS_DEBUG	"-std=c++17 -O0 -Wall -march=pentium4 -mmmx -g2 -ggdb")
        SET(CMAKE_CXX_FLAGS_RELEASE	"-std=c++17 -Wall -o3")

        #LIST(APPEND LIB_LINKS "-lzlib")
    ELSE()
        MESSAGE(STATUS "Compile Tool msvc")
        ADD_DEFINITIONS(-D_CRT_SECURE_NOWARNINGS)
        #这是为UNICODE编码模式
        ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
        #ADD_DEFINITIONS(-DV_COMPILER_MSVC)
        
        SET(CMAKE_CXX_STANDARD 17)
        SET(CMAKE_C_STANDARD 17)
        SET(CMAKE_CXX_FLAGS_DEBUG	"/D_DEBUG /MDd /Zi /Ob0 /Od /RTC1")
        SET(CMAKE_CXX_FLAGS_RELEASE	"/MD /O2 /Ob2 /D NDEBUG")

        ADD_LINK_OPTIONS(/SAFESEH:NO)

        ADD_COMPILE_OPTIONS("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
        ADD_COMPILE_OPTIONS("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

        ADD_COMPILE_OPTIONS(/Zc:preprocessor /wd5105)

    ENDIF()

    INCLUDE_DIRECTORIES(${LOCAL_CORE_SOURCE_DIR}/platforms/windows)
    INCLUDE_DIRECTORIES(${LOCAL_TEST_SOURCE_DIR}/platforms/windows)
    INCLUDE(${LOCAL_TEST_SOURCE_DIR}/platforms/windows/platform_windows_files.cmake)
    ADD_DEFINITIONS("-DV_NUMERICCAST_ENABLED=1")


ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "Platform Linux")
    MESSAGE(STATUS "Compile Tool gcc/g++")

    SET(CMAKE_CXX_COMPILER g++)
    SET(CMAKE_C_COMPILER gcc)

    SET(CMAKE_CXX_FLAGS_DEBUG	"-std=c++17 -O0 -Wall -march=pentium4 -mmmx -g2 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE	"-std=c++17 -Wall -o3")

    #LIST(APPEND LIB_LINKS "-lzlib")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    MESSAGE(STATUS "Platform FreeBSD")
    MESSAGE(STATUS "Compile Tool gcc/g++")

    SET(CMAKE_CXX_COMPILER g++)
    SET(CMAKE_C_COMPILER gcc)

    SET(CMAKE_CXX_FLAGS_DEBUG	"-std=c++17 -O0 -Wall -march=pentium4 -mmmx -g2 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE	"-std=c++17 -Wall -o3")
ELSE()
    # 考虑支持 Mac 系统
    MESSAGE(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Windows")

Message("-- Used googlebenchmark")
Message("-- Used googlemock")
Message("-- Used googletest")
INCLUDE_DIRECTORIES(${THIRDPARTY_DIR}/googlebenchmark/include)
INCLUDE_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googlemock/include)
INCLUDE_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googletest/include)
IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    LIST(APPEND LIB_LINKS "gmock gtest benchmark")
    IF (CMAKE_CL_64)
        IF(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googletest/lib/Win64/Debug)
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googlemock/lib/Win64/Debug)
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googlebenchmark/lib/Win64/Debug)
        ELSE()
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googletest/lib/Win64/Release)
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googlemock/lib/Win64/Release)
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googlebenchmark/lib/Win64/Release)
        ENDIF()
    ELSE()
    IF(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googletest/lib/Win32/Debug)
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googlemock/lib/Win32/Debug)
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googlebenchmark/lib/Win32/Debug)
        ELSE()
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googletest/lib/Win32/Release)
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googlemock/lib/Win32/Release)
            LINK_DIRECTORIES(${THIRDPARTY_DIR}/googlebenchmark/lib/Win32/Release)
        ENDIF()
    ENDIF()
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
LIST(APPEND LIB_LINKS "-lgmock -lgtest -lgmock_main")
    IF(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
        LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googletest/lib/Linux/Debug)
        LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googlemock/lib/Linux/Debug)
        LINK_DIRECTORIES(${THIRDPARTY_DIR}/googlebenchmark/lib/Linux)
    ELSE()
        LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googletest/lib/Linux/Release)
        LINK_DIRECTORIES(${THIRDPARTY_DIR}/googletest/googlemock/lib/Linux/Release)
        LINK_DIRECTORIES(${THIRDPARTY_DIR}/googlebenchmark/lib/Linux)
    ENDIF()
ENDIF()

INCLUDE(${LOCAL_TEST_SOURCE_DIR}/vtest/vtest_files.cmake)

ADD_LIBRARY(${PROJ_NAME_CORE} SHARED ${FILES})
TARGET_LINK_LIBRARIES(${PROJ_NAME_CORE} ${LIB_LINKS})