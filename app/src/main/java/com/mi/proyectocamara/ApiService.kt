package com.mi.proyectocamara

import okhttp3.MultipartBody
import okhttp3.ResponseBody
import retrofit2.Call
import retrofit2.http.Multipart
import retrofit2.http.POST
import retrofit2.http.Part

interface ApiService {
    @Multipart
    @POST("video_feed")
    fun uploadFrame(@Part frame: MultipartBody.Part): Call<ResponseBody>
}
