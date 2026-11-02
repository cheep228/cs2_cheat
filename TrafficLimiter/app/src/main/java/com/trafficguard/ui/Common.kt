package com.trafficguard.ui

import android.content.Context
import android.util.LruCache
import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Checkbox
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.core.graphics.drawable.toBitmap
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import com.trafficguard.data.bytesToMbText
import com.trafficguard.data.fmtBytes
import com.trafficguard.data.mbTextToBytes
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/** Increments every time the screen comes back (e.g. from a settings page). */
@Composable
fun rememberResumeTick(): Int {
    val owner = LocalLifecycleOwner.current
    var tick by remember { mutableIntStateOf(0) }
    DisposableEffect(owner) {
        val obs = LifecycleEventObserver { _, e -> if (e == Lifecycle.Event.ON_RESUME) tick++ }
        owner.lifecycle.addObserver(obs)
        onDispose { owner.lifecycle.removeObserver(obs) }
    }
    return tick
}

@Composable
fun CheckRow(
    checked: Boolean,
    title: String,
    subtitle: String? = null,
    enabled: Boolean = true,
    onChange: (Boolean) -> Unit,
) {
    Row(
        Modifier
            .fillMaxWidth()
            .clickable(enabled = enabled) { onChange(!checked) }
            .padding(vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Checkbox(checked = checked, onCheckedChange = onChange, enabled = enabled)
        Spacer(Modifier.width(4.dp))
        Column(Modifier.weight(1f)) {
            Text(title, style = MaterialTheme.typography.titleMedium)
            if (subtitle != null) {
                Text(subtitle, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
            }
        }
    }
}

@Composable
fun QuotaRow(label: String, used: Long, limit: Long) {
    Column(Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
        Row(Modifier.fillMaxWidth()) {
            Text(label, Modifier.weight(1f), style = MaterialTheme.typography.bodyMedium)
            Text(
                if (limit > 0) "${fmtBytes(used)} из ${fmtBytes(limit)}" else "${fmtBytes(used)} · без лимита",
                style = MaterialTheme.typography.bodyMedium,
                color = if (limit in 1..used) MaterialTheme.colorScheme.error else MaterialTheme.colorScheme.onSurface,
            )
        }
        if (limit > 0) {
            Spacer(Modifier.height(4.dp))
            LinearProgressIndicator(
                progress = { (used.toFloat() / limit).coerceIn(0f, 1f) },
                modifier = Modifier.fillMaxWidth(),
                color = if (used >= limit) MaterialTheme.colorScheme.error else MaterialTheme.colorScheme.primary,
            )
        }
    }
}

/** Megabyte input; empty means "no limit". Writes back only valid values. */
@Composable
fun MbField(label: String, bytes: Long, key: Any, modifier: Modifier = Modifier, onBytes: (Long) -> Unit) {
    var text by remember(key) { mutableStateOf(bytesToMbText(bytes)) }
    val valid = mbTextToBytes(text) != null
    OutlinedTextField(
        value = text,
        onValueChange = { t ->
            text = t
            mbTextToBytes(t)?.let(onBytes)
        },
        label = { Text(label) },
        placeholder = { Text("без лимита") },
        suffix = { Text("МБ") },
        isError = !valid,
        singleLine = true,
        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Decimal),
        modifier = modifier.fillMaxWidth(),
    )
}

object IconCache {
    private val cache = LruCache<String, ImageBitmap>(400)
    fun peek(pkg: String): ImageBitmap? = cache.get(pkg)
    fun load(ctx: Context, pkg: String): ImageBitmap? = try {
        ctx.packageManager.getApplicationIcon(pkg).toBitmap(96, 96).asImageBitmap().also { cache.put(pkg, it) }
    } catch (e: Exception) {
        null
    }
}

@Composable
fun AppIcon(pkg: String, size: Dp = 40.dp) {
    val ctx = LocalContext.current
    var bmp by remember(pkg) { mutableStateOf(IconCache.peek(pkg)) }
    LaunchedEffect(pkg) {
        if (bmp == null) bmp = withContext(Dispatchers.IO) { IconCache.load(ctx, pkg) }
    }
    val b = bmp
    if (b != null) Image(b, contentDescription = null, modifier = Modifier.size(size))
    else Box(Modifier.size(size))
}
