package com.example.myapp;
import android.util.Log;
public class NativeLib {
    static {
        System.loadLibrary("native-lib");
        Log.d("NativeLib", "native-lib 라이브러리 로드 완료");
    }
    // offline.cc에서 구현할 함수
    /////////////////////////////////////////////////////////////////
    public native int RunOfflineJNI(String modelPath, String inputPath);
    /////////////////////////////////////////////////////////////////

    // online.cc에서 구현할 함수
    /////////////////////////////////////////////////////////////////
    public native int RunOnlineOurJNI(String modelPath, String inputPath);
    public native int RunOnlineBaseJNI(String modelPath, String inputPath, String proc);
    /////////////////////////////////////////////////////////////////

    // scheduler.cc에서 구현할 함수
    /////////////////////////////////////////////////////////////////
    public native void RunOfflineSchedulerJNI();
    public native void RunOnlineSchedulerOurJNI();
    public native void RunOnlineSchedulerBaseCPUJNI();
    public native void RunOnlineSchedulerBaseXNNJNI();
    public native void RunOnlineSchedulerBaseGPUJNI();
    /////////////////////////////////////////////////////////////////
}
