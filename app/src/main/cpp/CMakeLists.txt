cmake_minimum_required(VERSION 3.22.1)

project("edgeviewer")

# OpenCV setup will go here later
# set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/../../../../opencv/sdk/native/jni)
# find_package(OpenCV REQUIRED)

add_library(
        edgeviewer
        SHARED
        native-lib.cpp)

find_library(
        log-lib
        log)

target_link_libraries(
        edgeviewer
        ${log-lib}
        # ${OpenCV_LIBS}
        )
