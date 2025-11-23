# Real-Time Edge Viewer

A high-performance Android application that captures camera frames, performs Canny Edge Detection using OpenCV (Native C++), and renders the result to the screen using OpenGL ES 2.0. It also includes a modern TypeScript-based web viewer interface.

## Features

-   **CameraX Integration**: Efficient camera frame capture.
-   **Native C++ Processing**: High-performance image processing using OpenCV 4.12.0.
-   **Canny Edge Detection**: Real-time edge detection algorithm.
-   **OpenGL ES 2.0 Rendering**: Low-latency rendering of processed frames via `GLSurfaceView`.
-   **Static Linking**: Self-contained native library (no external dependencies required at runtime).
-   **Web Viewer UI**: A premium, Cyberpunk-themed web interface (TypeScript/Vite) for viewing the feed (simulation).

## Prerequisites

-   **Android Studio** (Koala or later recommended)
-   **Android NDK** (Version 27.0.12077973)
-   **OpenCV Android SDK** (Version 4.12.0)
-   **Node.js & npm** (for Web Viewer)

## Setup & Build

### Android App

1.  **OpenCV SDK**: Ensure the OpenCV Android SDK is extracted to `/home/{user_device_name}/Android/OpenCV-android-sdk`.
2.  **Open Project**: Open the `RealTime-Edge-Viewer` folder in Android Studio.
3.  **Sync Gradle**: Allow Gradle to sync and configure the project.
5.  **Run**: Connect an Android device and click **Run 'app'** this will automatically build the project and install it on your device after asking the users permission.

### Web Viewer

1.  Navigate to the web viewer directory:
    ```bash
    cd web-viewer
    ```
2.  Install dependencies:
    ```bash
    npm install
    ```
3.  Start the development server:
    ```bash
    npm run dev
    ```
4.  Open the link shown in the terminal (e.g.,`http://localhost:5173`) to view the UI.

## Architecture

-   **MainActivity.kt**: Handles permissions, CameraX setup, and `GLSurfaceView` initialization.
-   **native-lib.cpp**: Contains the JNI bridge, OpenCV processing logic, and OpenGL rendering code.
-   **CMakeLists.txt**: Configures the native build, linking OpenCV statically.
-   **web-viewer/**: Contains the Vite+TypeScript web application.

## Troubleshooting

-   **UnsatisfiedLinkError**: If the app crashes on startup, ensure you have rebuilt the project with **NDK 27** and **Static Linking** enabled (default configuration).
-   **Camera Permission**: Grant camera permissions when prompted.
