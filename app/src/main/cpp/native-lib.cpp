#include <android/log.h>
#include <jni.h>
#include <opencv2/opencv.hpp>

#define TAG "NativeLib"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_realtime_1edge_1viewer_MainActivity_stringFromJNI(
    JNIEnv *env, jobject /* this */) {
  std::string hello = "Hello from C++";
  return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_realtime_1edge_1viewer_MainActivity_processFrame(
    JNIEnv *env, jobject /* this */, jint width, jint height, jobject buffer) {

  uint8_t *srcLumaPtr = (uint8_t *)env->GetDirectBufferAddress(buffer);
  if (srcLumaPtr == nullptr) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "Bitmap buffer is null");
    return;
  }

  // Create a Mat from the grayscale buffer (Y plane)
  cv::Mat grayMat(height, width, CV_8UC1, srcLumaPtr);

  // Perform Canny Edge Detection
  cv::Mat edges;
  cv::Canny(grayMat, edges, 50, 150);

  // For now, just log that we processed it.
  // In the next phase (OpenGL), we will render 'edges' to a texture.
  // __android_log_print(ANDROID_LOG_DEBUG, TAG, "Processed frame: %dx%d, Edges
  // detected", width, height);
}
