package com.mi.proyectocamara

import android.content.Intent
import android.graphics.Bitmap
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Handler
import android.provider.ContactsContract.CommonDataKinds.Website.URL
import android.view.PixelCopy
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import android.widget.Toast
import androidx.core.view.drawToBitmap
import androidx.media3.common.MediaItem
import androidx.media3.common.PlaybackException
import androidx.media3.common.Player
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.ui.PlayerView
import com.longdo.mjpegviewer.MjpegView
import com.mi.proyectocamara.databinding.ActivityMainBinding
import java.net.HttpURLConnection
import java.net.URL
import kotlin.concurrent.thread


class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    private var mjpeg: MjpegView? = null

    private lateinit var boton: Button

    private lateinit var handler: Handler
    private val captureInterval: Long = 100

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Example of a call to a native method
        binding.sampleText.text = stringFromJNI()

        mjpeg = findViewById(R.id.video)

        boton = findViewById(R.id.prender)

        boton.setOnClickListener{
            extraer()
        }
        checkUrlConnection("http://192.168.10.28:81/stream");


    }



    fun checkUrlConnection(urlString: String) {
        thread {
            try {
                val url = URL(urlString)
                val connection = url.openConnection() as HttpURLConnection
                connection.requestMethod = "GET"
                connection.connect()

                val responseCode = connection.responseCode
                if (responseCode == HttpURLConnection.HTTP_OK) {
                    println("Conexión exitosa: Código $responseCode")
                } else {
                    println("Error de conexión: Código $responseCode")
                }

                connection.disconnect()

            } catch (e: Exception) {
                e.printStackTrace()
                println("Error: ${e.message}")
            }
        }
    }



    private fun extraer(){
        mjpeg!!.mode = MjpegView.MODE_FIT_WIDTH
        mjpeg!!.isAdjustHeight = true
        mjpeg!!.supportPinchZoomAndPan = true
        mjpeg!!.setUrl("http://192.168.61.97:81/stream")
        mjpeg!!.isRecycleBitmap = true

        //mjpeg!!.
        mjpeg!!.startStream()
    }


    override fun onResume() {
        super.onResume()
        mjpeg!!.startStream()
        startFrameCapture()
    }

    override fun onPause() {
        super.onPause()
        mjpeg!!.stopStream()
        stopFrameCapture()
    }

    fun capture(mjpegView: MjpegView): Bitmap?{
        if (mjpegView.isLaidOut) {
            return mjpegView.drawToBitmap()
        } else {
            return null
        }
    }

    private fun startFrameCapture() {
        handler = Handler(mainLooper)
        handler.post(object : Runnable {
            override fun run() {
                val frameBitmap = capture(mjpeg!!)
                if (frameBitmap != null) {
                    processBitmap(frameBitmap)
//                    runOnUiThread {
//                        binding.imageView.setImageBitmap(frameBitmap)
//                    }
                }
                handler.postDelayed(this, captureInterval)
            }
        })
    }

    private fun stopFrameCapture() {
        handler.removeCallbacksAndMessages(null)
    }



    private fun processBitmap(bitmap: Bitmap) {
        var bOut: Bitmap = bitmap.copy(bitmap.config, true)
        detectorBordes(bitmap, bOut)
        binding.imageView.setImageBitmap(bOut)
    }


    /**
     * A native method that is implemented by the 'proyectocamara' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String
    external fun detectorBordes(bIn: Bitmap, bOut: Bitmap)
    //external fun filtro(bIn: Bitmap, bOut: Bitmap);

    companion object {
        // Used to load the 'proyectocamara' library on application startup.
        init {
            System.loadLibrary("proyectocamara")
        }
    }
}