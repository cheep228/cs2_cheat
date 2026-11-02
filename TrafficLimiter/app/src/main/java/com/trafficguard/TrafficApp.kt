package com.trafficguard

import android.app.Application
import com.trafficguard.data.SettingsRepo
import com.trafficguard.data.UsageStore

class TrafficApp : Application() {
    override fun onCreate() {
        super.onCreate()
        SettingsRepo.init(this)
        UsageStore.init(this)
    }
}
