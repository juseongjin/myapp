//#pragma once
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "tensorflow/lite/core/scheduler.h"
#include "tensorflow/lite/shared_defs.h"
#include "tensorflow/lite/core/interpreter_manager.h" // 필수

// 소켓 경로 매크로 정의
#define INTERPRETER_MANAGER_SOCK_INIT ANDROID_HOME_DIR "sock/INTERPRETER_MANAGER_SOCK_INIT"
#define INTERPRETER_MANAGER_SOCK_PREPARE_MAIN ANDROID_HOME_DIR "sock/INTERPRETER_MANAGER_PREPARE_MAIN"
#define INTERPRETER_MANAGER_SOCK_PREPARE_SUB ANDROID_HOME_DIR "sock/INTERPRETER_MANAGER_PREPARE_SUB"
#define INTERPRETER_MANAGER_SOCK_RUN_MAIN ANDROID_HOME_DIR "sock/INTERPRETER_MANAGER_RUN_MAIN"
#define INTERPRETER_MANAGER_SOCK_RUN_SUB ANDROID_HOME_DIR "sock/INTERPRETER_MANAGER_RUN_SUB"
#define INTERPRETER_MANAGER_SOCK_CLEAN_MAIN ANDROID_HOME_DIR "LiteRT_app/sock/INTERPRETER_MANAGER_CLEAN_MAIN"
#define INTERPRETER_MANAGER_SOCK_CLEAN_SUB ANDROID_HOME_DIR "LiteRT_app/sock/INTERPRETER_MANAGER_CLEAN_SUB"
#define SCHEDULER_SOCK_INIT ANDROID_HOME_DIR "sock/SCHEDULER_INIT_UNI"
#define SCHEDULER_SOCK_PREPARE_MAIN ANDROID_HOME_DIR "sock/SCHEDULER_PREPARE_MAIN"
#define SCHEDULER_SOCK_PREPARE_SUB ANDROID_HOME_DIR "sock/SCHEDULER_PREPARE_SUB"
#define SCHEDULER_SOCK_RUN_MAIN ANDROID_HOME_DIR "sock/SCHEDULER_RUN_MAIN"
#define SCHEDULER_SOCK_RUN_SUB ANDROID_HOME_DIR "sock/SCHEDULER_RUN_SUB"
#define SCHEDULER_SOCK_CLEAN_MAIN ANDROID_HOME_DIR "LiteRT_app/sock/SCHEDULER_CLEAN_MAIN"
#define SCHEDULER_SOCK_CLEAN_SUB ANDROID_HOME_DIR "LiteRT_app/sock/SCHEDULER_CLEAN_SUB"

using namespace tflite;

// "선언만" 남깁니다.
StatusType SendSchedulerSocket(SocketType type, ThreadType thread_type, Scheduler &scheduler);
StatusType RecieveSchedulerSocket(SocketType type, ThreadType thread_type, InterpreterManager &interpreter_manager);
StatusType SendInterpreterManagerSocket(SocketType type, ThreadType thread_type, InterpreterManager &interpreter_manager);
StatusType RecieveInterpreterManagerSocket(SocketType type, ThreadType thread_type, Scheduler &scheduler);

// 실제 동작(구현)은 .cc에 들어감
StatusType SendSchedulerSocket_impl(SocketType type, ThreadType thread_type, Scheduler &scheduler);
StatusType RecieveSchedulerSocket_impl(SocketType type, ThreadType thread_type, InterpreterManager &interpreter_manager);
StatusType SendInterpreterManagerSocket_impl(SocketType type, ThreadType thread_type, InterpreterManager &interpreter_manager);
StatusType RecieveInterpreterManagerSocket_impl(SocketType type, ThreadType thread_type, Scheduler &scheduler);

void CleanSocketFiles();
