# Android IVI App for YOLO v4 Tiny

- **Model**: `yolo_v4_tiny.tflite`
- **Input**: 약 300 프레임 이미지 (실제 영상에서 추출)


Model = "yolo_v4_tiny.tflite
Input = "About 300 frame images extracted from real-world video"

1. Make directory for project (name: "myapp", recommened)
```
cd myapp
```
2. Clone this repo in myapp directory
```
git clone git@github.com:juseongjin/myapp.git .
```
3. Configure requirements (i.e., library)
```
./shell.sh
```
4. Build android project
```
./gradlew :mobile:assembleDebug
```
5. The build output
```
cd /path/to/myapp/mobile/build/outputs/apk/debug/
```
6. App install in target android device
```
adb install -r mobile-debug.apk
```
