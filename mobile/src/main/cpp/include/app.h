#pragma once
#include "tensorflow/lite/core/interpreter.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/core/subgraph.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/core/scheduler.h"
#include "tensorflow/lite/core/interpreter_manager.h"
#include "tensorflow/lite/optional_debug_tools.h"
#include "tensorflow/lite/delegates/gpu/delegate.h"
#include "tensorflow/lite/shared_defs.h"

#include "ipc_socket.h"
#include "preprocessor.h"
#include "yaml-cpp/yaml.h"
#include "monitor.h"
#include "opencv2/opencv_modules.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"

#include <dirent.h>     // opendir, readdir
#include <sys/stat.h>   // stat
#include <chrono>
#include <thread>
#include <cstdio>
#include <vector>
#include <mutex>
#include <iostream>
#include <fstream>
#include <android/log.h>
#include <jni.h>
#include <filesystem>
#include <string>
#include <algorithm>

#define RED     "\033[0;31m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define BLUE    "\033[0;34m"
#define RESET   "\033[0m"

#define LOG_TAG "TFLITE_NATIVE"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace std;
using namespace tflite;

extern std::mutex interpreter_manager_mutex;

#define IPC_CHECK(x)                              \
  if (!(x)) {                                                \
    fprintf(stderr, "IPC FAILED. Error at %s:%d\n", __FILE__, __LINE__); \
    exit(1);                                               \
}

#ifdef __cplusplus
extern "C" {
#endif

// JNI에서 사용할 함수 선언 추가
int RunOfflineJNI(const char* modelname, const char* inputname);

int RunOnlineOurJNI(const char* modelname, const char* inputname, JNIEnv* env);
int RunOnlineBaseJNI(const char* modelname, const char* inputname, const char* proc, JNIEnv* env);

#ifdef __cplusplus
}
#endif

// interpreter
StatusType RecieveSchedulerSocket(SocketType socket_type, ThreadType thread_type, /*src*/ InterpreterManager &interpreter_manager);

StatusType SendInterpreterManagerSocket(SocketType socket_type, ThreadType thread_type, /*dst*/ InterpreterManager &interpreter_manager);

// scheduler
StatusType SendSchedulerSocket(SocketType socket_type, ThreadType thread_type, /*src*/ Scheduler &scheduler);

StatusType RecieveInterpreterManagerSocket(SocketType socket_type, ThreadType thread_type, /*dst*/ Scheduler &scheduler);

uint64_t millis__();