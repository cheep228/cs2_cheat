package com.trafficguard.data

enum class NetKind { WIFI, MOBILE, OTHER, NONE }

data class CurrentNet(val kind: NetKind, val ssid: String? = null)

enum class Quota { MINUTE, HOUR, DAY, TOTAL }

/** What the VPN layer has to do. Packages are routed into the VPN and dropped. */
sealed interface BlockPlan {
    data object None : BlockPlan

    /** Block exactly these apps. */
    data class Only(val packages: Set<String>) : BlockPlan

    /** Block every app on the device except these. */
    data class AllExcept(val packages: Set<String>) : BlockPlan
}

data class Decision(
    val plan: BlockPlan,
    /** The quota that is used up, if any; that is what triggered a full block. */
    val exceeded: Quota? = null,
    /** Apps blocked by their own rule or per-app daily limit. */
    val blockedApps: Set<String> = emptySet(),
    val note: String = "",
)

object ProfileMatcher {
    /**
     * Most specific profile wins: Wi-Fi with this exact name, then "any Wi-Fi",
     * then mobile, then "any network".
     */
    fun match(profiles: List<Profile>, net: CurrentNet): Profile? {
        if (net.kind == NetKind.NONE) return null
        if (net.kind == NetKind.WIFI) {
            val ssid = net.ssid?.trim().orEmpty()
            if (ssid.isNotEmpty()) {
                profiles.firstOrNull { it.type == NetType.WIFI && it.ssid.trim() == ssid }
                    ?.let { return it }
            }
            profiles.firstOrNull { it.type == NetType.WIFI && it.ssid.isBlank() }?.let { return it }
        }
        if (net.kind == NetKind.MOBILE) {
            profiles.firstOrNull { it.type == NetType.MOBILE }?.let { return it }
        }
        return profiles.firstOrNull { it.type == NetType.ANY }
    }
}

object Engine {

    /** The longest-lasting exhausted quota, so the user sees the real reason. */
    fun exceeded(p: Profile, u: Usage): Quota? = when {
        p.maxTotal > 0 && u.total >= p.maxTotal -> Quota.TOTAL
        p.perDay > 0 && u.day >= p.perDay -> Quota.DAY
        p.perHour > 0 && u.hour >= p.perHour -> Quota.HOUR
        p.perMinute > 0 && u.minute >= p.perMinute -> Quota.MINUTE
        else -> null
    }

    /**
     * @param appUsageToday bytes per package used today on the current network type
     * @param foreground package on screen, or null when the screen is off / locked
     * @param foregroundKnown false when usage access is missing and [foreground] means nothing
     */
    fun decide(
        limitingEnabled: Boolean,
        profile: Profile?,
        usage: Usage?,
        appUsageToday: Map<String, Long>,
        foreground: String?,
        foregroundKnown: Boolean,
        ownPackage: String,
        now: Long,
    ): Decision {
        if (!limitingEnabled) return Decision(BlockPlan.None, note = "Ограничение выключено главным переключателем")
        if (profile == null) return Decision(BlockPlan.None, note = "Для этой сети нет профиля")
        if (!profile.limitEnabled) return Decision(BlockPlan.None, note = "В этой сети ограничение не включено")

        val allowed = profile.apps.filterValues { it.mode == AppMode.ALLOW }.keys + ownPackage

        val quota = usage?.let { exceeded(profile, it.rolled(now)) }
        if (quota != null) {
            return Decision(BlockPlan.AllExcept(allowed), exceeded = quota)
        }

        val blocked = profile.apps.filter { (pkg, r) ->
            pkg != ownPackage && when (r.mode) {
                AppMode.BLOCK -> true
                AppMode.ALLOW -> false
                AppMode.NORMAL -> r.dailyLimit > 0 && (appUsageToday[pkg] ?: 0L) >= r.dailyLimit
            }
        }.keys

        if (profile.blockBackground) {
            if (foregroundKnown) {
                val pass = if (foreground != null && foreground !in blocked) allowed + foreground else allowed
                return Decision(BlockPlan.AllExcept(pass), blockedApps = blocked)
            }
            val plan = if (blocked.isEmpty()) BlockPlan.None else BlockPlan.Only(blocked)
            return Decision(plan, blockedApps = blocked, note = "Блокировка фона не работает: нет доступа к истории использования")
        }

        return if (blocked.isEmpty()) Decision(BlockPlan.None, note = "Трафик не ограничен")
        else Decision(BlockPlan.Only(blocked), blockedApps = blocked)
    }

    fun untilReset(q: Quota, now: Long): Long? = when (q) {
        Quota.MINUTE -> TimeKeys.untilNext(TimeKeys.MINUTE, now)
        Quota.HOUR -> TimeKeys.untilNext(TimeKeys.HOUR, now)
        Quota.DAY -> TimeKeys.untilNext(TimeKeys.DAY, now)
        Quota.TOTAL -> null
    }
}
