cmake_minimum_required(VERSION 3.5)
project(LRU)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1)
FetchContent_MakeAvailable(SFML)

add_executable(LRU 
                src/main.cpp
                src/cache/cache.hpp
                )

if(WIN32)    
    target_link_directories(LRU PRIVATE "external/lib/win32/mupdf")
    target_link_libraries(LRU PRIVATE libmupdf)
elseif(UNIX)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ")
    target_link_directories(LRU PRIVATE "external/lib/unix/mupdf")
    target_link_libraries(LRU PRIVATE mupdf mupdf-third)
endif()

include_directories("external/include")
target_link_libraries(LRU PRIVATE sfml-graphics)
target_compile_features(LRU PRIVATE cxx_std_17)

if(WIN32)
    add_custom_command(
        TARGET LRU
        COMMENT "Copy OpenAL DLL"
        PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:LRU>
        VERBATIM)
endif()

install(TARGETS LRU)


