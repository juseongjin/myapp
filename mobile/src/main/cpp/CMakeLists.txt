cmake_minimum_required(VERSION 3.22.1)
project(MyNativeLib)

# 1. include/lib 경로 필요시 추가
include_directories(${CMAKE_SOURCE_DIR}/include)
#link_directories(${CMAKE_SOURCE_DIR}/libs)

#set(OpenCV_DIR "/home/seongjin/AndroidStudioProjects/objectdetection/ObjectDetection/mobile/src/main/cpp/third_party/opencv/lib")
#find_package(OpenCV REQUIRED)

# 2. native-lib(혹은 원하는 이름) 공유 라이브러리 생성
add_library(native-lib # Sets the name of the library.
        SHARED
        NativeLib.cpp
        offline.cc
        online.cc
        scheduler.cc
        ipc_socket.cc
        preprocessor.cc
        app.cc
)

# OpenCV .so 라이브러리 IMPORTED 등록
add_library(libopencv_core           SHARED IMPORTED)
add_library(libopencv_imgproc        SHARED IMPORTED)
add_library(libopencv_imgcodecs      SHARED IMPORTED)
add_library(libopencv_highgui        SHARED IMPORTED)
add_library(libopencv_video          SHARED IMPORTED)
add_library(libopencv_videoio        SHARED IMPORTED)
add_library(libopencv_calib3d        SHARED IMPORTED)
add_library(libopencv_features2d     SHARED IMPORTED)
add_library(libopencv_flann          SHARED IMPORTED)
add_library(libopencv_photo          SHARED IMPORTED)
add_library(libopencv_ml             SHARED IMPORTED)
add_library(libopencv_objdetect      SHARED IMPORTED)
add_library(libopencv_dnn            SHARED IMPORTED)
add_library(libopencv_gapi           SHARED IMPORTED)
add_library(libopencv_stitching      SHARED IMPORTED)

# 경로 설정 (ANDROID_ABI는 예: arm64-v8a)
set(OpenCV_LIB_PATH ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})

set_target_properties(libopencv_core           PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_core.so)
set_target_properties(libopencv_imgproc        PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_imgproc.so)
set_target_properties(libopencv_imgcodecs      PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_imgcodecs.so)
set_target_properties(libopencv_highgui        PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_highgui.so)
set_target_properties(libopencv_video          PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_video.so)
set_target_properties(libopencv_videoio        PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_videoio.so)
set_target_properties(libopencv_calib3d        PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_calib3d.so)
set_target_properties(libopencv_features2d     PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_features2d.so)
set_target_properties(libopencv_flann          PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_flann.so)
set_target_properties(libopencv_photo          PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_photo.so)
set_target_properties(libopencv_ml             PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_ml.so)
set_target_properties(libopencv_objdetect      PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_objdetect.so)
set_target_properties(libopencv_dnn            PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_dnn.so)
set_target_properties(libopencv_gapi           PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_gapi.so)
set_target_properties(libopencv_stitching      PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_stitching.so)


add_library( libopencv_java4 SHARED IMPORTED )
set_target_properties( libopencv_java4 PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libopencv_java4.so )

# 3. prebuilt shared object(libtensorflowlite.so 등) 사용할 경우
add_library( libtensorflowlite SHARED IMPORTED )
set_target_properties( libtensorflowlite PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libtensorflowlite.so )

add_library( libtensorflowlite_gpu_delegate SHARED IMPORTED )
set_target_properties( libtensorflowlite_gpu_delegate PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libtensorflowlite_gpu_delegate.so )


target_include_directories(native-lib PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/tensorflow
        ${CMAKE_SOURCE_DIR}/third_party/flatbuffers
        ${CMAKE_SOURCE_DIR}/third_party/flatbuffers/include/
        ${CMAKE_SOURCE_DIR}/third_party/opencv/include  # opencv 헤더의 상위(=opencv2) 폴더
        ${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/include        # yaml-cpp 헤더의 상위(=yaml-cpp) 폴더
)


set(OPENCV_3RDPARTY_LIBS
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/libade.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/libIlmImf.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/liblibjpeg-turbo.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/liblibpng.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/liblibtiff.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/libquirc.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/libcpufeatures.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/libittnotify.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/liblibopenjp2.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/liblibprotobuf.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/liblibwebp.a
        ${CMAKE_SOURCE_DIR}/third_party/opencv/3rdparty/libtegra_hal.a
)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
set(CMAKE_CXX_STANDARD 17)

# 반드시 필요
set(CMAKE_ANDROID_STL_TYPE c++_shared)


# 4. 필요하다면 링크
target_link_libraries(native-lib
        ${OPENCV_3RDPARTY_LIBS}
        libopencv_java4
        libtensorflowlite
        libtensorflowlite_gpu_delegate
        libopencv_core
        libopencv_imgproc
        libopencv_dnn
        libopencv_features2d
        libopencv_flann
        libopencv_highgui
        libopencv_imgcodecs
        libopencv_ml
        libopencv_objdetect
        libopencv_photo
        libopencv_stitching
        libopencv_video
        libopencv_videoio
        libopencv_gapi
        android
        jnigraphics
        log
        mediandk
        z
        atomic
        m
)

