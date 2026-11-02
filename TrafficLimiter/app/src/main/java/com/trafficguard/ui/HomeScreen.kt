package com.trafficguard.ui

import android.Manifest
import android.content.Intent
import android.net.Uri
import android.net.VpnService
import android.os.Build
import android.provider.Settings
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Checkbox
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.trafficguard.data.BlockPlan
import com.trafficguard.data.Engine
import com.trafficguard.data.Live
import com.trafficguard.data.LiveStatus
import com.trafficguard.data.NetKind
import com.trafficguard.data.NetType
import com.trafficguard.data.Profile
import com.trafficguard.data.SettingsRepo
import com.trafficguard.data.fmtBytes
import com.trafficguard.data.fmtDuration
import com.trafficguard.data.quotaName
import com.trafficguard.service.NetworkWatcher
import com.trafficguard.service.Perms
import com.trafficguard.service.TrafficService
import java.text.DateFormat
import java.util.Date

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HomeScreen(onOpenProfile: (String) -> Unit) {
    val ctx = LocalContext.current
    val settings by SettingsRepo.settings.collectAsStateWithLifecycle()
    val live by Live.status.collectAsStateWithLifecycle()

    Scaffold(
        topBar = { TopAppBar(title = { Text("Лимит трафика") }) },
        floatingActionButton = {
            ExtendedFloatingActionButton(
                onClick = {
                    val net = live.net
                    val ssid = if (net.kind == NetKind.WIFI) net.ssid ?: NetworkWatcher.currentSsid(ctx) else null
                    val p = when {
                        ssid != null -> Profile(SettingsRepo.newId(), ssid, NetType.WIFI, ssid = ssid, limitEnabled = true)
                        net.kind == NetKind.MOBILE -> Profile(SettingsRepo.newId(), "Мобильная сеть", NetType.MOBILE, limitEnabled = true)
                        else -> Profile(SettingsRepo.newId(), "Новая сеть", NetType.WIFI, limitEnabled = true)
                    }
                    SettingsRepo.update { it.copy(profiles = it.profiles + p) }
                    onOpenProfile(p.id)
                },
                icon = { Icon(Icons.Default.Add, null) },
                text = { Text("Добавить сеть") },
            )
        },
    ) { pad ->
        LazyColumn(
            Modifier.fillMaxSize().padding(pad),
            contentPadding = PaddingValues(start = 16.dp, end = 16.dp, top = 8.dp, bottom = 96.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            item {
                Card(Modifier.fillMaxWidth()) {
                    Column(Modifier.padding(12.dp)) {
                        CheckRow(
                            checked = settings.limitingEnabled,
                            title = "Ограничивать трафик",
                            subtitle = "Главная галочка. Если снята — ничего не блокируется, трафик только считается.",
                        ) { v -> SettingsRepo.update { it.copy(limitingEnabled = v) } }
                        CheckRow(
                            checked = settings.monitorEnabled,
                            title = "Работать в фоне",
                            subtitle = "Служба считает трафик и применяет правила, даже когда приложение закрыто. Запускается после перезагрузки.",
                        ) { v ->
                            SettingsRepo.update { it.copy(monitorEnabled = v) }
                            if (v) TrafficService.start(ctx) else TrafficService.stop(ctx)
                        }
                    }
                }
            }
            item { StatusCard(live, settings.profiles.firstOrNull { it.id == live.profileId }, settings.limitingEnabled) }
            item { PermissionsCard(settings.profiles) }
            item {
                Text(
                    "Сети",
                    style = MaterialTheme.typography.titleLarge,
                    modifier = Modifier.padding(top = 8.dp),
                )
                Text(
                    "Для каждой сети свои лимиты и правила приложений. Если для Wi‑Fi задано имя — профиль работает только в этой сети.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            }
            items(settings.profiles, key = { it.id }) { p ->
                ProfileItem(p, active = p.id == live.profileId, onClick = { onOpenProfile(p.id) })
            }
        }
    }
}

@Composable
private fun StatusCard(live: LiveStatus, profile: Profile?, limiting: Boolean) {
    val d = live.decision
    val blocked = d?.exceeded != null
    Card(
        Modifier.fillMaxWidth(),
        colors = if (blocked) CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.errorContainer)
        else CardDefaults.cardColors(),
    ) {
        Column(Modifier.padding(16.dp)) {
            if (!live.running) {
                Text("Служба не запущена", style = MaterialTheme.typography.titleMedium)
                Text(
                    "Включите «Работать в фоне», чтобы считать и ограничивать трафик.",
                    style = MaterialTheme.typography.bodySmall,
                )
                return@Column
            }
            val netText = when (live.net.kind) {
                NetKind.WIFI -> "Wi‑Fi: " + (live.net.ssid?.let { "«$it»" } ?: "имя неизвестно (нужна геолокация)")
                NetKind.MOBILE -> "Мобильная сеть"
                NetKind.OTHER -> "Другая сеть (Ethernet, USB…)"
                NetKind.NONE -> "Нет подключения"
            }
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(netText, style = MaterialTheme.typography.titleMedium, modifier = Modifier.weight(1f))
                Text("${fmtBytes(live.bytesPerSecond)}/с", style = MaterialTheme.typography.titleMedium)
            }
            Text(
                profile?.let { "Профиль: ${it.name}" } ?: "Нет профиля для этой сети — трафик не ограничивается",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
            val u = live.usage
            if (profile != null && u != null) {
                Spacer(Modifier.height(8.dp))
                QuotaRow("За минуту", u.minute, profile.perMinute)
                QuotaRow("За час", u.hour, profile.perHour)
                QuotaRow("За день", u.day, profile.perDay)
                QuotaRow(
                    "Всего с ${DateFormat.getDateInstance(DateFormat.SHORT).format(Date(u.totalSince))}",
                    u.total, profile.maxTotal,
                )
            }
            Spacer(Modifier.height(8.dp))
            HorizontalDivider()
            Spacer(Modifier.height(8.dp))
            val now = System.currentTimeMillis()
            val state = when {
                d == null -> "—"
                d.exceeded != null -> {
                    val wait = Engine.untilReset(d.exceeded, now)?.let { "Разблокировка через ${fmtDuration(it)}." }
                        ?: "Сбросьте счётчик профиля, чтобы разблокировать."
                    "⛔ Лимит ${quotaName(d.exceeded)} исчерпан. Заблокировано всё, кроме «Всегда разрешать». $wait"
                }
                d.plan is BlockPlan.AllExcept ->
                    "Фоновый трафик заблокирован. Интернет есть только у открытого приложения" +
                        (live.foreground?.let { " ($it)" } ?: "") + " и у «Всегда разрешать»." +
                        if (d.blockedApps.isNotEmpty()) " Заблокировано правилами: ${d.blockedApps.size}." else ""
                d.plan is BlockPlan.Only -> "Заблокировано приложений: ${d.blockedApps.size}"
                !limiting -> "Ограничение выключено"
                else -> "✓ " + d.note
            }
            Text(state, style = MaterialTheme.typography.bodyMedium, fontWeight = FontWeight.Medium)
            if (live.vpnPermissionMissing) {
                Text("Нет разрешения VPN — блокировка не работает!", color = MaterialTheme.colorScheme.error)
            }
            live.vpnError?.let { Text(it, color = MaterialTheme.colorScheme.error) }
            if (d != null && d.plan !is BlockPlan.AllExcept && d.exceeded == null && d.note.startsWith("Блокировка фона")) {
                Text(d.note, color = MaterialTheme.colorScheme.error, style = MaterialTheme.typography.bodySmall)
            }
        }
    }
}

private data class PermItem(val title: String, val text: String, val button: String, val action: () -> Unit)

@Composable
private fun PermissionsCard(profiles: List<Profile>) {
    val ctx = LocalContext.current
    val tick = rememberResumeTick()
    val vpnLauncher = rememberLauncherForActivityResult(ActivityResultContracts.StartActivityForResult()) {}
    val permLauncher = rememberLauncherForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) {}

    val items = remember(tick, profiles) {
        val needSsid = profiles.any { it.type == NetType.WIFI && it.ssid.isNotBlank() }
        val needUsage = profiles.any { p -> p.blockBackground || p.apps.values.any { it.dailyLimit > 0 } }
        buildList {
            if (!Perms.vpn(ctx)) add(PermItem(
                "Разрешение VPN",
                "Нужно для блокировки приложений. Трафик никуда не отправляется — пакеты заблокированных приложений просто отбрасываются на телефоне.",
                "Разрешить",
            ) { VpnService.prepare(ctx)?.let { vpnLauncher.launch(it) } })
            if (!Perms.notifications(ctx) && Build.VERSION.SDK_INT >= 33) add(PermItem(
                "Уведомления",
                "Показывают расход в шторке и предупреждают о превышении лимита.",
                "Разрешить",
            ) { permLauncher.launch(arrayOf(Manifest.permission.POST_NOTIFICATIONS)) })
            if (!Perms.location(ctx)) add(PermItem(
                "Геолокация",
                "Android сообщает имя Wi‑Fi сети только приложениям с доступом к геолокации. Без неё профили для конкретного Wi‑Fi не сработают.",
                "Разрешить",
            ) {
                permLauncher.launch(arrayOf(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION))
            })
            else if (needSsid && !Perms.backgroundLocation(ctx)) add(PermItem(
                "Геолокация «Разрешить всегда»",
                "Чтобы имя Wi‑Fi определялось, когда приложение закрыто.",
                "Открыть",
            ) { permLauncher.launch(arrayOf(Manifest.permission.ACCESS_BACKGROUND_LOCATION)) })
            if (needSsid && !Perms.locationServiceOn(ctx)) add(PermItem(
                "Геолокация выключена",
                "Включите геолокацию в шторке — без неё Android скрывает имя Wi‑Fi.",
                "Настройки",
            ) { ctx.startActivity(Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS)) })
            if (!Perms.usageAccess(ctx)) add(PermItem(
                "Доступ к истории использования" + if (needUsage) " (нужен!)" else "",
                "Для расхода по приложениям, дневных лимитов приложений и блокировки фонового трафика.",
                "Открыть",
            ) { ctx.startActivity(Intent(Settings.ACTION_USAGE_ACCESS_SETTINGS)) })
            if (!Perms.ignoresBatteryOptimizations(ctx)) add(PermItem(
                "Оптимизация батареи",
                "Отключите, чтобы Android не останавливал службу в фоне.",
                "Отключить",
            ) {
                runCatching {
                    ctx.startActivity(
                        Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS, Uri.parse("package:${ctx.packageName}")),
                    )
                }
            })
        }
    }
    if (items.isEmpty()) return

    Card(
        Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.secondaryContainer),
    ) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(Icons.Default.Warning, null)
                Text("  Нужны разрешения", style = MaterialTheme.typography.titleMedium)
            }
            items.forEach { it ->
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Column(Modifier.weight(1f)) {
                        Text(it.title, style = MaterialTheme.typography.bodyLarge, fontWeight = FontWeight.Medium)
                        Text(it.text, style = MaterialTheme.typography.bodySmall)
                    }
                    FilledTonalButton(onClick = it.action, modifier = Modifier.padding(start = 8.dp)) { Text(it.button) }
                }
            }
        }
    }
}

fun profileTypeText(p: Profile) = when (p.type) {
    NetType.WIFI -> if (p.ssid.isBlank()) "Любой Wi‑Fi" else "Wi‑Fi «${p.ssid}»"
    NetType.MOBILE -> "Мобильная сеть"
    NetType.ANY -> "Любая сеть"
}

private fun limitsText(p: Profile): String {
    val parts = buildList {
        if (p.perMinute > 0) add("${fmtBytes(p.perMinute)}/мин")
        if (p.perHour > 0) add("${fmtBytes(p.perHour)}/час")
        if (p.perDay > 0) add("${fmtBytes(p.perDay)}/день")
        if (p.maxTotal > 0) add("макс ${fmtBytes(p.maxTotal)}")
        if (p.blockBackground) add("без фона")
        val blocked = p.apps.values.count { it.mode == com.trafficguard.data.AppMode.BLOCK }
        if (blocked > 0) add("блок: $blocked прил.")
    }
    return if (parts.isEmpty()) "Лимиты не заданы" else parts.joinToString(" · ")
}

@Composable
private fun ProfileItem(p: Profile, active: Boolean, onClick: () -> Unit) {
    Card(onClick = onClick, modifier = Modifier.fillMaxWidth()) {
        Row(Modifier.padding(vertical = 8.dp, horizontal = 4.dp), verticalAlignment = Alignment.CenterVertically) {
            Checkbox(
                checked = p.limitEnabled,
                onCheckedChange = { v -> SettingsRepo.updateProfile(p.id) { it.copy(limitEnabled = v) } },
            )
            Column(Modifier.weight(1f)) {
                Text(
                    p.name + if (active) "  • сейчас" else "",
                    style = MaterialTheme.typography.titleMedium,
                    color = if (active) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurface,
                )
                Text(profileTypeText(p), style = MaterialTheme.typography.bodySmall)
                Text(limitsText(p), style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
            }
            Icon(Icons.AutoMirrored.Filled.KeyboardArrowRight, null, Modifier.padding(end = 8.dp))
        }
    }
}
