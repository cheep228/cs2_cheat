package com.niri.recents.ui

import android.content.ClipData
import android.content.ClipboardManager
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Checkbox
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import com.niri.recents.DiagStore
import com.niri.recents.ModuleStatus
import com.niri.recents.RecentsInfo
import com.niri.recents.RootShell
import com.niri.recents.SettingsStore
import com.niri.recents.WallpaperSync
import com.niri.recents.overview.DemoData
import com.niri.recents.overview.NiriOverviewView
import com.niri.recents.overview.OvColumn
import com.niri.recents.overview.OverviewStyle
import com.niri.recents.shared.NiriSettings
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlin.math.roundToInt

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent { NiriTheme { SettingsScreen() } }
    }
}

private val NiriBlue = Color(0xFF7FC8FF)

@Composable
fun NiriTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = darkColorScheme(
            primary = NiriBlue,
            onPrimary = Color(0xFF00283D),
            secondary = Color(0xFFFFC87F),
            background = Color(0xFF1A1A1A),
            surface = Color(0xFF1A1A1A),
            surfaceVariant = Color(0xFF262626),
            surfaceContainer = Color(0xFF262626),
            surfaceContainerHighest = Color(0xFF2E2E2E),
        ),
        content = content,
    )
}

@Composable
private fun rememberResumeTick(): Int {
    val owner = LocalLifecycleOwner.current
    var tick by remember { mutableIntStateOf(0) }
    DisposableEffect(owner) {
        val obs = LifecycleEventObserver { _, e -> if (e == Lifecycle.Event.ON_RESUME) tick++ }
        owner.lifecycle.addObserver(obs)
        onDispose { owner.lifecycle.removeObserver(obs) }
    }
    return tick
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen() {
    val ctx = LocalContext.current
    val scope = rememberCoroutineScope()
    var s by remember { mutableStateOf(SettingsStore.load(ctx)) }
    val update: (NiriSettings) -> Unit = { n ->
        s = n
        SettingsStore.save(ctx, n)
    }
    val tick = rememberResumeTick()
    var diagVersion by remember { mutableIntStateOf(0) }
    val active = remember(tick) { ModuleStatus.isActive() }
    val recentsPkg = remember { RecentsInfo.recentsPackage(ctx) ?: "?" }
    val diag = remember(tick, diagVersion) { DiagStore.lines(ctx) }
    val launcherSeen = diag.any { it.contains("Подключено") }

    // The launcher can't read the real wallpaper itself; copy it with root whenever we're opened.
    var wpStatus by remember { mutableStateOf<String?>(null) }
    var wpBitmap by remember { mutableStateOf<android.graphics.Bitmap?>(null) }
    LaunchedEffect(tick, s.wallpaper) {
        if (!s.wallpaper) return@LaunchedEffect
        val err = withContext(Dispatchers.IO) { WallpaperSync.sync(ctx) }
        wpStatus = if (err == null) "Ваши обои скопированы" else "Обои не скопированы: $err"
        wpBitmap = withContext(Dispatchers.IO) { WallpaperSync.load(ctx, 720) }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Niri Recents", fontWeight = FontWeight.SemiBold) },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = MaterialTheme.colorScheme.background),
            )
        },
    ) { pad ->
        LazyColumn(
            Modifier.fillMaxSize().padding(pad),
            contentPadding = PaddingValues(start = 16.dp, end = 16.dp, top = 4.dp, bottom = 32.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            item {
                StatusCard(active, launcherSeen, recentsPkg) {
                    scope.launch {
                        val (ok, out) = withContext(Dispatchers.IO) { RootShell.run("am force-stop $recentsPkg") }
                        Toast.makeText(
                            ctx,
                            if (ok) "Лаунчер перезапущен" else "Не вышло: $out",
                            Toast.LENGTH_LONG,
                        ).show()
                        diagVersion++
                    }
                }
            }
            item {
                Card(Modifier.fillMaxWidth()) {
                    Column(Modifier.padding(12.dp)) {
                        CheckRow(
                            checked = s.enabled,
                            title = "Niri-обзор вместо стандартного",
                            subtitle = "Если снять галочку, будут обычные «Недавние». Применяется при следующем открытии.",
                        ) { update(s.copy(enabled = it)) }
                    }
                }
            }
            item { PreviewCard(s, wpBitmap) }
            item { LookCard(s, wpStatus, update) }
            item { GesturesCard() }
            item {
                DiagCard(
                    diag,
                    onCopy = {
                        val cm = ctx.getSystemService(ClipboardManager::class.java)
                        cm.setPrimaryClip(ClipData.newPlainText("niri-recents", diag.joinToString("\n")))
                        Toast.makeText(ctx, "Скопировано", Toast.LENGTH_SHORT).show()
                    },
                    onClear = {
                        DiagStore.clear(ctx)
                        diagVersion++
                    },
                )
            }
        }
    }
}

@Composable
private fun StatusCard(active: Boolean, launcherSeen: Boolean, recentsPkg: String, onRestart: () -> Unit) {
    // The launcher's own report is the real proof; Vector doesn't always hook this app itself.
    val ok = launcherSeen
    Card(
        Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = if (ok) Color(0xFF1E3442) else MaterialTheme.colorScheme.surfaceContainerHighest,
        ),
    ) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
            Text(
                when {
                    ok -> "✓ Работает"
                    active -> "Модуль включён, ждём лаунчер"
                    else -> "Модуль не активен"
                },
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.SemiBold,
            )
            Text("Модуль в лаунчере: " + if (launcherSeen) "работает" else "ещё не отзывался", style = MaterialTheme.typography.bodyMedium)
            if (!launcherSeen) {
                Text("Модуль в Vector: " + if (active) "активен" else "не видно", style = MaterialTheme.typography.bodyMedium)
            }
            Text("«Недавние» рисует: $recentsPkg", style = MaterialTheme.typography.bodyMedium)
            if (!ok) {
                Spacer(Modifier.height(4.dp))
                Text(
                    "1. Откройте Vector → Модули → Niri Recents, включите модуль.\n" +
                        "2. В области действия отметьте $recentsPkg.\n" +
                        "3. Нажмите «Перезапустить лаунчер» ниже, затем откройте «Недавние».",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            }
            Spacer(Modifier.height(4.dp))
            Button(onClick = onRestart) { Text("Перезапустить лаунчер (root)") }
        }
    }
}

@Composable
private fun PreviewCard(s: NiriSettings, wallpaper: android.graphics.Bitmap?) {
    val ctx = LocalContext.current
    val dm = ctx.resources.displayMetrics
    val aspect = dm.widthPixels.toFloat() / dm.heightPixels.coerceAtLeast(1)
    Card(Modifier.fillMaxWidth()) {
        Column(Modifier.padding(16.dp)) {
            Text("Предпросмотр", style = MaterialTheme.typography.titleMedium)
            Text(
                "Можно трогать: листайте, смахивайте окно вверх, зажмите окно и перетащите на другой стол.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
            Spacer(Modifier.height(12.dp))
            Box(Modifier.fillMaxWidth(), contentAlignment = Alignment.Center) {
                Box(
                    Modifier
                        .fillMaxWidth(0.78f)
                        .aspectRatio(aspect)
                        .clip(RoundedCornerShape(28.dp))
                        .border(2.dp, Color(0xFF3A3A3A), RoundedCornerShape(28.dp)),
                ) {
                    AndroidView(
                        modifier = Modifier.fillMaxSize(),
                        factory = { c -> buildPreview(c, s) },
                        update = { v ->
                            v.style = OverviewStyle(s, v.resources.displayMetrics.density)
                            v.wallpaper = wallpaper
                        },
                    )
                }
            }
        }
    }
}

private fun buildPreview(c: android.content.Context, s: NiriSettings): NiriOverviewView {
    val v = NiriOverviewView(c)
    v.respectSystemGestures = false
    v.style = OverviewStyle(s, c.resources.displayMetrics.density)
    val apps = DemoData.installedApps(c)
    val dm = c.resources.displayMetrics
    val reset = {
        val (spaces, activeIndex, focus) = DemoData.build(apps, dm.widthPixels, dm.heightPixels, dm.density)
        v.setData(spaces, activeIndex, focus)
        v.open(null)
    }
    v.host = object : NiriOverviewView.Host {
        override fun onLaunch(column: OvColumn) = Unit
        override fun onLaunchVisualDone(column: OvColumn) {
            v.postDelayed({ reset() }, 650)
        }
        override fun onDismiss(column: OvColumn) = Unit
        override fun onHome() = v.closeToHome()
        override fun onCloseAnimationDone() {
            v.postDelayed({ reset() }, 350)
        }
    }
    reset()
    return v
}

private val RingColors = listOf(0xFF7FC8FF, 0xFFFFC87F, 0xFFD71921, 0xFFFFFFFF, 0xFFA6E3A1, 0xFFCBA6F7)
private val InactiveColors = listOf(0xFF505050, 0xFF303030, 0xFF808080, 0x00000000)
private val BackdropColors = listOf(0xFF262626, 0xFF000000, 0xFF101010, 0xFF1E1E2E, 0xFF2E3440)
private val WorkspaceColors = listOf(0xFF3A3A3A, 0xFF2A2A2A, 0xFF1A1A1A, 0xFF313244, 0xFF3B4252)

@Composable
private fun LookCard(s: NiriSettings, wpStatus: String?, update: (NiriSettings) -> Unit) {
    Card(Modifier.fillMaxWidth()) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Text("Внешний вид", style = MaterialTheme.typography.titleMedium)
            LabeledSlider("Масштаб обзора (zoom)", s.zoom, 0.3f..0.75f, "%.2f".format(s.zoom)) { update(s.copy(zoom = it)) }
            LabeledSlider("Отступы между окнами (gaps)", s.gapsDp, 0f..40f, "${s.gapsDp.roundToInt()} dp") {
                update(s.copy(gapsDp = it.roundToInt().toFloat()))
            }
            LabeledSlider("Расстояние между столами", s.workspaceGapDp, 4f..80f, "${s.workspaceGapDp.roundToInt()} dp") {
                update(s.copy(workspaceGapDp = it.roundToInt().toFloat()))
            }
            LabeledSlider("Скругление углов", s.cornerDp, 0f..40f, "${s.cornerDp.roundToInt()} dp") {
                update(s.copy(cornerDp = it.roundToInt().toFloat()))
            }
            LabeledSlider("Толщина фокус-кольца", s.ringWidthDp, 0f..10f, "${s.ringWidthDp.roundToInt()} dp") {
                update(s.copy(ringWidthDp = it.roundToInt().toFloat()))
            }
            LabeledSlider("Скорость анимаций", s.animSpeed, 0.5f..2f, "×%.1f".format(s.animSpeed)) {
                update(s.copy(animSpeed = (it * 10).roundToInt() / 10f))
            }
            ColorRow("Кольцо активного окна", RingColors, s.ringActive) { update(s.copy(ringActive = it)) }
            ColorRow("Кольцо на других столах", InactiveColors, s.ringInactive) { update(s.copy(ringInactive = it)) }
            ColorRow("Фон за столами (backdrop)", BackdropColors, s.backdrop) { update(s.copy(backdrop = it)) }
            ColorRow("Цвет стола без обоев", WorkspaceColors, s.workspaceColor) { update(s.copy(workspaceColor = it)) }
            CheckRow(
                s.wallpaper,
                "Ваши обои на рабочих столах",
                "Как в niri: стол — уменьшенный экран с обоями. Копируются через root при открытии этого приложения" +
                    (if (s.wallpaper && wpStatus != null) ".\n$wpStatus" else ""),
            ) {
                update(s.copy(wallpaper = it))
            }
            CheckRow(s.shadows, "Тени под столами", "workspace-shadow из niri") { update(s.copy(shadows = it)) }
            CheckRow(s.labels, "Подписи приложений", "Название под каждым окном") { update(s.copy(labels = it)) }
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                OutlinedButton(onClick = { update(NiriSettings(enabled = s.enabled, resetToken = s.resetToken)) }) {
                    Text("Как в niri")
                }
                OutlinedButton(onClick = { update(s.copy(resetToken = System.currentTimeMillis())) }) {
                    Text("Сбросить столы")
                }
            }
        }
    }
}

@Composable
private fun GesturesCard() {
    Card(Modifier.fillMaxWidth()) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Text("Управление", style = MaterialTheme.typography.titleMedium)
            val rows = listOf(
                "Нажать на окно" to "открыть (обзор приближается к нему, как в niri)",
                "Листать вбок" to "лента окон текущего стола",
                "Листать вверх/вниз" to "переход между рабочими столами",
                "Смахнуть окно вверх" to "закрыть приложение",
                "Зажать окно и перетащить" to "на другой стол, между столами — новый стол",
                "Нажать на пустое место" to "на главный экран",
                "Назад / жест домой" to "как обычно",
            )
            rows.forEach { (k, v) ->
                Row {
                    Text(k, Modifier.width(150.dp), style = MaterialTheme.typography.bodySmall, fontWeight = FontWeight.Medium)
                    Text(v, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
                }
            }
            Text(
                "Новые приложения открываются на том столе, где вы были последним, а внизу всегда есть пустой стол — как в niri.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.padding(top = 6.dp),
            )
        }
    }
}

@Composable
private fun DiagCard(lines: List<String>, onCopy: () -> Unit, onClear: () -> Unit) {
    Card(Modifier.fillMaxWidth()) {
        Column(Modifier.padding(16.dp)) {
            Text("Диагностика лаунчера", style = MaterialTheme.typography.titleMedium)
            Text(
                "Сюда пишет модуль из лаунчера. Если что-то не работает — скопируйте и пришлите.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
            Spacer(Modifier.height(8.dp))
            if (lines.isEmpty()) {
                Text("Пока пусто", style = MaterialTheme.typography.bodySmall)
            } else {
                lines.takeLast(25).forEach {
                    Text(it, fontFamily = FontFamily.Monospace, fontSize = 11.sp, lineHeight = 14.sp)
                }
            }
            Row {
                TextButton(onClick = onCopy) { Text("Копировать") }
                TextButton(onClick = onClear) { Text("Очистить") }
            }
        }
    }
}

@Composable
private fun LabeledSlider(
    title: String,
    value: Float,
    range: ClosedFloatingPointRange<Float>,
    shown: String,
    onChange: (Float) -> Unit,
) {
    Column(Modifier.fillMaxWidth().padding(top = 4.dp)) {
        Row(Modifier.fillMaxWidth()) {
            Text(title, Modifier.weight(1f), style = MaterialTheme.typography.bodyMedium)
            Text(shown, style = MaterialTheme.typography.bodyMedium, color = MaterialTheme.colorScheme.primary)
        }
        Slider(value = value.coerceIn(range.start, range.endInclusive), onValueChange = onChange, valueRange = range)
    }
}

@Composable
private fun ColorRow(title: String, colors: List<Long>, selected: Int, onPick: (Int) -> Unit) {
    Column(Modifier.fillMaxWidth().padding(vertical = 6.dp)) {
        Text(title, style = MaterialTheme.typography.bodyMedium)
        Spacer(Modifier.height(6.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            colors.forEach { c ->
                val argb = c.toInt()
                val isSel = argb == selected
                Box(
                    Modifier
                        .size(32.dp)
                        .clip(CircleShape)
                        .background(if (argb ushr 24 == 0) Color(0xFF1A1A1A) else Color(argb))
                        .border(
                            if (isSel) 3.dp else 1.dp,
                            if (isSel) Color.White else Color(0xFF555555),
                            CircleShape,
                        )
                        .clickable { onPick(argb) },
                    contentAlignment = Alignment.Center,
                ) {
                    if (argb ushr 24 == 0) Text("—", color = Color.Gray)
                }
            }
        }
    }
}

@Composable
private fun CheckRow(checked: Boolean, title: String, subtitle: String? = null, onChange: (Boolean) -> Unit) {
    Row(
        Modifier.fillMaxWidth().clickable { onChange(!checked) }.padding(vertical = 2.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Checkbox(checked = checked, onCheckedChange = onChange)
        Column(Modifier.weight(1f)) {
            Text(title, style = MaterialTheme.typography.bodyLarge)
            if (subtitle != null) {
                Text(subtitle, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
            }
        }
    }
}
