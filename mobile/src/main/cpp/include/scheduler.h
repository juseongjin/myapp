#include "tensorflow/lite/core/interpreter_manager.h"
#include "tensorflow/lite/core/interpreter.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/core/subgraph.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/core/scheduler.h"
#include "tensorflow/lite/optional_debug_tools.h"
#include "tensorflow/lite/delegates/gpu/delegate.h"
#include "tensorflow/lite/shared_defs.h"

#include "ipc_socket.h"
#include "monitor.h"
#include "yaml-cpp/yaml.h"
#include "opencv2/opencv.hpp"
#include "opencv2/opencv_modules.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"

#include <chrono>
#include <thread>
#include <cstdio>
#include <vector>
#include <iostream>
#include <fstream>
#include <mutex>
#include <cmath>


using namespace std;
using namespace tflite;

#ifdef __cplusplus
extern "C" {
#endif

int RunOfflineSchedulerJNI();

int RunOnlineSchedulerOurJNI();
int RunOnlineSchedulerBaseCPUJNI();
int RunOnlineSchedulerBaseXNNJNI();
int RunOnlineSchedulerBaseGPUJNI();

#ifdef __cplusplus
}
#endif


