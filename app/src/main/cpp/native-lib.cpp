#include <jni.h>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/video.hpp>
#include "android/bitmap.h"


cv::Mat imgGx;
cv::Mat imgGy;
cv::Mat imgGxAbs;
cv::Mat imgGyAbs;
cv::Mat foregroundMask,pixelatedPerson,filteredFrame;
cv::Ptr<cv::BackgroundSubtractor> bgSubtractor = cv::createBackgroundSubtractorMOG2();

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
/////////////////////////////////////////////////////////////////////////////////////////////////////////

void pixelate3D(const cv::Mat& input, cv::Mat& output, int pixelSize) {
    int rows = input.rows;
    int cols = input.cols;
    input.copyTo(output);  // Crear copia del frame original

    for (int i = 0; i < rows; i += pixelSize) {
        for (int j = 0; j < cols; j += pixelSize) {
            cv::Rect region(j, i, pixelSize, pixelSize);  // Definir área del bloque

            if (i + pixelSize > rows) region.height = rows - i;
            if (j + pixelSize > cols) region.width = cols - j;

            cv::Scalar blockColor = mean(input(region));  // Color promedio del bloque

            // Rellenar el bloque con el color base
            rectangle(output, region, blockColor, cv::FILLED);

            // Dibujar el contorno claro (luz desde arriba a la izquierda)
            line(output, cv::Point(j, i), cv::Point(j + region.width, i), cv::Scalar(255, 255, 255), 1);  // Línea superior
            //line(output, Point(j, i), Point(j, i + region.height), Scalar(255, 255, 255), 1);  // Línea izquierda

            // Dibujar sombra en la parte inferior y derecha
            line(output, cv::Point(j, i + region.height - 1),
                 cv::Point(j + region.width, i + region.height - 1), cv::Scalar(50, 50, 50), 1);  // Línea inferior
            line(output, cv::Point(j + region.width - 1, i),
                 cv::Point(j + region.width - 1, i + region.height), cv::Scalar(50, 50, 50), 1);  // Línea derecha
        }
    }
}

void filtroBackgorund(cv::Mat& frame, cv::Mat& mask){
    // Espejar la imagen horizontalmente
    //flip(frame, frame, 1);

    // Aplicar el filtro pixelado
    cv::Mat filtro;

    //cvtColor(frame,filtro,COLOR_BGR2GRAY);

    // Aplicar la sustracción de fondo
    bgSubtractor->apply(frame, foregroundMask);
    //morphologyEx(foregroundMask,foregroundMask, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(5,5)));
    //dilate(foregroundMask,foregroundMask,getStructuringElement(MORPH_ELLIPSE, Size(5,5)));
    GaussianBlur(foregroundMask, foregroundMask, cv::Size(5,5), 1.72, 1.73);
    Sobel(foregroundMask,imgGx,CV_16S,1,0,3);
    Sobel(foregroundMask,imgGy,CV_16S,0,1,3);

    convertScaleAbs(imgGx,imgGxAbs);
    convertScaleAbs(imgGy,imgGyAbs);

    addWeighted(imgGxAbs, 0.5, imgGyAbs, 0.5, 0,foregroundMask);

    std::vector<std::vector<cv::Point>> contours;
    findContours(foregroundMask,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
    mask = cv::Mat::zeros(frame.size(), CV_8UC1);
    drawContours(mask, contours, -1, cv::Scalar(255), cv::FILLED);
    frame.copyTo(pixelatedPerson, mask);
    pixelate3D(pixelatedPerson,filteredFrame,15);
    //cv::Mat result;
    //frame.copyTo(result);  // Copiar la imagen original
    //filteredFrame.copyTo(result, mask);
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


    filtroBackgorund(frame_rei, frame_rei);

    if(!frame.empty()){
        cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
    }
    //morphologyEx(frame_rei, frame_rei, cv::MORPH_ERODE, elemento, cv::Point(-1,-1),3);

    frame_rei.copyTo(frame(rei));
    

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