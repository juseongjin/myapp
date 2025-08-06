#include "scheduler.h"
#include "monitor.h"

#include <android/log.h>
#define LOG_TAG "TFLITE_NATIVE"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace std;
using namespace tflite;

#define IPC_CHECK(x)                              \
  if (!(x)) {                                                \
    fprintf(stderr, "IPC FAILED. Error at %s:%d\n", __FILE__, __LINE__); \
    exit(1);                                                 \
}

int RunOfflineSchedulerJNI() {
    std::vector<float> util(2, -1);
    float cpu_sum = 0.0f;
    float gpu_sum = 0.0f;
    int count = 0;

    /* Model 0: Initialized */
    {
        LOGI("===================");
        Scheduler scheduler;
        const char*  yamlname = "/data/data/com.example.myapp/dummy.yaml";
        if (yamlname == nullptr) {
            // 에러 처리 또는 로그
            return -1;
        }
        LOGI("Yaml path = %s (In shceduler.cc)", yamlname);
        scheduler.init(yamlname);
        IPC_CHECK(SendSchedulerSocket(SocketType::INIT, ThreadType::UNI, scheduler) == StatusType::SUCCESS);
        IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, scheduler) == StatusType::SUCCESS);

        for (int outer_loop_num=0;outer_loop_num<scheduler.buffer_init.loop_num;outer_loop_num++){

            util[0] = 0.0;
            util[1] = 0.0;
            scheduler.prepare(yamlname, outer_loop_num, util);
            LOGI("Prepare done");
            IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            scheduler.run();
            LOGI("Run done");
            IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);

            scheduler.eval();
            LOGI("Eval done");
            scheduler.clean();
        }
        CleanSocketFiles();
        LOGI("Model initialized");
        LOGI("===================");
    }

    ///////////////////////////////
    /* Mode 1: Model Analyzer */
    {
        LOGI("===================");
        LOGI("Model analysis start");
        Scheduler scheduler;
        const char*  yamlname = "/data/data/com.example.myapp/model_whole/yolo_v4_tiny.yaml";
        if (yamlname == nullptr) {
            // 에러 처리 또는 로그
            return -1;
        }
        LOGI("Yaml path = %s (In shceduler.cc)", yamlname);
        scheduler.init(yamlname);
        IPC_CHECK(SendSchedulerSocket(SocketType::INIT, ThreadType::UNI, scheduler) == StatusType::SUCCESS);
        IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, scheduler) == StatusType::SUCCESS);

        for (int outer_loop_num=0;outer_loop_num<scheduler.buffer_init.loop_num;outer_loop_num++){
            LOGI("outer loop %d, buff outer loop %d", outer_loop_num, scheduler.buffer_init.loop_num);

            printf( YELLOW "===================== Case[%d]'s loop START =====================\n" RESET,
                    outer_loop_num);

            util[0] = 0.0;
            util[1] = 0.0;
            scheduler.prepare(yamlname, outer_loop_num, util);

            IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);

            scheduler.run();

            IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);

            scheduler.eval();
            scheduler.clean();
        }
        CleanSocketFiles();
        LOGI("Model analyzed");
        LOGI("===================");
    }
    ///////////////////////////////

    ///////////////////////////////
    /* Mode 2: Segment Profiler (v1) */
    {
        Scheduler scheduler;
        const char*  yamlname = "/data/data/com.example.myapp/segments/yolo_v4_tiny.yaml";
        if (yamlname == nullptr) {
            // 에러 처리 또는 로그
            return -1;
        }
        LOGI("Yaml path = %s (In shceduler.cc)", yamlname);
        scheduler.init(yamlname);
        IPC_CHECK(SendSchedulerSocket(SocketType::INIT, ThreadType::UNI, scheduler) == StatusType::SUCCESS);
        IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, scheduler) == StatusType::SUCCESS);

        for (int outer_loop_num=0;outer_loop_num<scheduler.buffer_init.loop_num;outer_loop_num++){

            auto start = std::chrono::steady_clock::now();
            auto end = start;

            while (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() < 30) {
                monitor::GetUtilization(util);
                if (std::isinf(util[0]) || std::isinf(util[1])) {
                    end = std::chrono::steady_clock::now();
                    continue;
                }
                cpu_sum += util[0];
                gpu_sum += util[1];
                ++count;
                std::cout << std::fixed << std::setprecision(6);

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                end = std::chrono::steady_clock::now();
            }
            util[0] = cpu_sum/(float)count;
            util[1] = gpu_sum/(float)count;
            scheduler.prepare(yamlname, outer_loop_num, util);

            IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::SUB, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::SUB, scheduler) == StatusType::SUCCESS);

            scheduler.run();

            std::thread main_thread_([&]() {
                IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
                IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            });
            std::thread sub_thread_([&]() {
                IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::SUB, scheduler) == StatusType::SUCCESS);
                IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::SUB, scheduler) == StatusType::SUCCESS);
            });
            main_thread_.join();
            sub_thread_.join();
            scheduler.eval();
            scheduler.clean();
        }
        CleanSocketFiles();
    }
    ///////////////////////////////

    ///////////////////////////////
    /* Mode 3: Segment Profiler (v2) */
    {
        Scheduler scheduler;
        const char* yamlname = "/data/data/com.example.myapp/segments/yolo_v4_tiny.yaml";
        if (yamlname == nullptr) {
            // 에러 처리 또는 로그
            return -1;
        }
        LOGI("Yaml path = %s (In shceduler.cc)", yamlname);
        scheduler.init(yamlname);
        IPC_CHECK(SendSchedulerSocket(SocketType::INIT, ThreadType::UNI, scheduler) == StatusType::SUCCESS);
        IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, scheduler) == StatusType::SUCCESS);

        for (int outer_loop_num=0;outer_loop_num<scheduler.buffer_init.loop_num;outer_loop_num++){

            auto start = std::chrono::steady_clock::now();
            auto end = start;

            while (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() < 30) {
                monitor::GetUtilization(util);
                if (std::isinf(util[0]) || std::isinf(util[1])) {
                    end = std::chrono::steady_clock::now();
                    continue;
                }
                cpu_sum += util[0];
                gpu_sum += util[1];
                ++count;
                std::cout << std::fixed << std::setprecision(6);

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                end = std::chrono::steady_clock::now();
            }
            util[0] = cpu_sum/(float)count;
            util[1] = gpu_sum/(float)count;
            scheduler.prepare(yamlname, outer_loop_num, util);

            IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::SUB, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::SUB, scheduler) == StatusType::SUCCESS);

            scheduler.run();

            std::thread main_thread_([&]() {
                IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
                IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            });
            std::thread sub_thread_([&]() {
                IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::SUB, scheduler) == StatusType::SUCCESS);
                IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::SUB, scheduler) == StatusType::SUCCESS);
            });
            main_thread_.join();
            sub_thread_.join();
            scheduler.eval();
            scheduler.clean();
        }
        CleanSocketFiles();
    }
    ///////////////////////////////
    return 0;
}

int RunOnlineSchedulerOurJNI() {
    const char *yamlname = "/data/data/com.example.myapp/preset/yolo_v4_tiny.yaml";

    std::vector<float> util(2, -1);
    float cpu_sum = 0.0f;
    float gpu_sum = 0.0f;
    int count = 0;
    Scheduler scheduler;

    auto start = std::chrono::steady_clock::now();
    auto end = start;


    scheduler.init(yamlname);
    IPC_CHECK(SendSchedulerSocket(SocketType::INIT, ThreadType::UNI, scheduler) ==
              StatusType::SUCCESS);
    IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::INIT, ThreadType::UNI, scheduler) ==
              StatusType::SUCCESS);

    for (int outer_loop_num=0;outer_loop_num<1;outer_loop_num++){
        while (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() < 30) {
            monitor::GetUtilization(util);
            if (std::isinf(util[0]) || std::isinf(util[1])) {
                end = std::chrono::steady_clock::now();
                continue;
            }
            cpu_sum += util[0];
            gpu_sum += util[1];
            ++count;
            std::cout << std::fixed << std::setprecision(6);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            end = std::chrono::steady_clock::now();
        }

        util[0] = cpu_sum/(float)count;
        util[1] = gpu_sum/(float)count;

        scheduler.prepare(yamlname, outer_loop_num, util);

        IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
        IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
        IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::SUB, scheduler) == StatusType::SUCCESS);
        IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::SUB, scheduler) == StatusType::SUCCESS);

        scheduler.run();

        std::thread main_thread_([&]() {
            IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
        });
        std::thread sub_thread_([&]() {
            IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::SUB, scheduler) == StatusType::SUCCESS);
            IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::SUB, scheduler) == StatusType::SUCCESS);
        });
        main_thread_.join();
        sub_thread_.join();

        scheduler.eval();
        scheduler.clean();
    }
    CleanSocketFiles();

    return 0;
}

int RunOnlineSchedulerBaseCPUJNI () {
    const char* yamlname = "/data/data/com.example.myapp/preset/yolo_v4_tiny_cpu.yaml";
    Scheduler scheduler;

    std::vector<float> util(2, 0.0); // not used in baseline

    scheduler.init(yamlname);
    IPC_CHECK(SendSchedulerSocket(SocketType::INIT, ThreadType::UNI, scheduler) == StatusType::SUCCESS);
    IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, scheduler) == StatusType::SUCCESS);

    scheduler.prepare(yamlname, 0, util);

    IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
    IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);

    scheduler.run();

    IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
    IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);

    scheduler.eval();
    scheduler.clean();
    CleanSocketFiles();

    return 0;
}

int RunOnlineSchedulerBaseXNNJNI () {
    const char* yamlname = "/data/data/com.example.myapp/preset/yolo_v4_tiny_xnn.yaml";
    Scheduler scheduler;

    std::vector<float> util(2, 0.0); // not used in baseline

    scheduler.init(yamlname);
    IPC_CHECK(SendSchedulerSocket(SocketType::INIT, ThreadType::UNI, scheduler) == StatusType::SUCCESS);
    IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, scheduler) == StatusType::SUCCESS);

    scheduler.prepare(yamlname, 0, util); /* 0 is XNN */

    IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
    IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);

    scheduler.run();

    IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
    IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);

    scheduler.eval();
    scheduler.clean();
    CleanSocketFiles();
    return 0;
}

int RunOnlineSchedulerBaseGPUJNI () {
    const char* yamlname = "/data/data/com.example.myapp/preset/yolo_v4_tiny_gpu.yaml";
    Scheduler scheduler;

    std::vector<float> util(2, 0.0); // not used in baseline

    scheduler.init(yamlname);
    IPC_CHECK(SendSchedulerSocket(SocketType::INIT, ThreadType::UNI, scheduler) == StatusType::SUCCESS);
    IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::INIT,ThreadType::UNI, scheduler) == StatusType::SUCCESS);

    scheduler.prepare(yamlname, 0, util); /* 0 is GPU */

    IPC_CHECK(SendSchedulerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
    IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::PREPARE, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);

    scheduler.run();

    IPC_CHECK(SendSchedulerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);
    IPC_CHECK(RecieveInterpreterManagerSocket(SocketType::RUN, ThreadType::MAIN, scheduler) == StatusType::SUCCESS);

    scheduler.eval();
    scheduler.clean();
    CleanSocketFiles();
    return 0;
}