package com.trafficguard.service

import android.content.Context
import android.content.pm.PackageManager
import android.os.Process
import android.util.Log
import com.trafficguard.data.BlockPlan
import java.util.concurrent.TimeUnit

/** Runs shell scripts as root through `su`. */
object RootShell {
    private const val TAG = "RootShell"

    data class Result(val ok: Boolean, val output: String)

    fun run(script: String, timeoutSec: Long = 30): Result = try {
        val p = ProcessBuilder("su").redirectErrorStream(true).start()
        p.outputStream.bufferedWriter().use { w ->
            w.write(script)
            w.write("\nexit \$?\n")
        }
        val out = StringBuilder()
        val reader = Thread { runCatching { out.append(p.inputStream.bufferedReader().readText()) } }
        reader.start()
        if (!p.waitFor(timeoutSec, TimeUnit.SECONDS)) {
            p.destroy()
            Result(false, "su не ответил вовремя")
        } else {
            reader.join(1_000)
            Result(p.exitValue() == 0, out.toString().trim())
        }
    } catch (e: Exception) {
        Log.w(TAG, "su failed", e)
        Result(false, e.message ?: "su недоступен")
    }

    /** True if `su` works and gives uid 0. May show the root manager's prompt. */
    fun available(): Boolean {
        val r = run("id", timeoutSec = 20)
        return r.ok && r.output.contains("uid=0")
    }
}

/**
 * Blocks apps with iptables owner rules in a dedicated chain hooked into OUTPUT,
 * for both IPv4 and IPv6. Blocked apps get an immediate reject instead of a timeout,
 * and no VPN slot is used, so the user can keep their own VPN.
 */
object RootFirewall {
    private const val CHAIN = "tlimit"

    /** Android system uids whose traffic is DNS on behalf of apps (netd, dns resolver). */
    private val DNS_UIDS = listOf(0, 1051)

    fun apply(ctx: Context, plan: BlockPlan): RootShell.Result {
        val script = StringBuilder()
        for (ipt in listOf("iptables", "ip6tables")) {
            script.append(setup(ipt))
            when (plan) {
                BlockPlan.None -> Unit
                is BlockPlan.Only -> uids(ctx, plan.packages).forEach { uid ->
                    script.append("$ipt -w -A $CHAIN -m owner --uid-owner $uid -j REJECT || exit 1\n")
                }
                is BlockPlan.AllExcept -> {
                    script.append("$ipt -w -A $CHAIN -o lo -j RETURN || exit 1\n")
                    // DNS for the apps that are still allowed.
                    for (proto in listOf("udp", "tcp")) {
                        script.append("$ipt -w -A $CHAIN -p $proto --dport 53 -j RETURN || exit 1\n")
                    }
                    (uids(ctx, plan.packages) + Process.myUid() + DNS_UIDS).distinct().forEach { uid ->
                        script.append("$ipt -w -A $CHAIN -m owner --uid-owner $uid -j RETURN || exit 1\n")
                    }
                    script.append("$ipt -w -A $CHAIN -j REJECT || exit 1\n")
                }
            }
        }
        return RootShell.run(script.toString())
    }

    /** Removes every rule this app added. */
    fun clear(): RootShell.Result {
        val script = StringBuilder()
        for (ipt in listOf("iptables", "ip6tables")) {
            script.append("while $ipt -w -D OUTPUT -j $CHAIN 2>/dev/null; do :; done\n")
            script.append("$ipt -w -F $CHAIN 2>/dev/null\n")
            script.append("$ipt -w -X $CHAIN 2>/dev/null\n")
        }
        script.append("true\n")
        return RootShell.run(script.toString())
    }

    private fun setup(ipt: String) = buildString {
        append("$ipt -w -N $CHAIN 2>/dev/null\n")
        append("$ipt -w -C OUTPUT -j $CHAIN 2>/dev/null || $ipt -w -I OUTPUT -j $CHAIN || exit 1\n")
        append("$ipt -w -F $CHAIN || exit 1\n")
    }

    private fun uids(ctx: Context, packages: Set<String>): List<Int> {
        val pm = ctx.packageManager
        return packages.mapNotNull { pkg ->
            try {
                pm.getApplicationInfo(pkg, 0).uid
            } catch (_: PackageManager.NameNotFoundException) {
                null
            }
        }.filter { it != Process.myUid() }.distinct()
    }
}
