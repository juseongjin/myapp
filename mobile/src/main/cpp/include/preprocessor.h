#pragma once
#include "tensorflow/lite/core/interpreter_manager.h"
#include "tensorflow/lite/core/interpreter.h"
#include "tensorflow/lite/core/subgraph.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/shared_defs.h"

#include "opencv2/opencv_modules.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>

void InputPreprocess(const char* inputname, cv::Mat& image, bool is_video);
void InputImagePreprocess(const char* inputname, cv::Mat& image);
void InputVideoPreprocess(cv::Mat& image);