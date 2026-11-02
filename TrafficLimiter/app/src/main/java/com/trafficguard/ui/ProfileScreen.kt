package com.trafficguard.ui

import android.widget.Toast
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilterChip
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.trafficguard.data.AppMode
import com.trafficguard.data.Live
import com.trafficguard.data.NetKind
import com.trafficguard.data.NetType
import com.trafficguard.data.SettingsRepo
import com.trafficguard.data.UsageStore
import com.trafficguard.service.NetworkWatcher

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ProfileScreen(id: String, onBack: () -> Unit, onApps: () -> Unit) {
    val ctx = LocalContext.current
    val settings by SettingsRepo.settings.collectAsStateWithLifecycle()
    val live by Live.status.collectAsStateWithLifecycle()
    val p = settings.profiles.firstOrNull { it.id == id }
    if (p == null) {
        LaunchedEffect(Unit) { onBack() }
        return
    }
    var confirmDelete by remember { mutableStateOf(false) }
    var confirmReset by remember { mutableStateOf(false) }
    var name by remember(id) { mutableStateOf(p.name) }
    var ssid by remember(id) { mutableStateOf(p.ssid) }
    val update: ((com.trafficguard.data.Profile) -> com.trafficguard.data.Profile) -> Unit =
        { f -> SettingsRepo.updateProfile(id, f) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(p.name.ifBlank { "Сеть" }) },
                navigationIcon = { IconButton(onClick = onBack) { Icon(Icons.AutoMirrored.Filled.ArrowBack, "Назад") } },
                actions = { IconButton(onClick = { confirmDelete = true }) { Icon(Icons.Default.Delete, "Удалить") } },
            )
        },
    ) { pad ->
        Column(
            Modifier.fillMaxSize().padding(pad).verticalScroll(rememberScrollState()).padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            OutlinedTextField(
                value = name,
                onValueChange = { v -> name = v; update { it.copy(name = v) } },
                label = { Text("Название") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
            )

            Text("Для какой сети", style = MaterialTheme.typography.titleMedium)
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                listOf(NetType.WIFI to "Wi‑Fi", NetType.MOBILE to "Мобильная", NetType.ANY to "Любая").forEach { (t, label) ->
                    FilterChip(selected = p.type == t, onClick = { update { it.copy(type = t) } }, label = { Text(label) })
                }
            }
            if (p.type == NetType.WIFI) {
                OutlinedTextField(
                    value = ssid,
                    onValueChange = { v -> ssid = v; update { it.copy(ssid = v.trim()) } },
                    label = { Text("Имя Wi‑Fi (SSID)") },
                    placeholder = { Text("пусто — любой Wi‑Fi") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth(),
                )
                TextButton(onClick = {
                    val cur = live.net.takeIf { it.kind == NetKind.WIFI }?.ssid ?: NetworkWatcher.currentSsid(ctx)
                    if (cur == null) {
                        Toast.makeText(ctx, "Не удалось узнать имя Wi‑Fi. Подключитесь к Wi‑Fi и дайте доступ к геолокации.", Toast.LENGTH_LONG).show()
                    } else {
                        ssid = cur
                        update { it.copy(ssid = cur) }
                    }
                }) { Text("Подставить текущую Wi‑Fi сеть") }
            }

            Card(Modifier.fillMaxWidth()) {
                Column(Modifier.padding(12.dp)) {
                    CheckRow(
                        checked = p.limitEnabled,
                        title = "Ограничивать трафик в этой сети",
                        subtitle = "Если снято — в этой сети трафик только считается.",
                    ) { v -> update { it.copy(limitEnabled = v) } }
                    CheckRow(
                        checked = p.blockBackground,
                        title = "Блокировать фоновый трафик",
                        subtitle = "Интернет получает только открытое на экране приложение и «Всегда разрешать». " +
                            "Фоновые загрузки, обновления, синхронизация — отключаются. Нужен доступ к истории использования.",
                    ) { v -> update { it.copy(blockBackground = v) } }
                }
            }

            Text("Лимиты", style = MaterialTheme.typography.titleMedium)
            Text(
                "Когда любой лимит исчерпан, отключается ВЕСЬ трафик, кроме приложений «Всегда разрешать». " +
                    "Минутный, часовой и дневной лимиты сбрасываются сами; «Максимум» — только кнопкой сброса. Пусто = без лимита.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
            MbField("В минуту", p.perMinute, id + "m") { b -> update { it.copy(perMinute = b) } }
            MbField("В час", p.perHour, id + "h") { b -> update { it.copy(perHour = b) } }
            MbField("В день", p.perDay, id + "d") { b -> update { it.copy(perDay = b) } }
            MbField("Максимум всего (до сброса)", p.maxTotal, id + "t") { b -> update { it.copy(maxTotal = b) } }

            val usage = live.usage?.takeIf { live.profileId == id } ?: UsageStore.get(id, System.currentTimeMillis())
            Card(Modifier.fillMaxWidth()) {
                Column(Modifier.padding(12.dp)) {
                    Text("Израсходовано", style = MaterialTheme.typography.titleMedium)
                    QuotaRow("За минуту", usage.minute, p.perMinute)
                    QuotaRow("За час", usage.hour, p.perHour)
                    QuotaRow("За день", usage.day, p.perDay)
                    QuotaRow("Всего", usage.total, p.maxTotal)
                    OutlinedButton(onClick = { confirmReset = true }, modifier = Modifier.padding(top = 8.dp)) {
                        Text("Сбросить счётчики")
                    }
                }
            }

            val blocked = p.apps.values.count { it.mode == AppMode.BLOCK }
            val allowed = p.apps.values.count { it.mode == AppMode.ALLOW }
            val limited = p.apps.values.count { it.mode == AppMode.NORMAL && it.dailyLimit > 0 }
            Button(onClick = onApps, modifier = Modifier.fillMaxWidth()) {
                Text("Приложения · блок $blocked · разрешено $allowed · с лимитом $limited")
            }
        }
    }

    if (confirmDelete) {
        AlertDialog(
            onDismissRequest = { confirmDelete = false },
            title = { Text("Удалить «${p.name}»?") },
            text = { Text("Лимиты и правила приложений для этой сети будут удалены.") },
            confirmButton = {
                TextButton(onClick = {
                    confirmDelete = false
                    SettingsRepo.update { s -> s.copy(profiles = s.profiles.filterNot { it.id == id }) }
                    UsageStore.remove(id)
                }) { Text("Удалить") }
            },
            dismissButton = { TextButton(onClick = { confirmDelete = false }) { Text("Отмена") } },
        )
    }
    if (confirmReset) {
        AlertDialog(
            onDismissRequest = { confirmReset = false },
            title = { Text("Сбросить счётчики?") },
            text = { Text("Расход за минуту, час, день и общий станет нулевым. Если трафик был заблокирован по лимиту — он разблокируется.") },
            confirmButton = {
                TextButton(onClick = {
                    confirmReset = false
                    UsageStore.reset(id, System.currentTimeMillis())
                }) { Text("Сбросить") }
            },
            dismissButton = { TextButton(onClick = { confirmReset = false }) { Text("Отмена") } },
        )
    }
}
