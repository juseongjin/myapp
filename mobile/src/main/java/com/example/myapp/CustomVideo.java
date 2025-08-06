package com.example.myapp;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.VideoView;

public class CustomVideo extends VideoView {

    private int videoWidth;
    private int videoHeight;

    public CustomVideo(Context context) {
        super(context);
    }

    public void setVideoSize(int width, int height) {
        videoWidth = width;
        videoHeight = height;
        requestLayout();
        invalidate();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int parentWidth = MeasureSpec.getSize(widthMeasureSpec);
        int parentHeight = MeasureSpec.getSize(heightMeasureSpec);

        if (videoWidth == 0 || videoHeight == 0) {
            setMeasuredDimension(parentWidth, parentHeight);
            return;
        }

        float videoAspect = (float) videoWidth / videoHeight;

        // 화면 비율
        float parentAspect = (float) parentWidth / parentHeight;

        int measuredWidth, measuredHeight;

        if (videoWidth > 0 && videoHeight > 0) {
            // 영상이 부모보다 더 넓다 → 너비 기준, 높이 줄이기
            setMeasuredDimension(videoWidth, videoHeight);
        } else {
            // 영상이 부모보다 덜 넓다 → 높이 기준, 너비 줄이기
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }
    }
}