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
import androidx.compose.foundation.layout.BoxWithConstraints
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
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import android.content.pm.PackageManager
import android.provider.Settings
import androidx.compose.material3.dynamicDarkColorScheme
import com.niri.recents.overview.Md3Palette
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
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
        setContent { App() }
    }
}

@Composable
private fun App() {
    val ctx = LocalContext.current
    var s by remember { mutableStateOf(SettingsStore.load(ctx)) }
    val tick = rememberResumeTick()
    // The launcher may have changed the window size (pinch) while we were away.
    LaunchedEffect(tick) { s = SettingsStore.load(ctx) }
    val update: (NiriSettings) -> Unit = { n ->
        s = n
        SettingsStore.save(ctx, n)
    }
    NiriTheme(md3 = s.md3) { SettingsScreen(s, update, tick) }
}

private val NiriBlue = Color(0xFF7FC8FF)

/** niri's dark look, or with MD3 on, the system's Material You scheme. */
@Composable
fun NiriTheme(md3: Boolean = false, content: @Composable () -> Unit) {
    val ctx = LocalContext.current
    val scheme = if (md3) {
        dynamicDarkColorScheme(ctx)
    } else {
        darkColorScheme(
            primary = NiriBlue,
            onPrimary = Color(0xFF00283D),
            secondary = Color(0xFFFFC87F),
            background = Color(0xFF1A1A1A),
            surface = Color(0xFF1A1A1A),
            surfaceVariant = Color(0xFF262626),
            surfaceContainer = Color(0xFF262626),
            surfaceContainerHighest = Color(0xFF2E2E2E),
        )
    }
    MaterialTheme(colorScheme = scheme, content = content)
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
fun SettingsScreen(s: NiriSettings, update: (NiriSettings) -> Unit, tick: Int) {
    val ctx = LocalContext.current
    val scope = rememberCoroutineScope()
    var diagVersion by remember { mutableIntStateOf(0) }
    var freeformVersion by remember { mutableIntStateOf(0) }
    val freeform = remember(tick, freeformVersion) { freeformEnabled(ctx) }
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
            item {
                ModesCard(
                    s, update, freeform,
                    onEnableFreeform = {
                        scope.launch {
                            val (ok, out) = withContext(Dispatchers.IO) {
                                RootShell.run(
                                    "settings put global enable_freeform_support 1; " +
                                        "settings put global force_resizable_activities 1",
                                )
                            }
                            Toast.makeText(
                                ctx,
                                if (ok) "Включено. Перезагрузите телефон, чтобы система подхватила" else "Не вышло: $out",
                                Toast.LENGTH_LONG,
                            ).show()
                            freeformVersion++
                        }
                    },
                    onReboot = {
                        scope.launch { withContext(Dispatchers.IO) { RootShell.run("svc power reboot || reboot") } }
                    },
                )
            }
            item { PreviewCard(s, wpBitmap, update) }
            item { LookCard(s, wpStatus, update) }
            item { GesturesCard(s.liveWindows) }
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
private fun PreviewCard(s: NiriSettings, wallpaper: android.graphics.Bitmap?, update: (NiriSettings) -> Unit) {
    val ctx = LocalContext.current
    val dm = ctx.resources.displayMetrics
    val aspect = dm.widthPixels.toFloat() / dm.heightPixels.coerceAtLeast(1)
    val latest by rememberUpdatedState(s)
    val latestUpdate by rememberUpdatedState(update)
    val palette = remember(s.md3) { Md3Palette.system(ctx) }
    Card(Modifier.fillMaxWidth()) {
        Column(Modifier.padding(16.dp)) {
            Text("Предпросмотр", style = MaterialTheme.typography.titleMedium)
            Text(
                "Уменьшенная копия экрана. Можно трогать: листайте, смахивайте окно вверх, зажмите окно и " +
                    "перетащите на другой стол, двумя пальцами меняйте размер окон.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
            Spacer(Modifier.height(12.dp))
            BoxWithConstraints(Modifier.fillMaxWidth(), contentAlignment = Alignment.Center) {
                val frameWidth = maxWidth * 0.8f
                // Everything inside is the real screen scaled down, gaps and text included.
                val scale = with(LocalDensity.current) { frameWidth.toPx() } / dm.widthPixels.coerceAtLeast(1)
                val density = dm.density * scale
                Box(
                    Modifier
                        .width(frameWidth)
                        .aspectRatio(aspect)
                        .clip(RoundedCornerShape(24.dp))
                        .border(1.5.dp, Color(0xFF3A3A3A), RoundedCornerShape(24.dp)),
                ) {
                    AndroidView(
                        modifier = Modifier.fillMaxSize(),
                        factory = { c ->
                            buildPreview(c, OverviewStyle(s, density, palette)) { z -> latestUpdate(latest.copy(zoom = z)) }
                        },
                        update = { v ->
                            v.style = OverviewStyle(s, density, palette)
                            v.wallpaper = wallpaper
                        },
                    )
                }
            }
        }
    }
}

private fun buildPreview(c: android.content.Context, style: OverviewStyle, onZoom: (Float) -> Unit): NiriOverviewView {
    val v = NiriOverviewView(c)
    v.respectSystemGestures = false
    v.style = style
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
        override fun onZoomChanged(zoom: Float) = onZoom(zoom)
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
            LabeledSlider("Размер окон (zoom, или щипок в обзоре)", s.zoom, 0.2f..0.6f, "${(s.zoom * 100).roundToInt()}% экрана") {
                update(s.copy(zoom = (it * 100).roundToInt() / 100f))
            }
            LabeledSlider("Отступы между окнами (gaps)", s.gapsDp, 4f..32f, "${s.gapsDp.roundToInt()} dp") {
                update(s.copy(gapsDp = it.roundToInt().toFloat()))
            }
            LabeledSlider("Расстояние между столами", s.workspaceGapDp, 4f..48f, "${s.workspaceGapDp.roundToInt()} dp") {
                update(s.copy(workspaceGapDp = it.roundToInt().toFloat()))
            }
            if (!s.md3) {
                LabeledSlider("Скругление углов окон", s.cornerDp, 0f..32f, "${s.cornerDp.roundToInt()} dp") {
                    update(s.copy(cornerDp = it.roundToInt().toFloat()))
                }
            }
            LabeledSlider("Толщина фокус-кольца", s.ringWidthDp, 0f..8f, "${s.ringWidthDp.roundToInt()} dp") {
                update(s.copy(ringWidthDp = it.roundToInt().toFloat()))
            }
            LabeledSlider("Скорость анимаций", s.animSpeed, 0.5f..2f, "×%.1f".format(s.animSpeed)) {
                update(s.copy(animSpeed = (it * 10).roundToInt() / 10f))
            }
            if (s.md3) {
                Text(
                    "Material Design 3: цвета и скругления берутся из системы (Material You, под ваши обои).",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.padding(vertical = 6.dp),
                )
            } else {
                ColorRow("Кольцо активного окна", RingColors, s.ringActive) { update(s.copy(ringActive = it)) }
                ColorRow("Кольцо на других столах", InactiveColors, s.ringInactive) { update(s.copy(ringInactive = it)) }
                ColorRow("Фон за столами (backdrop)", BackdropColors, s.backdrop) { update(s.copy(backdrop = it)) }
                ColorRow("Цвет стола без обоев", WorkspaceColors, s.workspaceColor) { update(s.copy(workspaceColor = it)) }
            }
            CheckRow(
                s.wallpaper,
                "Ваши обои на рабочих столах",
                "Как в niri: под окнами каждого стола ваши обои. Копируются через root при открытии этого приложения" +
                    (if (s.wallpaper && wpStatus != null) ".\n$wpStatus" else ""),
            ) {
                update(s.copy(wallpaper = it))
            }
            CheckRow(s.shadows, "Тени под столами", "workspace-shadow из niri") { update(s.copy(shadows = it)) }
            CheckRow(s.labels, "Подписи приложений", "Иконка и название под каждым окном") { update(s.copy(labels = it)) }
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                OutlinedButton(onClick = {
                    update(
                        NiriSettings(
                            enabled = s.enabled, resetToken = s.resetToken, md3 = s.md3, tiling = s.tiling,
                            perWorkspace = s.perWorkspace, columnWidth = s.columnWidth, hypr = s.hypr,
                        ),
                    )
                }) {
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
private fun ModesCard(
    s: NiriSettings,
    update: (NiriSettings) -> Unit,
    freeform: Boolean,
    onEnableFreeform: () -> Unit,
    onReboot: () -> Unit,
) {
    Card(Modifier.fillMaxWidth()) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Text("Режимы", style = MaterialTheme.typography.titleMedium)
            CheckRow(
                s.md3,
                "Material Design 3",
                "Цвета из системы (Material You, под обои), скругления и подписи-чипы как в Material 3. " +
                    "Это приложение тоже перекрашивается.",
            ) { update(s.copy(md3 = it)) }
            CheckRow(
                s.tiling,
                "Как в niri: живые окна (эксперимент)",
                "Нажали на окно в обзоре — его стол открывается живыми окнами колонками рядом, " +
                    "ими можно пользоваться. Снизу панель: колонки, ширина, закрыть; свайп по панели " +
                    "вверх/вниз — другой стол; «Назад» — в обзор.",
            ) { update(s.copy(tiling = it, hypr = if (it) false else s.hypr)) }
            CheckRow(
                s.hypr,
                "Как в Hyprland: живые окна (эксперимент)",
                "Окна стола сами делят экран (dwindle): одно — весь экран, два — пополам, дальше " +
                    "спиралью. Рамки с градиентом, отступы, сверху Waybar: столы 1-2-3, окно в фокусе, " +
                    "часы, батарея. Нажмите на окно в Waybar — меню (togglesplit, поменять, на другой стол).",
            ) { update(s.copy(hypr = it, tiling = if (it) false else s.tiling)) }
            if (s.liveWindows) {
                Text(
                    if (freeform) "Плавающие окна Android: включены" else "Плавающие окна Android выключены, без них режим не работает",
                    style = MaterialTheme.typography.bodySmall,
                    color = if (freeform) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.error,
                    modifier = Modifier.padding(start = 12.dp),
                )
                if (!freeform) {
                    Row(Modifier.padding(start = 12.dp), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        Button(onClick = onEnableFreeform) { Text("Включить (root)") }
                        OutlinedButton(onClick = onReboot) { Text("Перезагрузить") }
                    }
                }
            }
            if (s.tiling) {
                Text("Ширина новой колонки", style = MaterialTheme.typography.bodyMedium, modifier = Modifier.padding(top = 8.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    listOf("⅓" to 1f / 3f, "½" to 0.5f, "⅔" to 2f / 3f, "1" to 1f).forEach { (label, w) ->
                        val sel = kotlin.math.abs(s.columnWidth - w) < 0.02f
                        if (sel) {
                            Button(onClick = {}) { Text(label) }
                        } else {
                            OutlinedButton(onClick = { update(s.copy(columnWidth = w)) }) { Text(label) }
                        }
                    }
                }
            }
            LabeledSlider(
                "Окон на столе, потом новый стол",
                s.perWorkspace.toFloat(),
                0f..6f,
                if (s.perWorkspace == 0) "без предела, как в niri" else "${s.perWorkspace}",
            ) { update(s.copy(perWorkspace = it.roundToInt())) }
            Text(
                "Новые приложения встают в ряд, а когда на столе столько окон, следующее открывает новый стол ниже.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
        }
    }
}

/** Freeform windows: declared by the device, or the developer option. */
private fun freeformEnabled(ctx: android.content.Context): Boolean =
    ctx.packageManager.hasSystemFeature(PackageManager.FEATURE_FREEFORM_WINDOW_MANAGEMENT) ||
        Settings.Global.getInt(ctx.contentResolver, "enable_freeform_support", 0) != 0

@Composable
private fun GesturesCard(tiling: Boolean) {
    Card(Modifier.fillMaxWidth()) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Text("Управление", style = MaterialTheme.typography.titleMedium)
            val rows = listOf(
                "Нажать на окно" to if (tiling) "открыть его стол живыми окнами рядом" else "открыть (обзор приближается к нему, как в niri)",
                "Листать вбок" to "лента окон текущего стола",
                "Листать вверх/вниз" to "переход между рабочими столами",
                "Щипок двумя пальцами" to "окна крупнее или мельче (запоминается)",
                "Смахнуть окно вверх" to "закрыть приложение",
                "Зажать окно и перетащить" to "на другой стол, между столами — новый стол",
                "«Очистить всё» внизу" to "закрыть все приложения",
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
