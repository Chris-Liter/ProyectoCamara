package com.mi.proyectocamara

import android.graphics.Bitmap
import android.net.TrafficStats
import android.os.Bundle
import android.os.Handler
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.SeekBar
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.core.view.drawToBitmap
import com.longdo.mjpegviewer.MjpegView
import com.mi.proyectocamara.databinding.FragmentIntercicloBinding
import java.net.HttpURLConnection
import java.net.URL
import kotlin.concurrent.thread

class Interciclo : Fragment() {

    private var _binding: FragmentIntercicloBinding? = null
    private val binding get() = _binding!!

    private var mjpeg: MjpegView? = null
    private lateinit var boton: Button
    private lateinit var handler: Handler
    private val captureInterval: Long = 50
    private lateinit var seekBar: SeekBar
    private var valor: Int = 10
    private lateinit var anchoBanda: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        _binding = FragmentIntercicloBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // Inicialización de vistas usando View Binding
        mjpeg = binding.video
        boton = binding.prender
        anchoBanda = binding.ancho
        seekBar = binding.barra

        boton.setOnClickListener {
            extraer()
        }

        seekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                valor = progress
                onTrack(valor)
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                valor = seekBar!!.progress
                onTrack(valor)
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                valor = seekBar!!.progress
                onTrack(valor)
            }
        })
    }

    private fun extraer() {
        mjpeg?.apply {
            mode = MjpegView.MODE_FIT_WIDTH
            isAdjustHeight = true
            supportPinchZoomAndPan = true
            setUrl("http://192.168.239.97:81/stream")
            isRecycleBitmap = true
            startStream()
        }
        startFrameCapture()
    }

    override fun onResume() {
        super.onResume()
        mjpeg?.startStream()
        startFrameCapture()
    }

    override fun onPause() {
        super.onPause()
        mjpeg?.stopStream()
        stopFrameCapture()
    }

    private fun capture(mjpegView: MjpegView): Bitmap? {
        return if (mjpegView.isLaidOut) {
            mjpegView.drawToBitmap()
        } else {
            null
        }
    }

    private fun startFrameCapture() {
        handler = Handler()
        handler.post(object : Runnable {
            override fun run() {
                val frameBitmap = capture(mjpeg!!)
                if (frameBitmap != null) {
                    processBitmap(frameBitmap)
                }
                handler.postDelayed(this, captureInterval)
            }
        })
    }

    private fun stopFrameCapture() {
        handler.removeCallbacksAndMessages(null)
    }

    private fun processBitmap(bitmap: Bitmap) {
        val bOut: Bitmap = bitmap.copy(bitmap.config, true)
        detectorBordes(bitmap, bOut)
        binding.imageView.setImageBitmap(bOut)
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

    external fun stringFromJNI(): String
    external fun detectorBordes(bIn: Bitmap, bOut: Bitmap)
    external fun onTrack(valor: Int)

    companion object {
        init {
            System.loadLibrary("proyectocamara")
        }

        @JvmStatic
        fun newInstance(param1: String, param2: String) =
            Interciclo().apply {
                arguments = Bundle().apply {
                    putString("param1", param1)
                    putString("param2", param2)
                }
            }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
