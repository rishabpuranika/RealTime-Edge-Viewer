cmake_minimum_required(VERSION 3.22.1)

project("edgeviewer")

# OpenCV setup
set(OpenCV_DIR /home/rishvaish/Android/OpenCV-android-sdk/sdk/native/jni)
set(OpenCV_SHARED OFF)
find_package(OpenCV REQUIRED)

# Manually set include directory
include_directories(/home/rishvaish/Android/OpenCV-android-sdk/sdk/native/jni/include)

add_library(
        edgeviewer
        SHARED
        native-lib.cpp)

find_library(
        log-lib
        log)

target_link_libraries(
        edgeviewer
        PRIVATE
        ${log-lib}
        ${OpenCV_LIBS}
        GLESv2
        EGL
        android
        )
