package com.example.realtime_edge_viewer

import android.Manifest
import android.content.pm.PackageManager
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MainActivity : AppCompatActivity() {

    private lateinit var cameraExecutor: ExecutorService
    private lateinit var glSurfaceView: GLSurfaceView
    private lateinit var fpsText: android.widget.TextView
    private lateinit var renderer: EdgeRenderer

    private val activityResultLauncher =
        registerForActivityResult(
            ActivityResultContracts.RequestPermission()
        ) { isGranted ->
            if (isGranted) {
                startCamera()
            } else {
                Toast.makeText(this, "Camera permission denied", Toast.LENGTH_SHORT).show()
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        glSurfaceView = findViewById(R.id.viewFinder)
        fpsText = findViewById(R.id.fps_text)
        glSurfaceView.setEGLContextClientVersion(2)
        renderer = EdgeRenderer()
        glSurfaceView.setRenderer(renderer)
        glSurfaceView.renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY

        cameraExecutor = Executors.newSingleThreadExecutor()

        if (ContextCompat.checkSelfPermission(
                this,
                Manifest.permission.CAMERA
            ) == PackageManager.PERMISSION_GRANTED
        ) {
            startCamera()
        } else {
            activityResultLauncher.launch(Manifest.permission.CAMERA)
        }
        RemoteControlServer().start()
        MJPEGServer().start()
        showIpAddress()
    }

    private fun showIpAddress() {
        val wifiManager = applicationContext.getSystemService(android.content.Context.WIFI_SERVICE) as android.net.wifi.WifiManager
        val ipAddress = android.text.format.Formatter.formatIpAddress(wifiManager.connectionInfo.ipAddress)
        Toast.makeText(this, "Control available at ws://$ipAddress:8081", Toast.LENGTH_LONG).show()
        Log.d(TAG, "Control available at ws://$ipAddress:8081")
    }
    private fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)

        cameraProviderFuture.addListener({
            val cameraProvider: ProcessCameraProvider = cameraProviderFuture.get()

            // We no longer need Preview use case since we render manually
            // val preview = Preview.Builder().build() ...

            val imageAnalyzer = ImageAnalysis.Builder()
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                .build()
                .also {
                    it.setAnalyzer(cameraExecutor, FrameAnalyzer())
                }

            val cameraSelector = CameraSelector.DEFAULT_BACK_CAMERA

            try {
                cameraProvider.unbindAll()
                // Bind only image analysis
                cameraProvider.bindToLifecycle(
                    this, cameraSelector, imageAnalyzer
                )
            } catch (exc: Exception) {
                Log.e(TAG, "Use case binding failed", exc)
            }

        }, ContextCompat.getMainExecutor(this))
    }

    private inner class FrameAnalyzer : ImageAnalysis.Analyzer {
        override fun analyze(image: ImageProxy) {
            val buffer = image.planes[0].buffer
            processFrame(image.width, image.height, buffer)
            image.close()
        }
    }

    private inner class EdgeRenderer : GLSurfaceView.Renderer {
        private var startTime = System.nanoTime()
        private var frameCount = 0

        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            nativeInitGL()
        }

        override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
            nativeResizeGL(width, height)
        }

        override fun onDrawFrame(gl: GL10?) {
            nativeRenderGL()
            
            frameCount++
            val currentTime = System.nanoTime()
            if (currentTime - startTime >= 1_000_000_000) {
                val fps = frameCount
                frameCount = 0
                startTime = currentTime
                
                runOnUiThread {
                    fpsText.text = "FPS: $fps"
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraExecutor.shutdown()
    }

    companion object {
        private const val TAG = "EdgeViewer"
        init {
            System.loadLibrary("edgeviewer")
        }
    }

    external fun processFrame(width: Int, height: Int, buffer: java.nio.ByteBuffer)
    external fun nativeInitGL()
    external fun nativeResizeGL(width: Int, height: Int)
    external fun nativeRenderGL()
    external fun setThresholds(low: Int, high: Int)
    external fun getJPEG(): ByteArray?

    private fun saveImageToGallery() {
        val jpeg = getJPEG() ?: return
        val filename = "EdgeViewer_${System.currentTimeMillis()}.jpg"
        val contentValues = android.content.ContentValues().apply {
            put(android.provider.MediaStore.Images.Media.DISPLAY_NAME, filename)
            put(android.provider.MediaStore.Images.Media.MIME_TYPE, "image/jpeg")
            put(android.provider.MediaStore.Images.Media.RELATIVE_PATH, android.os.Environment.DIRECTORY_PICTURES)
        }

        val resolver = applicationContext.contentResolver
        val uri = resolver.insert(android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI, contentValues)
        
        uri?.let {
            resolver.openOutputStream(it)?.use { stream ->
                stream.write(jpeg)
            }
            runOnUiThread {
                Toast.makeText(this, "Snapshot saved to Gallery", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private inner class MJPEGServer : Thread() {
        private val serverSocket = java.net.ServerSocket(8080)

        override fun run() {
            while (!isInterrupted) {
                try {
                    val socket = serverSocket.accept()
                    Thread { handleConnection(socket) }.start()
                } catch (e: Exception) {
                    Log.e(TAG, "Server error", e)
                }
            }
        }

        private fun handleConnection(socket: java.net.Socket) {
            try {
                val output = socket.getOutputStream()
                output.write(("HTTP/1.1 200 OK\r\n" +
                        "Content-Type: multipart/x-mixed-replace; boundary=mjpegstream\r\n" +
                        "\r\n").toByteArray())

                while (!socket.isClosed) {
                    val jpeg = getJPEG()
                    if (jpeg != null) {
                        output.write(("--mjpegstream\r\n" +
                                "Content-Type: image/jpeg\r\n" +
                                "Content-Length: ${jpeg.size}\r\n" +
                                "\r\n").toByteArray())
                        output.write(jpeg)
                        output.write("\r\n".toByteArray())
                        output.flush()
                    }
                    Thread.sleep(33) // ~30 FPS
                }
            } catch (e: Exception) {
                Log.e(TAG, "Connection error", e)
            } finally {
                socket.close()
            }
        }
    }

    private inner class RemoteControlServer : org.java_websocket.server.WebSocketServer(java.net.InetSocketAddress(8081)) {
        override fun onOpen(conn: org.java_websocket.WebSocket?, handshake: org.java_websocket.handshake.ClientHandshake?) {
            Log.d(TAG, "WebSocket connected: " + conn?.remoteSocketAddress)
        }

        override fun onClose(conn: org.java_websocket.WebSocket?, code: Int, reason: String?, remote: Boolean) {
            Log.d(TAG, "WebSocket closed")
        }

        override fun onMessage(conn: org.java_websocket.WebSocket?, message: String?) {
            Log.d(TAG, "Message received: $message")
            if (message != null) {
                try {
                    val json = org.json.JSONObject(message)
                    val type = json.getString("type")
                    
                    if (type == "threshold") {
                        val low = json.getInt("low")
                        val high = json.getInt("high")
                        setThresholds(low, high)
                    } else if (type == "snapshot") {
                        saveImageToGallery()
                    }
                } catch (e: Exception) {
                    Log.e(TAG, "JSON parse error", e)
                }
            }
        }

        override fun onError(conn: org.java_websocket.WebSocket?, ex: Exception?) {
            Log.e(TAG, "WebSocket error", ex)
        }

        override fun onStart() {
            Log.d(TAG, "WebSocket server started on port 8081")
        }
    }
}