package com.example.myapp;

import android.os.Handler;
import android.os.Looper;

import android.util.Log;

//import com.example.myapp.CustomVideo;
import android.widget.VideoView;
import android.os.Bundle;
import android.view.View;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.Toast;
import android.widget.FrameLayout;
import androidx.appcompat.app.AppCompatActivity;
import android.net.Uri;                     // Uri 클래스
import android.graphics.Color;               // Color 관련
import android.util.TypedValue;              // TypedValue, 단위 변환용
import android.view.Gravity;                 // Gravity (텍스트 중앙정렬 등)
import android.widget.ImageView;
import android.graphics.Bitmap;

import java.io.File;
import java.io.InputStream;                  // InputStream (assets 파일 읽기용)
import java.io.FileOutputStream;             // FileOutputStream (파일 쓰기용)
import java.io.IOException;                  // IOException (try/catch 예외 처리용)

public class MainActivity extends AppCompatActivity {

    private NativeLib nativeLib;
    public native void nativeInit();

    // 방향지시등 관련
    private Handler indicatorHandler = new Handler(Looper.getMainLooper());
    private boolean indicatorBlinking = false;
    private Runnable indicatorBlinkRunnable;
    private boolean isProcessing = false;

    private final String modelAssetPath = "yolo_v4_tiny.tflite"; // assets 내 모델 파일명
    private final String inputAssetPath = "person.jpg"; // assets 내 입력 이미지 파일명
    private final String FrontCamImitaion = "/data/local/tmp/scene1_24fps.mp4";
    private final String MusicPlay = "/data/local/tmp/output.mp4";
    private final String OnlineInputPath = "/data/local/tmp/scene2";

    private ImageView overlayView;
    private VideoView musicView;
    ImageView rightBlinkView;
    ImageView leftBlinkView;
    ImageView steeringWheelView;

    private Spinner modeSpinner;

    // 현재 선택된 모드 저장 (1:OURS, 2:BASE CPU, 4:BASE GPU, 3:BASE XNN)
    private int selectedMode;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        nativeLib = new NativeLib();
        nativeInit();          // 반드시 onCreate에서 호출! (this 전달됨)

        Log.d("MainActivity", "nativeInit() called from Java");
        Toast.makeText(this, "nativeInit() 호출", Toast.LENGTH_SHORT).show();

        FrameLayout rootLayout = new FrameLayout(this);
        rootLayout.setBackgroundColor(Color.BLACK);

        // musicView 설정 (가로는 화면 꽉 채우고, 세로는 화면 절반, 아래쪽에 위치)
//        musicView = new VideoView(this);
//        musicView.setVisibility(View.INVISIBLE); // 보이지 않게 (오디오만)
//        int screenWidth = getResources().getDisplayMetrics().widthPixels;
//        int screenHeight = getResources().getDisplayMetrics().heightPixels;
//        int videoHeight = screenHeight / 2;
//        int bottomMargin = (int) TypedValue.applyDimension(
//                TypedValue.COMPLEX_UNIT_DIP, 16, getResources().getDisplayMetrics());
//        FrameLayout.LayoutParams musicParams = new FrameLayout.LayoutParams(
//                screenWidth,
//                videoHeight,
//                Gravity.BOTTOM
//        );
//        musicParams.setMargins(0, 0, 0, bottomMargin);
//        musicView.setLayoutParams(musicParams);
//        rootLayout.addView(musicView);
        musicView = new VideoView(this);
        rootLayout.addView(musicView);

        // overlayView 설정
        overlayView = new ImageView(this);
        overlayView.setVisibility(View.GONE);
//        int overlaySizePx = (int) TypedValue.applyDimension(
//                TypedValue.COMPLEX_UNIT_DIP, 150, getResources().getDisplayMetrics());
        int overlaySizePx = 480;
        int horizon_offsetPx = 650;
        int vertical_offsetPx = 200;
        FrameLayout.LayoutParams overlayLayoutParams = new FrameLayout.LayoutParams(
                overlaySizePx,
                overlaySizePx,
                Gravity.CENTER
        );
        overlayLayoutParams.setMargins(-horizon_offsetPx, vertical_offsetPx, 0, 0);
        rootLayout.addView(overlayView, overlayLayoutParams);

        // 스피너 생성 및 설정
        modeSpinner = new Spinner(this);
        String[] modeItems = {"OFFLINE", "OURS", "BASE CPU", "BASE GPU", "BASE XNN"};
        ArrayAdapter<String> adapter = new ArrayAdapter<>(
                this,
                R.layout.simple_spinner_item_white,   // 커스텀 layout 사용
                modeItems
        );
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        modeSpinner.setAdapter(adapter);

        // 오른쪽 아래에 스피너 배치
        // 스피너 레이아웃 파라미터 수정 (최상단 중앙에 위치)
        FrameLayout.LayoutParams spinnerLayoutParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.TOP | Gravity.START);

        // 마진 아주 작게 (예: 4dp → px 변환)
        int margin = (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP, 4, getResources().getDisplayMetrics());
        spinnerLayoutParams.setMargins(margin, margin, margin, margin);
        modeSpinner.setLayoutParams(spinnerLayoutParams);

        modeSpinner.setAdapter(adapter);

        rootLayout.addView(modeSpinner, spinnerLayoutParams);

        createAppDirectories();

        // 스피너 선택 리스너 - 선택된 모드 변수에 저장
        modeSpinner.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(android.widget.AdapterView<?> parent, View view, int position, long id) {
                switch (position) {
                    case 0: selectedMode = 0; break; // OFFLINE
                    case 1: selectedMode = 1; break; // OURS
                    case 2: selectedMode = 2; break; // BASE CPU
                    case 3: selectedMode = 4; break; // BASE GPU
                    case 4: selectedMode = 3; break; // BASE XNN
                }
            }
            @Override
            public void onNothingSelected(android.widget.AdapterView<?> parent) {
                // 아무것도 선택 안 됐을 때 처리할 게 있으면
            }
        });

        int screenWidth2 = getResources().getDisplayMetrics().widthPixels;
        int screenHeight2 = getResources().getDisplayMetrics().heightPixels;
        int quad2_width = screenWidth2 / 2;
        int quad2_height = screenHeight2 / 2;

        // 이미지 크기/간격 dp→px 변환
        int indicatorPx = (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP, 48, getResources().getDisplayMetrics());
        int steeringPx = (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP, 64, getResources().getDisplayMetrics());

        int totalImageWidth = 2 * indicatorPx + steeringPx;
        int spacing = (quad2_width - totalImageWidth) / 4;
        int x_left_blink = spacing;
        int x_steering = x_left_blink + indicatorPx + spacing;
        int x_right_blink = x_steering + steeringPx + spacing;
        int mid_y = quad2_height / 2;

        // 컨테이너 (FrameLayout) 생성: 제2사분면(left top 1/4)만큼만 크기
        FrameLayout quad2Container = new FrameLayout(this);
        FrameLayout.LayoutParams quad2Params = new FrameLayout.LayoutParams(
                quad2_width, quad2_height, Gravity.TOP | Gravity.START);
        quad2Container.setLayoutParams(quad2Params);

        // 1. left_blink_off
        leftBlinkView = new ImageView(this);
        leftBlinkView.setImageResource(R.drawable.left_blink_off);
        FrameLayout.LayoutParams leftParams = new FrameLayout.LayoutParams(indicatorPx, indicatorPx);
        leftParams.leftMargin = x_left_blink;
        leftParams.topMargin = mid_y - (indicatorPx/2);

        // 2. steering_wheel
        steeringWheelView = new ImageView(this);
        steeringWheelView.setImageResource(R.drawable.steering_wheel);
        FrameLayout.LayoutParams steeringParams = new FrameLayout.LayoutParams(steeringPx, steeringPx);
        steeringParams.leftMargin = x_steering;
        steeringParams.topMargin = mid_y - (steeringPx/2);

        // 3. right_blink_off (on 기능 포함)
        rightBlinkView = new ImageView(this);
        rightBlinkView.setImageResource(R.drawable.right_blink_off);
        FrameLayout.LayoutParams rightParams = new FrameLayout.LayoutParams(indicatorPx, indicatorPx);
        rightParams.leftMargin = x_right_blink;
        rightParams.topMargin = mid_y - (indicatorPx/2);

        // quad2Container에 추가
        quad2Container.addView(leftBlinkView, leftParams);
        quad2Container.addView(steeringWheelView, steeringParams);
        quad2Container.addView(rightBlinkView, rightParams);
        rootLayout.addView(quad2Container);

        // rightIndicatorView 클릭 시 선택 모드 실행 및 깜빡임 시작
        rightBlinkView.setOnClickListener(v -> {
            if (!isProcessing) {
                if (selectedMode == 1 || selectedMode == 2 || selectedMode == 4 || selectedMode == 3) {
                    isProcessing = true;
                    startIndicatorBlink(rightBlinkView);
                    steeringWheelView.setImageResource(R.drawable.rotate_right);
                    new Thread(() -> {
                        run(selectedMode);
                    }).start();
                } else {
                    Toast.makeText(this, "이 모드에서는 사용할 수 없습니다.", Toast.LENGTH_SHORT).show();
                }
            } else {
                Toast.makeText(this, "이미 실행 중입니다.", Toast.LENGTH_SHORT).show();
            }
        });

        steeringWheelView.setOnClickListener(v -> {
            if (!isProcessing) {
                if (selectedMode == 0) { // OFFLINE 모드일 때만 실행
                    isProcessing = true;
                    new Thread(() -> {
                        run(selectedMode);
                        runOnUiThread(() -> {
                            isProcessing = false;
                        });
                    }).start();
                } else {
                    Toast.makeText(this, "OFFLINE 모드에서만 사용할 수 있습니다.", Toast.LENGTH_SHORT).show();
                }
            }
        });

        setContentView(rootLayout);

        playMusic();

        View rootView = findViewById(android.R.id.content);
        rootView.setKeepScreenOn(true);

        createAppDirectories();
    }


    // 깜빡임 시작: rightIndicatorView 이미지 교체 반복
    private void startIndicatorBlink(ImageView indicatorView) {
        indicatorBlinking = true;
        indicatorBlinkRunnable = new Runnable() {
            private boolean isOn = false;
            @Override
            public void run() {
                if (!indicatorBlinking) {
                    indicatorView.setImageResource(R.drawable.right_blink_off);
                    return;
                }
                indicatorView.setImageResource(isOn ? R.drawable.right_blink_on : R.drawable.right_blink_off);
                isOn = !isOn;
                indicatorHandler.postDelayed(this, 500);
            }
        };
        indicatorHandler.post(indicatorBlinkRunnable);
    }

    private void stopIndicatorBlink(ImageView indicatorView) {
        indicatorBlinking = false;
        if (indicatorBlinkRunnable != null) {
            indicatorHandler.removeCallbacks(indicatorBlinkRunnable);
        }
        indicatorView.setImageResource(R.drawable.right_blink_off);
    }


    public void updateOverlayFrame(Bitmap bitmap) {
        runOnUiThread(() -> {
            Bitmap bmp = (bitmap.getWidth() != 416 || bitmap.getHeight() != 416)
                    ? Bitmap.createScaledBitmap(bitmap, 416*2, 416*2, true)
                    : bitmap;
            overlayView.setImageBitmap(bmp);
            overlayView.setVisibility(View.VISIBLE);
        });
    }

    private void playMusic() {
        File musicFile = new File(MusicPlay);
        if (!musicFile.exists()) {
            Toast.makeText(this, "Music file not found", Toast.LENGTH_SHORT).show();
            Log.e("MusicPlay", "File not found: " + MusicPlay);
            return;
        }

        Uri musicUri = Uri.fromFile(musicFile);

        // 화면 크기 가져오기
        int screenWidth = getResources().getDisplayMetrics().widthPixels;
        int screenHeight = getResources().getDisplayMetrics().heightPixels;

        // LayoutParams 설정: 가로는 화면 폭, 세로는 고정 높이, 위치는 아래쪽
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                screenWidth,
                screenHeight,
                Gravity.BOTTOM
        );

        musicView.setLayoutParams(params);

        musicView.setVideoURI(musicUri);

        musicView.setOnPreparedListener(mp -> {
            mp.setLooping(true);
            mp.start();
            Log.d("MusicPlay", "Music started");
        });

        musicView.setOnErrorListener((mp, what, extra) -> {
            Log.e("MusicPlay", "Error occurred: what=" + what + ", extra=" + extra);
            return true;
        });

        musicView.setVisibility(View.VISIBLE);
        musicView.requestFocus();
    }

    // run 함수는 기존 방식 유지, 실행 완료 후 깜빡임 및 isProcessing 해제는 호출자에서 처리함
    private void run(int mode) {
        String modelPath = getAssetFilePath(modelAssetPath);
        String inputPath = getAssetFilePath(inputAssetPath); // For offline

        if (modelPath == null || inputPath == null) {
            runOnUiThread(() -> Toast.makeText(this, "필요한 자산 파일 복사 실패", Toast.LENGTH_LONG).show());
            return;
        }

        switch (mode){
            case 0:
                Thread offlineThread = new Thread(() -> {
                    runOnUiThread(() -> Toast.makeText(this, "RunOfflineJNI 시작", Toast.LENGTH_SHORT).show());
                    int result = nativeLib.RunOfflineJNI(modelPath, inputPath);
                    runOnUiThread(() -> Toast.makeText(this, "RunOfflineJNI 완료: " + result, Toast.LENGTH_SHORT).show());
                });

                Thread schedulerThread = new Thread(() -> {
                    runOnUiThread(() -> Toast.makeText(this, "RunSchedulerJNI 시작", Toast.LENGTH_SHORT).show());
                    nativeLib.RunOfflineSchedulerJNI();
                    runOnUiThread(() -> Toast.makeText(this, "RunSchedulerJNI 완료", Toast.LENGTH_SHORT).show());
                });
                schedulerThread.start();
                offlineThread.start();

                try {
                    schedulerThread.join();
                    offlineThread.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                runOnUiThread(() -> {
                    isProcessing = false;
                });
                break;
            case 1:
                Thread scheduler1Thread = new Thread(() -> {
                    runOnUiThread(() -> Toast.makeText(this, "RunSchedulerJNI 시작", Toast.LENGTH_SHORT).show());
                    nativeLib.RunOnlineSchedulerOurJNI();
                    runOnUiThread(() -> Toast.makeText(this, "RunSchedulerJNI 완료", Toast.LENGTH_SHORT).show());
                });

                Thread online1Thread = new Thread(() -> {
                    runOnUiThread(() -> Toast.makeText(this, "RunOnlineOurJNI 시작", Toast.LENGTH_SHORT).show());
                    int result = nativeLib.RunOnlineOurJNI(modelPath, OnlineInputPath);
                    runOnUiThread(() -> Toast.makeText(this, "RunOnlineOurJNI 완료: " + result, Toast.LENGTH_SHORT).show());
                });
                scheduler1Thread.start();
                online1Thread.start();

                try {
                    scheduler1Thread.join();
                    online1Thread.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                runOnUiThread(() -> {
                    stopIndicatorBlink(rightBlinkView);
                    steeringWheelView.setImageResource(R.drawable.steering_wheel);
//                    overlayView.setVisibility(View.GONE);
                    new Handler(Looper.getMainLooper()).postDelayed(() -> {
                        overlayView.setVisibility(View.GONE);
                        isProcessing = false;
                    }, 3000);
                });
                break;
            case 2:
                Thread scheduler2Thread = new Thread(() -> {
                    runOnUiThread(() -> Toast.makeText(this, "RunSchedulerJNI 시작", Toast.LENGTH_SHORT).show());
                    nativeLib.RunOnlineSchedulerBaseCPUJNI();
                    runOnUiThread(() -> Toast.makeText(this, "RunSchedulerJNI 완료", Toast.LENGTH_SHORT).show());
                });

                Thread online2Thread = new Thread(() -> {
                    runOnUiThread(() -> Toast.makeText(this, "RunOnlineBaseCPUJNI 시작", Toast.LENGTH_SHORT).show());
                    int result = nativeLib.RunOnlineBaseJNI(modelPath, OnlineInputPath, "CPU");
                    runOnUiThread(() -> Toast.makeText(this, "RunOnlineBaseCPUJNI 완료: " + result, Toast.LENGTH_SHORT).show());
                });
                scheduler2Thread.start();
                online2Thread.start();


                try {
                    scheduler2Thread.join();
                    online2Thread.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                runOnUiThread(() -> {
                    stopIndicatorBlink(rightBlinkView);
                    steeringWheelView.setImageResource(R.drawable.steering_wheel);
//                    overlayView.setVisibility(View.GONE);
                    new Handler(Looper.getMainLooper()).postDelayed(() -> {
                        overlayView.setVisibility(View.GONE);
                        isProcessing = false;
                    }, 3000);
                });
                break;
            case 3:
                Thread scheduler3Thread = new Thread(() -> {
                    runOnUiThread(() -> Toast.makeText(this, "RunSchedulerJNI 시작", Toast.LENGTH_SHORT).show());
                    nativeLib.RunOnlineSchedulerBaseXNNJNI();
                    runOnUiThread(() -> Toast.makeText(this, "RunSchedulerJNI 완료", Toast.LENGTH_SHORT).show());
                });

                Thread online3Thread = new Thread(() -> {
                    runOnUiThread(() -> Toast.makeText(this, "RunOnlineBaseXNNJNI 시작", Toast.LENGTH_SHORT).show());
                    int result = nativeLib.RunOnlineBaseJNI(modelPath, OnlineInputPath, "XNN");
                    runOnUiThread(() -> Toast.makeText(this, "RunOnlineBaseXNNJNI 완료: " + result, Toast.LENGTH_SHORT).show());
                });
                scheduler3Thread.start();
                online3Thread.start();

                try {
                    scheduler3Thread.join();
                    online3Thread.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                runOnUiThread(() -> {
                    stopIndicatorBlink(rightBlinkView);
                    steeringWheelView.setImageResource(R.drawable.steering_wheel);
//                    overlayView.setVisibility(View.GONE);
                    new Handler(Looper.getMainLooper()).postDelayed(() -> {
                        overlayView.setVisibility(View.GONE);
                        isProcessing = false;
                    }, 3000);
                });
                break;
            case 4:
                Thread scheduler4Thread = new Thread(() -> {
                    runOnUiThread(() -> Toast.makeText(this, "RunSchedulerJNI 시작", Toast.LENGTH_SHORT).show());
                    nativeLib.RunOnlineSchedulerBaseGPUJNI();
                    runOnUiThread(() -> Toast.makeText(this, "RunSchedulerJNI 완료", Toast.LENGTH_SHORT).show());
                });

                Thread online4Thread = new Thread(() -> {
                    runOnUiThread(() -> Toast.makeText(this, "RunOnlineBaseGPUJNI 시작", Toast.LENGTH_SHORT).show());
                    int result = nativeLib.RunOnlineBaseJNI(modelPath, OnlineInputPath, "GPU");
                    runOnUiThread(() -> Toast.makeText(this, "RunOnlineBaseGPUJNI 완료: " + result, Toast.LENGTH_SHORT).show());
                });
                scheduler4Thread.start();
                online4Thread.start();

                try {
                    scheduler4Thread.join();
                    online4Thread.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                runOnUiThread(() -> {
                    stopIndicatorBlink(rightBlinkView);
                    steeringWheelView.setImageResource(R.drawable.steering_wheel);
//                    overlayView.setVisibility(View.GONE);
                    new Handler(Looper.getMainLooper()).postDelayed(() -> {
                        overlayView.setVisibility(View.GONE);
                        isProcessing = false;
                    }, 3000);
                });
                break;
            default:
                runOnUiThread(() -> Toast.makeText(this, "Press Button [OFFLINE, ONLINE(OURS, BASE CPU/XNN/GPU)]", Toast.LENGTH_SHORT).show());
                break;
        }
    }

    private String getAssetFilePath(String assetName) {
        File file = new File(getCacheDir(), assetName);
        if (!file.exists()) {
            try (InputStream is = getAssets().open(assetName);
                 FileOutputStream fos = new FileOutputStream(file)) {
                byte[] buffer = new byte[1024];
                int read;
                while ((read = is.read(buffer)) != -1) {
                    fos.write(buffer, 0, read);
                }
                fos.flush();
            } catch(IOException e) {
                Log.e("MainActivity", "Asset 파일 복사 중 오류", e);
            }
        }
        return file.getAbsolutePath();
    }

    private void createAppDirectories() {
        File baseDir = new File("/data/data/com.example.myapp/");
        if (!baseDir.exists()) {
            if(!baseDir.mkdirs()) {
                Log.e("MainActivity", "디렉토리 생성 실패: " + baseDir.getAbsolutePath());
                runOnUiThread(() -> Toast.makeText(this, "베이스 디렉토리 생성 실패", Toast.LENGTH_SHORT).show());
            }
        }
        createDummyYamlFile();

        File modelWholeDir = new File(baseDir, "model_whole");
        File segmentsDir = new File(baseDir, "segments");
        File profiledDir = new File(baseDir, "profiled");
        File presetDir = new File(baseDir, "preset");
        File sock = new File(baseDir, "sock");

        if (!modelWholeDir.exists()) {
            if(!modelWholeDir.mkdirs()){
                Log.e("MainActivity", "model whole 디렉토리 생성 실패: " + modelWholeDir.getAbsolutePath());
                runOnUiThread(() -> Toast.makeText(this, "베이스 디렉토리 생성 실패", Toast.LENGTH_SHORT).show());
            }
        }
        if (!segmentsDir.exists()) {
            if(!segmentsDir.mkdirs()){
                Log.e("MainActivity", "디렉토리 생성 실패: " + segmentsDir.getAbsolutePath());
                runOnUiThread(() -> Toast.makeText(this, "segmentsDir 디렉토리 생성 실패", Toast.LENGTH_SHORT).show());

            }
        }
        if (!profiledDir.exists()) {
            if(!profiledDir.mkdirs()){
                Log.e("MainActivity", "디렉토리 생성 실패: " + profiledDir.getAbsolutePath());
                runOnUiThread(() -> Toast.makeText(this, "profiledDir 디렉토리 생성 실패", Toast.LENGTH_SHORT).show());
            }
        }
        if (!presetDir.exists()) {
            if (!presetDir.mkdirs()) {
                Log.e("MainActivity", "디렉토리 생성 실패: " + presetDir.getAbsolutePath());
                runOnUiThread(() -> Toast.makeText(this, "presetDir 디렉토리 생성 실패", Toast.LENGTH_SHORT).show());
            }
        }
        if (!sock.exists()) {
            if (!sock.mkdirs()) {
                Log.e("MainActivity", "디렉토리 생성 실패: " + sock.getAbsolutePath());
                runOnUiThread(() -> Toast.makeText(this, "sock 디렉토리 생성 실패", Toast.LENGTH_SHORT).show());
            }
        }
    }

    private void createDummyYamlFile() {
        String yamlContent = "level:\n" +
                "  - partitions:\n" +
                "      - op:\n" +
                "          - [0]\n" +
                "        resource: [\"C\"]\n" +
                "        type: [\"N\"]\n" +
                "        activated: [\"O\"]\n";

        File yamlFile = new File("/data/data/com.example.myapp/dummy.yaml");

        if (!yamlFile.exists()) {
            try (FileOutputStream fos = new FileOutputStream(yamlFile)) {
                fos.write(yamlContent.getBytes());
                fos.flush();
                Log.i("MainActivity", "dummy.yaml 파일 생성 완료: " + yamlFile.getAbsolutePath());
            } catch (IOException e) {
                Log.e("MainActivity", "dummy.yaml 파일 생성 오류", e);
            }
        } else {
            Log.i("MainActivity", "dummy.yaml 파일 이미 존재함: " + yamlFile.getAbsolutePath());
        }
    }
}
