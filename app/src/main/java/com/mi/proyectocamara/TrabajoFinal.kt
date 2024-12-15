package com.mi.proyectocamara

import android.app.Activity
import retrofit2.Call
import okhttp3.ResponseBody
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.ContactsContract.Contacts
import android.util.Log
import android.util.Size
import androidx.fragment.app.Fragment
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.Toast
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageCapture
import androidx.camera.core.ImageProxy
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.mi.proyectocamara.databinding.FragmentIntercicloBinding
import com.mi.proyectocamara.databinding.FragmentTrabajoFinalBinding
import java.util.concurrent.ExecutorService
import okhttp3.*
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import okhttp3.RequestBody.Companion.create
import retrofit2.Retrofit
import retrofit2.awaitResponse
import retrofit2.converter.gson.GsonConverterFactory
import java.io.ByteArrayOutputStream
import java.io.IOException
import java.util.concurrent.Executors

/**
 * A simple [Fragment] subclass.
 * Use the [TrabajoFinal.newInstance] factory method to
 * create an instance of this fragment.
 */
class TrabajoFinal : Fragment() {
    // TODO: Rename and change types of parameters

    private lateinit var imageCapture: ImageCapture
    private lateinit var img: ImageView

    private val handler = Handler(Looper.getMainLooper())
    private val frameInterval: Long = 5  // Intervalo en milisegundos (500ms)


    private val executor: ExecutorService = Executors.newSingleThreadExecutor()

    //////////////////////////////
    val retrofit = Retrofit.Builder()
        .baseUrl("http://192.168.10.127:5000/")  // URL de tu servidor Flask
        .addConverterFactory(GsonConverterFactory.create())
        .build()

    val apiService = retrofit.create(ApiService::class.java)



    private var _binding: FragmentTrabajoFinalBinding? = null
    private val binding get() = _binding!!

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

    }



    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        _binding = FragmentTrabajoFinalBinding.inflate(inflater, container, false)
        //return inflater.inflate(R.layout.fragment_trabajo_final, container, false)

        //img = binding.im

        if(allPermissionsGranted()){
            startCamera()
        }else{
            activity?.let { ActivityCompat.requestPermissions(it, Constans.REQUIRED_PERMISSIONS, Constans.REQUEST_CODE_PERMISSIONS) }
        }

        return binding.root
    }

    private fun process(bitmap: Bitmap){
        val bIn: Bitmap = bitmap.copy(bitmap.config, true)

        detectorBordes(bitmap, bIn)
        sendFrameToServer(bitmap)
    }
    private fun startSendingFrames() {
        // Cada frameInterval milisegundos, ejecuta la función que envía el frame
        handler.postDelayed(object : Runnable {
            override fun run() {
                binding.camara.bitmap?.let { sendFrameToServer(it) }
                // Llama nuevamente a esta función después del intervalo
                handler.postDelayed(this, frameInterval)
            }
        }, frameInterval)
    }


    override fun onResume() {
        super.onResume()
        startSendingFrames()
    }

    override fun onPause() {
        super.onPause()
        // Detener el envío de frames cuando la actividad esté pausada
        handler.removeCallbacksAndMessages(null)
    }

    private fun allPermissionsGranted()=
        Constans.REQUIRED_PERMISSIONS.all{
            context?.let { it1 ->
                ContextCompat.checkSelfPermission(
                    it1, it
                )
            } == PackageManager.PERMISSION_GRANTED
        }



    private fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(requireContext())

        cameraProviderFuture.addListener({
            val cameraProvider = cameraProviderFuture.get()
            val preview = Preview.Builder()
                .build()
                .also {
                    it.setSurfaceProvider(binding.camara.surfaceProvider)
                }



            val cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA

            try {
                cameraProvider.unbindAll()
                cameraProvider.bindToLifecycle(
                    viewLifecycleOwner, cameraSelector, preview
                )
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }, ContextCompat.getMainExecutor(requireContext()))
    }


    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        if(requestCode == Constans.REQUEST_CODE_PERMISSIONS){
            if(allPermissionsGranted()){
                startCamera()
            }else{
                Toast.makeText(context, "Los permisos no se concedieron.", Toast.LENGTH_SHORT).show()


            }
        }
    }

    private fun sendFrameToServer(bitmap: Bitmap) {
        executor.execute {
            // Convertir el Bitmap a bytes
            val byteArrayOutputStream = ByteArrayOutputStream()
            bitmap.compress(Bitmap.CompressFormat.JPEG, 90, byteArrayOutputStream)
            val byteArray = byteArrayOutputStream.toByteArray()

            if (byteArray.isEmpty()) {
                Log.e("Error", "La imagen no se convirtió correctamente a bytes.")
            }else{
                Log.e("Si", "Funciona")
            }


            // Crear el archivo MultipartBody
            val requestBody = create("image/jpeg".toMediaTypeOrNull(), byteArray)
            val multipartBody = MultipartBody.Part.createFormData("frame", "frame.jpg", requestBody)

            val call = apiService.uploadFrame(multipartBody)


            try {
                val response = call.execute()  // Llamada sincrónica
                if (response.isSuccessful) {
                    val body = response.body()  // La respuesta ya está en el cuerpo
                    Log.d("Retrofit", "Respuesta exitosa: $body")
                } else {
                    val error = response.errorBody()?.string()
                    Log.e("Retrofit", "Error en la respuesta: $error")
                }
            } catch (e: Exception) {
                Log.e("Retrofit", "Error en la solicitud: ${e.message}")
            }
        }
    }





    external fun detectorBordes(bIn: Bitmap, bOut: Bitmap)
    companion object {
        init {
            System.loadLibrary("proyectocamara")
        }
        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @param param1 Parameter 1.
         * @param param2 Parameter 2.
         * @return A new instance of fragment TrabajoFinal.
         */
        // TODO: Rename and change types and number of parameters
        @JvmStatic
        fun newInstance(param1: String, param2: String) =
            TrabajoFinal().apply {

            }
    }
}
