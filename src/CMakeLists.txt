cmake_minimum_required(VERSION 3.5.1)

project(rtsp2rtmp CXX)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../build)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -o0 ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdlib/ffmpeg4.2.3/include
                ${CMAKE_CURRENT_SOURCE_DIR}/.
		${CMAKE_CURRENT_SOURCE_DIR}/Queue)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/TransStream TRANS_STREAM_SRC)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdlib/ffmpeg4.2.3/lib)

add_executable(${PROJECT_NAME} ${TRANS_STREAM_SRC} Rtsp2Rtmp.cpp)
#add_library(${PROJECT_NAME} SHARED ${TRANS_STREAM_SRC} Rtsp2Rtmp.cpp)

target_link_libraries(${PROJECT_NAME} avcodec avformat avutil pthread)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "rtsp2rtmp")

install(TARGETS ${PROJECT_NAME} DESTINATION ${LIBRARY_OUTPUT_PATH})
