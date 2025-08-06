/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "app.h"
#include <android/log.h>

#define LOG_TAG "TFLITE_NATIVE"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
// TF ver 2.18.0

#define SLEEP_TIME 10000
#define VIDEO_FRAME_FPS 24

extern JavaVM* g_javaVM;
extern jobject g_mainActivityObj;

extern "C" {
    jobject matToBitmap(JNIEnv* env, const cv::Mat& mat);
    void callJavaUpdateOverlay(JNIEnv* env, jobject bitmap);
}

using namespace std;
using namespace tflite;
using namespace filesystem;

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::vector<std::string> GetSortedImagePaths(const std::string& directory) {
    std::vector<std::string> files;
    DIR* dir = opendir(directory.c_str());
    if (!dir) {
        LOGE("Failed to open directory: %s", directory.c_str());
        return files;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (entry->d_type == DT_REG &&
        ends_with(name, ".png") || ends_with(name, ".jpg") || ends_with(name, ".jpeg")) {
            files.emplace_back(directory + "/" + name);
        }
    }
    closedir(dir);
    std::sort(files.begin(), files.end());
    return files;
}

// online.cc
// 실제 프레임 반복 처리 함수 (online.cc에서 RunOnlineOurJNI을 이 함수를 호출하도록 하세요)

int RunOnlineOurJNI(const char* modelname, const char* inputname, JNIEnv* env) {
    double start = millis__(), end;
    const char* sub_input;
    cv::Mat image;
    InterpreterManager interpreter_manager;
    bool didAttach = false;

    IPC_CHECK(RecieveSchedulerSocket(SocketType::INIT, ThreadType::UNI, interpreter_manager) ==
              StatusType::SUCCESS);
    interpreter_manager.init(ManagerType::ONLINE_OURS, modelname);
    IPC_CHECK(
            SendInterpreterManagerSocket(SocketType::INIT, ThreadType::UNI, interpreter_manager) ==
            StatusType::SUCCESS);

    std::promise<void> main_prepared;
    std::shared_future<void> main_ready_future = main_prepared.get_future();
    std::mutex shared_mtx;
    std::condition_variable shared_cv;
    cv::Mat shared_frame;
    bool frame_ready = false;
    int inner_loop_num = 0;
    bool sync = false;

    std::thread main([&]() {
        {
            std::lock_guard<std::mutex> lock(interpreter_manager_mutex);
            IPC_CHECK(RecieveSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN,
                                             interpreter_manager) == StatusType::SUCCESS);
            double START = millis__();
            interpreter_manager.prepare(InterpreterType::MAIN);
            LOGI("Prepare time: %.3lfms", (millis__() - START));
            IPC_CHECK(SendInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN,
                                                   interpreter_manager) == StatusType::SUCCESS);
        }
        main_prepared.set_value();
        IPC_CHECK(RecieveSchedulerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) ==
                  StatusType::SUCCESS);

        if (!filesystem::is_directory(inputname)) {
            LOGE("%s is not exist", inputname);
        }

        std::vector<std::string> frame_paths = GetSortedImagePaths(inputname);
        if (frame_paths.empty()) {
            LOGE("No frames found in: %s", inputname);
            return;
        }

        float checkpoint = 0.0f;
        bool prepare_end = false;
        bool first_invoke = false;
        cv::Mat output;

        if (g_javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_EDETACHED) {
            if (g_javaVM->AttachCurrentThread(&env, nullptr) != 0) {
                LOGE("env is nullptr");
                return;
            }
            didAttach = true;
        }
        LOGI("%s=====================ONLINE START=====================%s", GREEN, RESET);

        end = millis__();
        for (const auto& frame_path : frame_paths) {
            sub_input = frame_path.c_str();

            if (!prepare_end) {
                checkpoint += (1.0f / VIDEO_FRAME_FPS) * 1000.0f;
//                LOGI("checkpoint: %.3lf, end - start: %.3lf", checkpoint, (end-start));
                if (checkpoint < (end - start)) {
                    inner_loop_num++;
                    if(inner_loop_num >= interpreter_manager.buffer_init.inner_loop_num) {
                        LOGI("%s=====================ONLINE FINISHED=====================%s", GREEN, RESET);
                        break;
                    }
                    LOGI("Not prepared (%.3lf/%.3lf)", checkpoint, (end-start) );
//                    LOGI("inner loop num %d", inner_loop_num);
                    InputPreprocess(frame_path.c_str(), image, false);
                    if (image.empty()) {
                        LOGE("Failed to load frame: %s", frame_path.c_str());
                        continue;
                    }
                    continue;
                }
                prepare_end = true;
                first_invoke = true;
                checkpoint = 0.0f;
            }

            if((inner_loop_num>0)&&(!first_invoke)){
                checkpoint += (1.0f/VIDEO_FRAME_FPS)*1000.0f;
                if(checkpoint<interpreter_manager.main_time_table[interpreter_manager.main_time_table.size()-1]){
                    LOGI("%sCurrent image(%d) is dropped%s", RED, inner_loop_num, RESET);
                    inner_loop_num++;
                    if(inner_loop_num >= interpreter_manager.buffer_init.inner_loop_num - 1) {
                        {
                            std::unique_lock<std::mutex> lock(shared_mtx);
                            sync = true;
                            frame_ready = false;       // 깨어나는 조건을 충족시키기 위해 필요
                            shared_cv.notify_one();   // 조건 변수 깨움
                        }
                        LOGI("%s=====================ONLINE FINISHED=====================%s", GREEN, RESET);
                        break;
                    }
                    continue;
                }
                checkpoint = 0.0f;
            }

            if (inner_loop_num < interpreter_manager.buffer_init.inner_loop_num) {
                {
                    std::unique_lock<std::mutex> lock(shared_mtx);
                    shared_frame = image.clone();
                    frame_ready = true;       // 깨어나는 조건을 충족시키기 위해 필요
                    shared_cv.notify_one();   // 조건 변수 깨움
                }

                InputPreprocess(frame_path.c_str(), image, false);
                if (image.empty()) {
                    LOGE("Failed to load frame: %s", frame_path.c_str());
                    continue;
                }
                // LOGI("image size: %d * %d", image.rows, image.cols);
                LOGI("%sCurrent image(%d) is processed%s", YELLOW, inner_loop_num, RESET);
                interpreter_manager.run(InterpreterType::MAIN, modelname, image);
                interpreter_manager.eval(InterpreterType::MAIN, modelname, image, inner_loop_num, 0, &output);
            }
//                LOGI("output size: %d x %d", output.rows, output.cols);
            if (output.empty()) {
                LOGE("[ERROR] eval() 호출 후 output이 nullptr입니다.");
            }
            jobject bitmap = matToBitmap(env, output);
            if (bitmap != nullptr) {
                callJavaUpdateOverlay(env, bitmap);
                env->DeleteLocalRef(bitmap);
            } else {
                LOGE("bitmap is null");
            }
//            LOGI("Main inner loop: %d", inner_loop_num);
            inner_loop_num++;
            first_invoke = false;
            if(inner_loop_num >= interpreter_manager.buffer_init.inner_loop_num){
                {
                    std::unique_lock<std::mutex> lock(shared_mtx);
                    sync = true;
                    frame_ready = false;       // 깨어나는 조건을 충족시키기 위해 필요
                    shared_cv.notify_one();   // 조건 변수 깨움
                }
                LOGI("%s=====================ONLINE FINISHED=====================%s", GREEN, RESET);
                break;
            }
        }
        if (didAttach) {
            g_javaVM->DetachCurrentThread();
        }

        IPC_CHECK(SendInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN,
                                               interpreter_manager) == StatusType::SUCCESS);
    });

    // 기존 sub 스레드 코드 유지
    std::thread sub([&]() {
        main_ready_future.wait();
        {
            std::lock_guard<std::mutex> lock(interpreter_manager_mutex);
            IPC_CHECK(RecieveSchedulerSocket(SocketType::PREPARE, ThreadType::SUB,
                                             interpreter_manager) == StatusType::SUCCESS);
            interpreter_manager.prepare(InterpreterType::SUB);
            IPC_CHECK(SendInterpreterManagerSocket(SocketType::PREPARE, ThreadType::SUB,
                                                   interpreter_manager) == StatusType::SUCCESS);
        }
        IPC_CHECK(RecieveSchedulerSocket(SocketType::RUN, ThreadType::SUB, interpreter_manager) ==
                  StatusType::SUCCESS);

        while(true) {
            cv::Mat sub_frame;
            {
                std::unique_lock<std::mutex> lock(shared_mtx);
//                shared_cv.wait(lock, [&] { return frame_ready;});
                shared_cv.wait(lock, [&] {
                    return frame_ready || sync;
                });

                if (frame_ready &&
                    inner_loop_num < interpreter_manager.buffer_init.inner_loop_num)
                    sub_frame = shared_frame.clone();
//                LOGI("Sub inner loop: %d", inner_loop_num);
                if(sync || inner_loop_num >= interpreter_manager.buffer_init.inner_loop_num) {
//                    LOGI("Sub is terminated");
                    break;
                }
                frame_ready = false;
            }

            // 영상 전처리
            InputPreprocess(sub_input, sub_frame, false);

            // 실행 및 평가
            interpreter_manager.run(InterpreterType::SUB, modelname, sub_frame);
            interpreter_manager.eval(InterpreterType::SUB, modelname, sub_frame, inner_loop_num, 0);
        }
        IPC_CHECK(SendInterpreterManagerSocket(SocketType::RUN, ThreadType::SUB,
                                               interpreter_manager) == StatusType::SUCCESS);
    });
    main.join();
    sub.join();

    interpreter_manager.clean();
    return 0;
}

int RunOnlineBaseJNI(const char* modelname, const char* inputname, const char* proc, JNIEnv* env) {
    double start = millis__(), end;
    cv::Mat image;
    InterpreterManager interpreter_manager;
    ManagerType base;
    if(strcmp(proc, "CPU") == 0) base = ManagerType::ONLINE_BASE_CPU;
    else if(strcmp(proc, "XNN") == 0) base = ManagerType::ONLINE_BASE_XNN;
    else if(strcmp(proc, "GPU") == 0) base = ManagerType::ONLINE_BASE_GPU;
    else base = ManagerType::ONLINE_BASE_CPU;

    IPC_CHECK(RecieveSchedulerSocket(SocketType::INIT, ThreadType::UNI, interpreter_manager) ==
              StatusType::SUCCESS);
    interpreter_manager.init(base, modelname);
    IPC_CHECK(
            SendInterpreterManagerSocket(SocketType::INIT, ThreadType::UNI, interpreter_manager) ==
            StatusType::SUCCESS);

    IPC_CHECK(RecieveSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
    double START = millis__();
    interpreter_manager.prepare(InterpreterType::MAIN);
    LOGI("Prepare time: %.3lfms", (millis__() - START));
    IPC_CHECK(SendInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
    IPC_CHECK(RecieveSchedulerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);

    if (!filesystem::is_directory(inputname)) {
        LOGE("%s is not exist", inputname);
    }

    std::vector<std::string> frame_paths = GetSortedImagePaths(inputname);
    if (frame_paths.empty()) {
        LOGE("No frames found in: %s", inputname);
        return -1;
    }

    int inner_loop_num = 0;
    float checkpoint = 0.0f;
    bool prepare_end = false;
    bool first_invoke = false;
    cv::Mat output;

    bool didAttach = false;
    if (g_javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_javaVM->AttachCurrentThread(&env, nullptr) != 0) {
            return -1;
        }
        didAttach = true;
    }
    LOGI("%s=====================BASLINE START=====================%s", GREEN, RESET);
    end = millis__();

    for (const auto& frame_path : frame_paths) {
        if (!prepare_end) {
            checkpoint += (1.0f / VIDEO_FRAME_FPS) * 1000.0f;
            // LOGI("checkpoint: %.3lf", checkpoint);
            // LOGI("end - start: %.3lf", end-start);
            if (checkpoint < (end - start)) {
                inner_loop_num++;
                if(inner_loop_num >= interpreter_manager.buffer_init.inner_loop_num) {
                    LOGI("%s=====================BASELINE FINISHED=====================%s", GREEN, RESET);
                    break;
                }
                LOGI("Not prepared (%.3lf/%.3lf)", checkpoint, (end-start) );
                InputPreprocess(frame_path.c_str(), image, false);
                if (image.empty()) {
                    LOGE("Failed to load frame: %s", frame_path.c_str());
                    continue;
                }

                continue;
            }
            prepare_end = true;
            first_invoke = true;
            checkpoint = 0.0f;
        }

        if((inner_loop_num>0)&&(!first_invoke)){
            checkpoint += (1.0f/VIDEO_FRAME_FPS)*1000.0f;
            if(checkpoint<interpreter_manager.main_time_table[interpreter_manager.main_time_table.size()-1]){
                LOGI("%sCurrent image(%d) is dropped%s", RED, inner_loop_num, RESET);
                inner_loop_num++;
                if(inner_loop_num >= interpreter_manager.buffer_init.inner_loop_num) {
                    LOGI("%s=====================BASELINE FINISHED=====================%s", GREEN, RESET);
                    break;
                }
                continue;
            }
            checkpoint = 0.0f;
        }

        if (inner_loop_num < interpreter_manager.buffer_init.inner_loop_num) {
            InputPreprocess(frame_path.c_str(), image, false);
            if (image.empty()) {
                LOGE("Failed to load frame: %s", frame_path.c_str());
                continue;
            }
            // LOGI("image size: %d x %d", image.rows, image.cols);
            LOGI("%sCurrent image(%d) is processed%s", YELLOW, inner_loop_num, RESET);
            interpreter_manager.run(InterpreterType::MAIN, modelname, image);
            interpreter_manager.eval(InterpreterType::MAIN, modelname, image, inner_loop_num, 0,
                                     &output);
        }
//        LOGI("output size: %d x %d", output.rows, output.cols);
        if (output.empty()) LOGE("[ERROR] eval() 호출 후 output이 nullptr입니다.");

        jobject bitmap = matToBitmap(env, output);
        if (bitmap != nullptr) {
            callJavaUpdateOverlay(env, bitmap);
            env->DeleteLocalRef(bitmap);
        } else {
            LOGE("bitmap is null");
        }

        inner_loop_num++;
        first_invoke = false;
        if(inner_loop_num >= interpreter_manager.buffer_init.inner_loop_num) {
            LOGI("%s=====================BASELINE FINISHED=====================%s", GREEN, RESET);
            break;
        }
    }
    if (didAttach) {
        g_javaVM->DetachCurrentThread();
    }

    IPC_CHECK(SendInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
    interpreter_manager.clean();
    return 0;

}