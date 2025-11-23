#include <GLES2/gl2.h>
#include <android/log.h>
#include <atomic>
#include <jni.h>
#include <mutex>
#include <opencv2/opencv.hpp>

#define TAG "NativeLib"

// Global variables for OpenGL and synchronization
GLuint gProgram;
GLuint gTextureId;
GLint gPositionHandle;
GLint gTexCoordHandle;
GLint gSamplerHandle;

std::mutex gFrameMutex;
cv::Mat gSharedFrame;
bool gFrameReady = false;

// Vertex Shader
const char *vertexShaderCode = R"(
    attribute vec4 vPosition;
    attribute vec2 vTexCoord;
    varying vec2 fTexCoord;
    void main() {
        gl_Position = vPosition;
        fTexCoord = vTexCoord;
    }
)";

// Fragment Shader
const char *fragmentShaderCode = R"(
    precision mediump float;
    varying vec2 fTexCoord;
    uniform sampler2D sTexture;
    void main() {
        float gray = texture2D(sTexture, fTexCoord).r;
        gl_FragColor = vec4(gray, gray, gray, 1.0);
    }
)";

// Full-screen quad vertices
const GLfloat vertices[] = {
    -1.0f, -1.0f, 0.0f, // Bottom-left
    1.0f,  -1.0f, 0.0f, // Bottom-right
    -1.0f, 1.0f,  0.0f, // Top-left
    1.0f,  1.0f,  0.0f  // Top-right
};

// Texture coordinates (flipped vertically for Android camera)
// Note: CameraX usually gives rotated frames, so we might need to adjust
// rotation here or in Java. For now, let's assume standard orientation and
// adjust if needed.
const GLfloat texCoords[] = {
    1.0f, 1.0f, // Bottom-left (was 0,1) -> 1,1
    1.0f, 0.0f, // Bottom-right (was 1,1) -> 1,0
    0.0f, 1.0f, // Top-left (was 0,0) -> 0,1
    0.0f, 0.0f  // Top-right (was 1,0) -> 0,0
};

GLuint loadShader(GLenum type, const char *shaderCode) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &shaderCode, NULL);
  glCompileShader(shader);
  return shader;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_realtime_1edge_1viewer_MainActivity_nativeInitGL(
    JNIEnv *env, jobject thiz) {
  GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderCode);
  GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderCode);

  gProgram = glCreateProgram();
  glAttachShader(gProgram, vertexShader);
  glAttachShader(gProgram, fragmentShader);
  glLinkProgram(gProgram);

  gPositionHandle = glGetAttribLocation(gProgram, "vPosition");
  gTexCoordHandle = glGetAttribLocation(gProgram, "vTexCoord");
  gSamplerHandle = glGetUniformLocation(gProgram, "sTexture");

  glGenTextures(1, &gTextureId);
  glBindTexture(GL_TEXTURE_2D, gTextureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_realtime_1edge_1viewer_MainActivity_nativeResizeGL(
    JNIEnv *env, jobject thiz, jint width, jint height) {
  glViewport(0, 0, width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_realtime_1edge_1viewer_MainActivity_nativeRenderGL(
    JNIEnv *env, jobject thiz) {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(gProgram);

  // Upload texture if new frame is ready
  {
    std::lock_guard<std::mutex> lock(gFrameMutex);
    if (gFrameReady && !gSharedFrame.empty()) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, gTextureId);

      // Note: GL_LUMINANCE is deprecated in GLES 3.0 but valid in 2.0.
      // For GLES 3.0+, use GL_RED.
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, gSharedFrame.cols,
                   gSharedFrame.rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                   gSharedFrame.data);
      gFrameReady = false;
    }
  }

  // Draw Quad
  glVertexAttribPointer(gPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, vertices);
  glEnableVertexAttribArray(gPositionHandle);

  glVertexAttribPointer(gTexCoordHandle, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
  glEnableVertexAttribArray(gTexCoordHandle);

  glUniform1i(gSamplerHandle, 0);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableVertexAttribArray(gPositionHandle);
  glDisableVertexAttribArray(gTexCoordHandle);
}

// Global thresholds with default values
std::atomic<int> gLowThreshold(50);
std::atomic<int> gHighThreshold(150);

extern "C" JNIEXPORT void JNICALL
Java_com_example_realtime_1edge_1viewer_MainActivity_setThresholds(JNIEnv *env,
                                                                   jobject thiz,
                                                                   jint low,
                                                                   jint high) {
  gLowThreshold = low;
  gHighThreshold = high;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_realtime_1edge_1viewer_MainActivity_processFrame(
    JNIEnv *env, jobject /* this */, jint width, jint height, jobject buffer) {

  uint8_t *srcLumaPtr = (uint8_t *)env->GetDirectBufferAddress(buffer);
  if (srcLumaPtr == nullptr)
    return;

  cv::Mat grayMat(height, width, CV_8UC1, srcLumaPtr);
  cv::Mat edges;
  // Use atomic thresholds
  cv::Canny(grayMat, edges, gLowThreshold.load(), gHighThreshold.load());

  // Copy to shared frame for rendering
  {
    std::lock_guard<std::mutex> lock(gFrameMutex);
    // Ensure gSharedFrame is allocated
    if (gSharedFrame.empty() || gSharedFrame.size() != edges.size()) {
      gSharedFrame = cv::Mat(edges.size(), CV_8UC1);
    }
    edges.copyTo(gSharedFrame);
    gFrameReady = true;
  }
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_realtime_1edge_1viewer_MainActivity_getJPEG(JNIEnv *env,
                                                             jobject thiz) {
  std::vector<uchar> buf;
  {
    std::lock_guard<std::mutex> lock(gFrameMutex);
    if (gSharedFrame.empty()) {
      return nullptr;
    }
    // Rotate frame for streaming (90 degrees clockwise)
    cv::Mat rotated;
    cv::rotate(gSharedFrame, rotated, cv::ROTATE_90_CLOCKWISE);

    // Encode to JPEG
    cv::imencode(".jpg", rotated, buf);
  }

  jbyteArray ret = env->NewByteArray(buf.size());
  env->SetByteArrayRegion(ret, 0, buf.size(), (jbyte *)buf.data());
  return ret;
}
