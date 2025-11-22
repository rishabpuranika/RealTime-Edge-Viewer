#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_realtime_1edge_1viewer_MainActivity_stringFromJNI(
    JNIEnv *env, jobject /* this */) {
  std::string hello = "Hello from C++";
  return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_realtime_1edge_1viewer_MainActivity_processFrame(
    JNIEnv *env, jobject /* this */, jint width, jint height, jobject buffer) {

  // TODO: Process frame with OpenCV
  // uint8_t* data = (uint8_t*)env->GetDirectBufferAddress(buffer);
}
