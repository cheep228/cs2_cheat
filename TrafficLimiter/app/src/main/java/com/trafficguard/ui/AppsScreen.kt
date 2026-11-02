package com.trafficguard.ui

import android.content.Intent
import android.provider.Settings
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectable
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilterChip
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.trafficguard.data.AppMode
import com.trafficguard.data.AppRule
import com.trafficguard.data.NetKind
import com.trafficguard.data.NetType
import com.trafficguard.data.SettingsRepo
import com.trafficguard.data.fmtBytes
import com.trafficguard.service.AppInfo
import com.trafficguard.service.AppList
import com.trafficguard.service.NetStats
import com.trafficguard.service.Perms
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext

private enum class Filter(val label: String) { ALL("Все"), BLOCK("Блок"), ALLOW("Разрешены"), LIMIT("С лимитом") }

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AppsScreen(profileId: String, onBack: () -> Unit) {
    val ctx = LocalContext.current
    val settings by SettingsRepo.settings.collectAsStateWithLifecycle()
    val p = settings.profiles.firstOrNull { it.id == profileId }
    if (p == null) {
        androidx.compose.runtime.LaunchedEffect(Unit) { onBack() }
        return
    }
    val tick = rememberResumeTick()
    val hasUsage = remember(tick) { Perms.usageAccess(ctx) }

    val apps by produceState<List<AppInfo>?>(null) {
        value = withContext(Dispatchers.IO) { AppList.load(ctx) }
    }
    val kind = when (p.type) {
        NetType.WIFI -> NetKind.WIFI
        NetType.MOBILE -> NetKind.MOBILE
        NetType.ANY -> NetKind.OTHER
    }
    var usage by remember { mutableStateOf<Map<String, Long>>(emptyMap()) }
    androidx.compose.runtime.LaunchedEffect(hasUsage, kind) {
        while (hasUsage) {
            usage = withContext(Dispatchers.IO) { runCatching { NetStats.todayByPackage(ctx, kind) }.getOrDefault(emptyMap()) }
            delay(15_000)
        }
    }

    var query by rememberSaveable { mutableStateOf("") }
    var showSystem by rememberSaveable { mutableStateOf(false) }
    var filter by rememberSaveable { mutableStateOf(Filter.ALL) }
    var editing by remember { mutableStateOf<AppInfo?>(null) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Приложения · ${p.name}", maxLines = 1, overflow = TextOverflow.Ellipsis) },
                navigationIcon = { IconButton(onClick = onBack) { Icon(Icons.AutoMirrored.Filled.ArrowBack, "Назад") } },
            )
        },
    ) { pad ->
        Column(Modifier.fillMaxSize().padding(pad)) {
            OutlinedTextField(
                value = query,
                onValueChange = { query = it },
                leadingIcon = { Icon(Icons.Default.Search, null) },
                placeholder = { Text("Поиск") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp),
            )
            Row(
                Modifier.horizontalScroll(rememberScrollState()).padding(horizontal = 16.dp, vertical = 4.dp),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Filter.entries.forEach { f ->
                    FilterChip(selected = filter == f, onClick = { filter = f }, label = { Text(f.label) })
                }
                FilterChip(selected = showSystem, onClick = { showSystem = !showSystem }, label = { Text("Системные") })
            }
            if (!hasUsage) {
                Row(Modifier.padding(horizontal = 16.dp), verticalAlignment = Alignment.CenterVertically) {
                    Text(
                        "Нет доступа к истории использования: расход и дневные лимиты приложений не работают.",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.error,
                        modifier = Modifier.weight(1f),
                    )
                    TextButton(onClick = { ctx.startActivity(Intent(Settings.ACTION_USAGE_ACCESS_SETTINGS)) }) { Text("Дать") }
                }
            }

            val list = apps
            if (list == null) {
                Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) { CircularProgressIndicator() }
                return@Column
            }
            val q = query.trim().lowercase()
            val shown = list.asSequence()
                .filter { a -> showSystem || !a.system || !p.rule(a.pkg).isDefault }
                .filter { a -> q.isEmpty() || a.label.lowercase().contains(q) || a.pkg.contains(q) }
                .filter { a ->
                    val r = p.rule(a.pkg)
                    when (filter) {
                        Filter.ALL -> true
                        Filter.BLOCK -> r.mode == AppMode.BLOCK
                        Filter.ALLOW -> r.mode == AppMode.ALLOW
                        Filter.LIMIT -> r.mode == AppMode.NORMAL && r.dailyLimit > 0
                    }
                }
                .sortedWith(compareByDescending<AppInfo> { usage[it.pkg] ?: 0L }.thenBy { it.label.lowercase() })
                .toList()

            LazyColumn(Modifier.fillMaxSize()) {
                items(shown, key = { it.pkg }) { a ->
                    AppRow(a, p.rule(a.pkg), usage[a.pkg], onClick = { editing = a })
                }
            }
        }
    }

    editing?.let { a ->
        RuleDialog(
            app = a,
            rule = p.rule(a.pkg),
            onDismiss = { editing = null },
            onSave = { r ->
                editing = null
                SettingsRepo.updateProfile(profileId) {
                    it.copy(apps = if (r.isDefault) it.apps - a.pkg else it.apps + (a.pkg to r))
                }
            },
        )
    }
}

@Composable
private fun AppRow(a: AppInfo, rule: AppRule, used: Long?, onClick: () -> Unit) {
    Row(
        Modifier.fillMaxWidth().clickable(onClick = onClick).padding(horizontal = 16.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        AppIcon(a.pkg)
        Spacer(Modifier.width(12.dp))
        Column(Modifier.weight(1f)) {
            Text(a.label, maxLines = 1, overflow = TextOverflow.Ellipsis, style = MaterialTheme.typography.bodyLarge)
            Text(
                buildString {
                    append(used?.let { "сегодня ${fmtBytes(it)}" } ?: a.pkg)
                    if (rule.mode == AppMode.NORMAL && rule.dailyLimit > 0) append(" из ${fmtBytes(rule.dailyLimit)}")
                },
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
        }
        val (text, color) = when {
            rule.mode == AppMode.BLOCK -> "Блок" to MaterialTheme.colorScheme.errorContainer
            rule.mode == AppMode.ALLOW -> "Всегда" to MaterialTheme.colorScheme.primaryContainer
            rule.dailyLimit > 0 -> "Лимит" to MaterialTheme.colorScheme.tertiaryContainer
            else -> "" to MaterialTheme.colorScheme.surface
        }
        if (text.isNotEmpty()) {
            Surface(color = color, shape = MaterialTheme.shapes.small) {
                Text(text, Modifier.padding(horizontal = 8.dp, vertical = 4.dp), style = MaterialTheme.typography.labelMedium)
            }
        }
    }
}

@Composable
private fun RuleDialog(app: AppInfo, rule: AppRule, onDismiss: () -> Unit, onSave: (AppRule) -> Unit) {
    var mode by remember { mutableStateOf(rule.mode) }
    var limit by remember { mutableLongStateOf(rule.dailyLimit) }
    AlertDialog(
        onDismissRequest = onDismiss,
        icon = { AppIcon(app.pkg, 48.dp) },
        title = { Text(app.label) },
        text = {
            Column {
                listOf(
                    AppMode.NORMAL to ("Обычный" to "Подчиняется лимитам сети. Можно задать свой дневной лимит."),
                    AppMode.BLOCK to ("Блокировать всегда" to "В этой сети приложение не получает интернет."),
                    AppMode.ALLOW to ("Всегда разрешать" to "Не блокируется никогда, даже когда лимит сети исчерпан."),
                ).forEach { (m, t) ->
                    Row(
                        Modifier.fillMaxWidth().selectable(selected = mode == m, onClick = { mode = m }).padding(vertical = 4.dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        RadioButton(selected = mode == m, onClick = { mode = m })
                        Column {
                            Text(t.first, style = MaterialTheme.typography.bodyLarge)
                            Text(t.second, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
                        }
                    }
                }
                if (mode == AppMode.NORMAL) {
                    MbField("Лимит приложения в день", limit, app.pkg, Modifier.padding(top = 8.dp)) { limit = it }
                }
            }
        },
        confirmButton = {
            TextButton(onClick = { onSave(AppRule(mode, if (mode == AppMode.NORMAL) limit else 0L)) }) { Text("Сохранить") }
        },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Отмена") } },
    )
}
