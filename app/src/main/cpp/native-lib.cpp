#include <jni.h>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/video.hpp>
#include "android/bitmap.h"


extern "C" JNIEXPORT jstring JNICALL
Java_com_mi_proyectocamara_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Trabajo Interciclo con Kotlin y C++";
    return env->NewStringUTF(hello.c_str());
}

void bitmapToMat(JNIEnv * env, jobject bitmap, cv::Mat &dst, jboolean needUnPremultiplyAlpha){
    AndroidBitmapInfo info;
    void* pixels = 0;
    try {
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        dst.create(info.height, info.width, CV_8UC4);
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
        {
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(needUnPremultiplyAlpha) cvtColor(tmp, dst, cv::COLOR_mRGBA2RGBA);
            else tmp.copyTo(dst);
        } else {
// info.format == ANDROID_BITMAP_FORMAT_RGB_565
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            cvtColor(tmp, dst, cv::COLOR_BGR5652RGBA);
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
//jclass je = env->FindClass("org/opencv/core/CvException");
        jclass je = env->FindClass("java/lang/Exception");
//if(!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nBitmapToMat}");
        return;
    }
}

void matToBitmap(JNIEnv * env, cv::Mat src, jobject bitmap, jboolean needPremultiplyAlpha) {
    AndroidBitmapInfo info;
    void* pixels = 0;
    try {
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 || info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( src.dims == 2 && info.height == (uint32_t)src.rows && info.width == (uint32_t)src.cols );
        CV_Assert( src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
        {
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(src.type() == CV_8UC1)
            {
                cvtColor(src, tmp, cv::COLOR_GRAY2RGBA);
            } else if(src.type() == CV_8UC3){cvtColor(src, tmp, cv::COLOR_RGB2RGBA);
            } else if(src.type() == CV_8UC4){
                if(needPremultiplyAlpha) cvtColor(src, tmp, cv::COLOR_RGBA2mRGBA);
                else src.copyTo(tmp);
            }
        } else {
// info.format == ANDROID_BITMAP_FORMAT_RGB_565
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            if(src.type() == CV_8UC1)
            {
                cvtColor(src, tmp, cv::COLOR_GRAY2BGR565);
            } else if(src.type() == CV_8UC3){
                cvtColor(src, tmp, cv::COLOR_RGB2BGR565);
            } else if(src.type() == CV_8UC4){
                cvtColor(src, tmp, cv::COLOR_RGBA2BGR565);
            }
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
//jclass je = env->FindClass("org/opencv/core/CvException");
        jclass je = env->FindClass("java/lang/Exception");
//if(!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nMatToBitmap}");
        return;
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_mi_proyectocamara_MainActivity_detectorBordes(JNIEnv* env,jobject /*this*/,jobject bitmapIn, jobject bitmapOut){
    cv::Mat frame;
    bitmapToMat(env, bitmapIn, frame, false);  // bitmapToMat es una función personalizada

    cv::flip(frame, frame, 1);
    cv::Mat elemento = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

    cv::Rect roi(0,0, frame.cols/2, frame.rows/2);

    cv::Rect rei(frame.cols/2, 0, frame.cols/2, frame.rows/2);
    cv::Rect tercero(0, frame.rows/2, frame.cols/2, frame.rows/2);
    cv::Rect cuarto(frame.cols/2, frame.rows/2, frame.cols/2, frame.rows/2);


    cv::Mat frame_roi = frame(roi);
    cv::Mat frame_rei = frame(rei);

    morphologyEx(frame_roi, frame_roi, cv::MORPH_DILATE, elemento, cv::Point(-1,-1),3);


    morphologyEx(frame_rei, frame_rei, cv::MORPH_ERODE, elemento, cv::Point(-1,-1),3);

    cv::Mat dilat = frame(tercero);
    cv::Mat dilatC = frame(cuarto);

//    cvtColor(dilat, dilat, cv::COLOR_BGR2GRAY);
//    cvtColor(dilat, dilat, cv::COLOR_GRAY2BGR);


//    cvtColor(dilatC, dilatC, cv::COLOR_BGR2GRAY);
//    cvtColor(dilatC, dilatC, cv::COLOR_GRAY2BGR);


    ////////

    cv::Mat erosis ;
    cv::Mat erosisss ;
    morphologyEx(dilat, erosisss, cv::MORPH_ERODE, elemento, cv::Point(-1,-1),3);
    morphologyEx(dilat, erosis, cv::MORPH_DILATE, elemento, cv::Point(-1,-1),3);


    cv::Mat cu;
    cv::Mat cc;


    morphologyEx(dilatC, cu, cv::MORPH_ERODE, elemento, cv::Point(-1,-1),3);
    morphologyEx(dilatC, cc, cv::MORPH_DILATE, elemento, cv::Point(-1,-1),3);



    cv::Mat dife;
    absdiff(erosis, erosisss, dife);

    cv::Mat diferencia;
    cv::Mat negado;
    absdiff(cc,cu,diferencia);


    bitwise_not(diferencia, negado);



        dilat.copyTo(frame(tercero));


        dife.copyTo(frame(tercero));

        dilatC.copyTo(frame(cuarto));

        negado.copyTo(frame(cuarto));



    matToBitmap(env, frame, bitmapOut, false);
}