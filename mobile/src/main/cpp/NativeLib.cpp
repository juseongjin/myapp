#include <android/bitmap.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "app.h"
#include "scheduler.h"

JavaVM* g_javaVM = nullptr;
jobject g_mainActivityObj = nullptr;

#ifdef __cplusplus
extern "C" {
#endif

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("==================================");
    LOGI("JNI_OnLoad() called!");
    g_javaVM = vm;
    return JNI_VERSION_1_6;
}

// 액티비티 참조 저장 (MainActivity에서 반드시 호출)
JNIEXPORT void JNICALL
Java_com_example_myapp_MainActivity_nativeInit(JNIEnv* env, jobject thiz) {
    LOGI("JNI nativeInit() called!"); // <-- 이 줄을 추가
    if (g_mainActivityObj != nullptr) env->DeleteGlobalRef(g_mainActivityObj);
    g_mainActivityObj = env->NewGlobalRef(thiz);
    LOGI("nativeInit done");
}

// cv::Mat → Bitmap 변환 함수
jobject matToBitmap(JNIEnv* env, const cv::Mat& mat) {
    if (!env) {
        LOGE("matToBitmap: env is nullptr!");
        return nullptr;
    }
    if (mat.empty()) {
        LOGE("matToBitmap: mat is empty!");
        return nullptr;
    }

    int width = mat.cols;
    int height = mat.rows;

    int getEnvStat = g_javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);

    if (getEnvStat == JNI_EDETACHED) {
        LOGE("스레드가 JVM에 attach되어 있지 않음 (JNI_EDETACHED)");
        if (g_javaVM->AttachCurrentThread(&env, nullptr) != 0) {
            // Attach 실패 처리
            return nullptr;
        }
        // 반드시 AttachCurrentThread 필요!
    } else if (getEnvStat == JNI_OK) {
//        LOGI("스레드가 이미 JVM에 attach된 상태 (JNI_OK)");
    } else {
        LOGE("GetEnv에서 알 수 없는 에러 발생!");
    }

    jclass bitmapCls = env->FindClass("android/graphics/Bitmap");
    jmethodID createMethod = env->GetStaticMethodID(bitmapCls, "createBitmap",
                                                    "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    jclass configCls = env->FindClass("android/graphics/Bitmap$Config");
    jmethodID valueOfMethod = env->GetStaticMethodID(configCls, "valueOf",
                                                     "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;");

    jstring argbStr = env->NewStringUTF("ARGB_8888");
    jobject config = env->CallStaticObjectMethod(configCls, valueOfMethod, argbStr);
    env->DeleteLocalRef(argbStr);

    jobject bitmap = env->CallStaticObjectMethod(bitmapCls, createMethod, width, height, config);
    if (!bitmap) return nullptr;

    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) return nullptr;
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) return nullptr;

    void* pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) return nullptr;

    cv::Mat rgbaMat;
    if (mat.channels() == 1)
        cv::cvtColor(mat, rgbaMat, cv::COLOR_GRAY2RGBA);
    else if (mat.channels() == 3)
        cv::cvtColor(mat, rgbaMat, cv::COLOR_RGB2RGBA);
    else if (mat.channels() == 4)
        rgbaMat = mat;
    else {
        AndroidBitmap_unlockPixels(env, bitmap);
        return nullptr;
    }

    memcpy(pixels, rgbaMat.data, width * height * 4);
    AndroidBitmap_unlockPixels(env, bitmap);

    return bitmap;
}

// Java 콜백 호출 함수
void callJavaUpdateOverlay(JNIEnv* env, jobject bitmap) {
    if (!g_mainActivityObj || !bitmap) {
        LOGE("g_mainActivityObj or bitmap is null!");
        return;
    }
    jclass clazz = env->GetObjectClass(g_mainActivityObj);

    jclass classClass = env->FindClass("java/lang/Class");
    jmethodID getName = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
    jstring clsNameStr = (jstring)env->CallObjectMethod(clazz, getName);
    const char* clsName = env->GetStringUTFChars(clsNameStr, nullptr);
//    LOGI("JNI - 실제 클래스명: %s", clsName);
    env->ReleaseStringUTFChars(clsNameStr, clsName);
    env->DeleteLocalRef(clsNameStr);
    env->DeleteLocalRef(classClass);


    jmethodID mid = env->GetMethodID(clazz, "updateOverlayFrame", "(Landroid/graphics/Bitmap;)V");
    if (!mid) {
        LOGE("GetMethodID returned null for updateOverlayFrame");
        env->DeleteLocalRef(clazz);
        return;
    }
//    LOGI("Calling updateOverlayFrame method");
    env->CallVoidMethod(g_mainActivityObj, mid, bitmap);
    env->DeleteLocalRef(clazz);
}

// OFFLINE API
//////////////////////////////////////////////////////////////////////////////
JNIEXPORT jint JNICALL
Java_com_example_myapp_NativeLib_RunOfflineJNI(
        JNIEnv* env,
        jobject /* this */,
        jstring jmodelPath,
        jstring jinputPath) {

    const char* modelname = env->GetStringUTFChars(jmodelPath, nullptr);
    const char* inputname = env->GetStringUTFChars(jinputPath, nullptr);

    int result = RunOfflineJNI(modelname, inputname);

    env->ReleaseStringUTFChars(jmodelPath, modelname);
    env->ReleaseStringUTFChars(jinputPath, inputname);

    return result;
}

// ONLINE API
//////////////////////////////////////////////////////////////////////////////
JNIEXPORT jint JNICALL
Java_com_example_myapp_NativeLib_RunOnlineOurJNI(
        JNIEnv* env,
        jobject /* this */,
        jstring jmodelPath,
        jstring jinputPath) {

    const char* modelname = env->GetStringUTFChars(jmodelPath, nullptr);
    const char* inputname = env->GetStringUTFChars(jinputPath, nullptr);

    // env를 인자로 넘겨서 Java 콜백 가능하도록 함
    int result = RunOnlineOurJNI(modelname, inputname, env);

    env->ReleaseStringUTFChars(jmodelPath, modelname);
    env->ReleaseStringUTFChars(jinputPath, inputname);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_example_myapp_NativeLib_RunOnlineBaseJNI(
        JNIEnv* env,
        jobject /* this */,
        jstring jmodelPath,
        jstring jinputPath,
        jstring jproc) {

    const char* modelname = env->GetStringUTFChars(jmodelPath, nullptr);
    const char* inputname = env->GetStringUTFChars(jinputPath, nullptr);
    const char* proc = env->GetStringUTFChars(jproc, nullptr);

    // env를 인자로 넘겨서 Java 콜백 가능하도록 함
    int result = RunOnlineBaseJNI(modelname, inputname, proc, env);

    env->ReleaseStringUTFChars(jmodelPath, modelname);
    env->ReleaseStringUTFChars(jinputPath, inputname);

    return result;
}

// scheduler code
//////////////////////////////////////////////////////////////////////////////
JNIEXPORT void JNICALL
Java_com_example_myapp_NativeLib_RunOfflineSchedulerJNI(
        JNIEnv* env, jobject /* this */) {
    RunOfflineSchedulerJNI();
}

JNIEXPORT void JNICALL
Java_com_example_myapp_NativeLib_RunOnlineSchedulerOurJNI(
        JNIEnv* env, jobject /* this */) {
    RunOnlineSchedulerOurJNI();
}

JNIEXPORT void JNICALL
Java_com_example_myapp_NativeLib_RunOnlineSchedulerBaseCPUJNI(
        JNIEnv* env, jobject /* this */) {
    RunOnlineSchedulerBaseCPUJNI();
}

JNIEXPORT void JNICALL
Java_com_example_myapp_NativeLib_RunOnlineSchedulerBaseXNNJNI(
        JNIEnv* env, jobject /* this */) {
    RunOnlineSchedulerBaseXNNJNI();
}

JNIEXPORT void JNICALL
Java_com_example_myapp_NativeLib_RunOnlineSchedulerBaseGPUJNI(
        JNIEnv* env, jobject /* this */) {
    RunOnlineSchedulerBaseGPUJNI();
}
//////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
