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

// TF ver 2.18.0

extern JavaVM* g_javaVM;
extern jobject g_mainActivityObj;

extern "C" {
jobject matToBitmap(JNIEnv* env, const cv::Mat& mat);
void callJavaUpdateOverlay(JNIEnv* env, jobject bitmap);
}

using namespace std;
using namespace tflite;

#define IPC_CHECK(x)                              \
  if (!(x)) {                                                \
    fprintf(stderr, "IPC FAILED. Error at %s:%d\n", __FILE__, __LINE__); \
    exit(1);                                               \
}


// offline.cc
int RunOfflineJNI(const char* modelname, const char* inputname) {
    cv::Mat image;
    InputPreprocess(inputname, image, false);

    // Model initialize
    {
        InterpreterManager interpreter_manager;

        IPC_CHECK(RecieveSchedulerSocket(SocketType::INIT,ThreadType::UNI, interpreter_manager) == StatusType::SUCCESS);
        interpreter_manager.init(ManagerType::INITIALIZER, modelname);
        IPC_CHECK(SendInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, interpreter_manager) == StatusType::SUCCESS);

        IPC_CHECK(RecieveSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
        interpreter_manager.prepare(InterpreterType::MAIN);
        IPC_CHECK(SendInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS)

        IPC_CHECK(RecieveSchedulerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
        interpreter_manager.run(InterpreterType::MAIN, modelname, image);
        IPC_CHECK(SendInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS)

        interpreter_manager.eval(InterpreterType::MAIN, modelname, image, 0, 0);
        interpreter_manager.clean();
    }

    {
        InterpreterManager interpreter_manager;

        IPC_CHECK(RecieveSchedulerSocket(SocketType::INIT,ThreadType::UNI, interpreter_manager) == StatusType::SUCCESS);
        interpreter_manager.init(ManagerType::MODEL_ANALYZER, modelname);
        IPC_CHECK(SendInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, interpreter_manager) == StatusType::SUCCESS);

        IPC_CHECK(RecieveSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
        interpreter_manager.prepare(InterpreterType::MAIN);
        interpreter_manager.segmentize(InterpreterType::MAIN);
        IPC_CHECK(SendInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS)

        IPC_CHECK(RecieveSchedulerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
        interpreter_manager.run(InterpreterType::MAIN, modelname, image);
        IPC_CHECK(SendInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS)

        interpreter_manager.eval(InterpreterType::MAIN, modelname, image, 0, 0);
        interpreter_manager.clean();
    }

    /* Mode 2: Segment Profiler (v1) */
    {
        InterpreterManager interpreter_manager;
        IPC_CHECK(RecieveSchedulerSocket(SocketType::INIT,ThreadType::UNI, interpreter_manager) == StatusType::SUCCESS);
        interpreter_manager.init(ManagerType::SEGMENT_PROFILER_V1, modelname);
        IPC_CHECK(SendInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, interpreter_manager) == StatusType::SUCCESS);

        for (int outer_loop_num=0; outer_loop_num<interpreter_manager.buffer_init.loop_num; outer_loop_num++){

            LOGI("===================== Case[%d] loop START =====================", outer_loop_num);

            std::promise<void> main_prepared;
            std::shared_future<void> main_ready_future = main_prepared.get_future();

            std::thread main([&]() {
                {
                    std::lock_guard<std::mutex> lock(interpreter_manager_mutex);
                    IPC_CHECK(RecieveSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
                    interpreter_manager.prepare(InterpreterType::MAIN);
                    IPC_CHECK(SendInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
                }
                main_prepared.set_value();

                IPC_CHECK(RecieveSchedulerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
                std::string dot_line = "Loop progress: ";
                for (int inner_loop_num = 0; inner_loop_num < interpreter_manager.buffer_init.inner_loop_num; inner_loop_num++) {
                    if(inner_loop_num % 2 == 1) {
                        dot_line += ".";
                        LOGI("%s", dot_line.c_str());
                    }
                    interpreter_manager.run(InterpreterType::MAIN, modelname, image);
                    interpreter_manager.eval(InterpreterType::MAIN, modelname, image,
                                             inner_loop_num, outer_loop_num);
                }
                IPC_CHECK(SendInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
            });

            std::thread sub([&]() {
                main_ready_future.wait();
                {
                    std::lock_guard<std::mutex> lock(interpreter_manager_mutex);
                    IPC_CHECK(RecieveSchedulerSocket(SocketType::PREPARE, ThreadType::SUB, interpreter_manager) == StatusType::SUCCESS);
                    interpreter_manager.prepare(InterpreterType::SUB);
                    IPC_CHECK(SendInterpreterManagerSocket(SocketType::PREPARE, ThreadType::SUB, interpreter_manager) == StatusType::SUCCESS);
                }

                IPC_CHECK(RecieveSchedulerSocket(SocketType::RUN, ThreadType::SUB, interpreter_manager) == StatusType::SUCCESS);
                for (int inner_loop_num = 0; inner_loop_num < interpreter_manager.buffer_init.inner_loop_num; inner_loop_num++) {
                    interpreter_manager.run(InterpreterType::SUB, modelname, image);
                    interpreter_manager.eval(InterpreterType::SUB, modelname, image, inner_loop_num,
                                             outer_loop_num);
                }
                IPC_CHECK(SendInterpreterManagerSocket(SocketType::RUN, ThreadType::SUB, interpreter_manager) == StatusType::SUCCESS);
            });

            main.join();
            sub.join();
            LOGI("Model profiled(v1)");
            interpreter_manager.clean();
        }
    }
    ///////////////////////////////

    ///////////////////////////////
    /* Mode 3: Segment Profiler (v2) */
    {
        InterpreterManager interpreter_manager;
        IPC_CHECK(RecieveSchedulerSocket(SocketType::INIT,ThreadType::UNI, interpreter_manager) == StatusType::SUCCESS);
        interpreter_manager.init(ManagerType::SEGMENT_PROFILER_V2, modelname);
        IPC_CHECK(SendInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, interpreter_manager) == StatusType::SUCCESS);

        for (int outer_loop_num=0; outer_loop_num<interpreter_manager.buffer_init.loop_num; outer_loop_num++){

            LOGI("===================== Case[%d] loop START =====================", outer_loop_num);

            std::promise<void> main_prepared;
            std::shared_future<void> main_ready_future = main_prepared.get_future();

            std::thread main([&]() {
                {
                    std::lock_guard<std::mutex> lock(interpreter_manager_mutex);
                    IPC_CHECK(RecieveSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
                    interpreter_manager.prepare(InterpreterType::MAIN);
                    IPC_CHECK(SendInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
                }
                main_prepared.set_value();

                IPC_CHECK(RecieveSchedulerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
                std::string dot_line = "Loop progress: ";
                for (int inner_loop_num = 0; inner_loop_num < interpreter_manager.buffer_init.inner_loop_num; inner_loop_num++) {
                    if(inner_loop_num % 2 == 1) {
                        dot_line += ".";
                        LOGI("%s", dot_line.c_str());
                    }
                    interpreter_manager.run(InterpreterType::MAIN, modelname, image);
                    interpreter_manager.eval(InterpreterType::MAIN, modelname, image,
                                             inner_loop_num, outer_loop_num);
                }
                IPC_CHECK(SendInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, interpreter_manager) == StatusType::SUCCESS);
            });

            std::thread sub([&]() {
                main_ready_future.wait();
                {
                    std::lock_guard<std::mutex> lock(interpreter_manager_mutex);
                    IPC_CHECK(RecieveSchedulerSocket(SocketType::PREPARE, ThreadType::SUB, interpreter_manager) == StatusType::SUCCESS);
                    interpreter_manager.prepare(InterpreterType::SUB);
                    IPC_CHECK(SendInterpreterManagerSocket(SocketType::PREPARE, ThreadType::SUB, interpreter_manager) == StatusType::SUCCESS);
                }

                IPC_CHECK(RecieveSchedulerSocket(SocketType::RUN, ThreadType::SUB, interpreter_manager) == StatusType::SUCCESS);
                for (int inner_loop_num = 0; inner_loop_num < interpreter_manager.buffer_init.inner_loop_num; inner_loop_num++) {
                    interpreter_manager.run(InterpreterType::SUB, modelname, image);
                    interpreter_manager.eval(InterpreterType::SUB, modelname, image, inner_loop_num,
                                             outer_loop_num);
                }
                IPC_CHECK(SendInterpreterManagerSocket(SocketType::RUN, ThreadType::SUB, interpreter_manager) == StatusType::SUCCESS);
            });

            main.join();
            sub.join();
            LOGI("Model profiled(v2)");
            interpreter_manager.clean();
        }
    }
    return 0;
}
