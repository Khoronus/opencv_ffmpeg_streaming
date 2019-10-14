project(simple_opencv_streaming)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set ( CMAKE_EXE_LINKER_FLAGS_RELEASE  "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -std=c++11 -Wall ")
set ( CMAKE_EXE_LINKER_FLAGS_DEBUG  "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined ")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  -std=c++11 -Wall -O0 -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -D__FORTIFY_SOURCE=2 -rdynamic -DDEBUG")

#set ( CMAKE_EXE_LINKER_FLAGS_DEBUG  "${CMAKE_EXE_LINKER_FLAGS_DEBUG}  ")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  -std=c++11 -Wall -O0 -fno-omit-frame-pointer ")

find_package(OpenCV REQUIRED)

#find_package(Qt5Core)

include(CMakeToolsHelpers OPTIONAL)

include_directories(${OpenCV_INCLUDE_DIRS} 
  "D:/workspace/programs/ThirdPartyProg/ffmpeg-msvc-build/ffmpeg"
  "D:/workspace/programs/ThirdPartyProg/ffmpeg-msvc-build/ffmpeg/fftools"
  "D:/workspace/programs/ThirdPartyLib/SDL2-2.0.10/include"
  "D:/workspace/university/todai/BirdView3D/src/Memory/SharedMemoryCStyle/code"
)

add_subdirectory(streamer)

add_executable(simple_opencv_streaming main.c "D:/workspace/programs/ThirdPartyProg/ffmpeg-msvc-build/FFmpeg/fftools/cmdutils.c")
target_compile_options(simple_opencv_streaming PRIVATE "/TC")

find_library(AVCODEC_LIBRARY avcodec)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWSCALE_LIBRARY swscale)

set(AVLIB 
  "D:/workspace/programs/ThirdPartyLib/ffmpeg-20190926-525de95-win64-dev/lib/avdevice.lib"
  "D:/workspace/programs/ThirdPartyLib/ffmpeg-20190926-525de95-win64-dev/lib/swresample.lib"
  "D:/workspace/programs/ThirdPartyLib/ffmpeg-20190926-525de95-win64-dev/lib/postproc.lib"
)

MESSAGE(${AVFORMAT_LIBRARY})

set(SDL2LIBRARY 
  "D:/workspace/programs/ThirdPartyLib/SDL2-2.0.10/lib/x64/sdl2.lib" 
  "D:/workspace/programs/ThirdPartyLib/SDL2-2.0.10/lib/x64/sdl2main.lib"
)

set(SHAREDSTYLE
  "D:/workspace/university/todai/BirdView3D/src/Memory/SharedMemoryCStyle/build/code/release/SharedMemoryCStyle.lib"
)

target_link_libraries(simple_opencv_streaming  streamer ${OpenCV_LIBS} ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY} ${AVLIB} ${SDL2LIBRARY} ${SHAREDSTYLE})
#target_link_libraries(simple_opencv_streaming avcodec avformat avutil swscale avfilter avdevice