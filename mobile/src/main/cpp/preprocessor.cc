#include "preprocessor.h"

using namespace tflite;

void InputPreprocess(const char* inputname, cv::Mat& image, bool is_video){
    if(is_video) InputVideoPreprocess(image);
    else{
        InputImagePreprocess(inputname, image);
    }
}

void InputImagePreprocess(const char* inputname, cv::Mat& image){
    image = cv::imread(inputname, cv::IMREAD_COLOR);
    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
    cv::resize(image, image, cv::Size(416,416));
}

void InputVideoPreprocess(cv::Mat& image){
    cv::resize(image, image, cv::Size(416,416));
}