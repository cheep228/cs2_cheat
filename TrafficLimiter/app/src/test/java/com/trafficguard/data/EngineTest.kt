package com.trafficguard.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class EngineTest {
    private val own = "com.trafficguard"
    private val now = 1_700_000_000_000L

    private fun profile(
        perMinute: Long = 0, perHour: Long = 0, perDay: Long = 0, maxTotal: Long = 0,
        blockBackground: Boolean = false, apps: Map<String, AppRule> = emptyMap(), enabled: Boolean = true,
    ) = Profile("p", "p", NetType.MOBILE, limitEnabled = enabled, perMinute = perMinute, perHour = perHour,
        perDay = perDay, maxTotal = maxTotal, blockBackground = blockBackground, apps = apps)

    private fun decide(
        p: Profile?, u: Usage? = Usage().rolled(now), limiting: Boolean = true,
        appUsage: Map<String, Long> = emptyMap(), fg: String? = null, fgKnown: Boolean = true,
    ) = Engine.decide(limiting, p, u, appUsage, fg, fgKnown, own, now)

    @Test
    fun masterSwitchOffNeverBlocks() {
        val p = profile(perMinute = 1, apps = mapOf("x" to AppRule(AppMode.BLOCK)))
        assertEquals(BlockPlan.None, decide(p, Usage().plus(100, now), limiting = false).plan)
    }

    @Test
    fun profileCheckboxOffNeverBlocks() {
        val p = profile(perMinute = 1, enabled = false)
        assertEquals(BlockPlan.None, decide(p, Usage().plus(100, now)).plan)
    }

    @Test
    fun noProfileNoBlock() {
        assertEquals(BlockPlan.None, decide(null).plan)
    }

    @Test
    fun minuteQuotaBlocksEverythingExceptAllowed() {
        val p = profile(perMinute = 1000, apps = mapOf("tg" to AppRule(AppMode.ALLOW), "yt" to AppRule(AppMode.BLOCK)))
        val d = decide(p, Usage().plus(1000, now))
        assertEquals(Quota.MINUTE, d.exceeded)
        assertEquals(BlockPlan.AllExcept(setOf("tg", own)), d.plan)
    }

    @Test
    fun minuteQuotaClearsInNextMinute() {
        val p = profile(perMinute = 1000)
        val u = Usage().plus(5000, now - 120_000)
        assertNull(decide(p, u).exceeded)
    }

    @Test
    fun longestQuotaIsReported() {
        val p = profile(perMinute = 10, perHour = 10, perDay = 10)
        assertEquals(Quota.DAY, decide(p, Usage().plus(10, now)).exceeded)
        assertEquals(Quota.TOTAL, decide(profile(perMinute = 1, maxTotal = 1), Usage().plus(10, now)).exceeded)
    }

    @Test
    fun blockedAndOverLimitAppsOnly() {
        val p = profile(apps = mapOf(
            "yt" to AppRule(AppMode.BLOCK),
            "vk" to AppRule(AppMode.NORMAL, dailyLimit = 100),
            "tg" to AppRule(AppMode.NORMAL, dailyLimit = 100),
            "ok" to AppRule(AppMode.ALLOW),
        ))
        val d = decide(p, appUsage = mapOf("vk" to 150, "tg" to 50))
        assertEquals(BlockPlan.Only(setOf("yt", "vk")), d.plan)
    }

    @Test
    fun backgroundModeLetsOnlyForegroundAndAllowedThrough() {
        val p = profile(blockBackground = true, apps = mapOf("ok" to AppRule(AppMode.ALLOW), "yt" to AppRule(AppMode.BLOCK)))
        assertEquals(BlockPlan.AllExcept(setOf("ok", own, "browser")), decide(p, fg = "browser").plan)
        // A blocked app stays blocked even when opened.
        assertEquals(BlockPlan.AllExcept(setOf("ok", own)), decide(p, fg = "yt").plan)
        // Screen off: nothing in foreground.
        assertEquals(BlockPlan.AllExcept(setOf("ok", own)), decide(p, fg = null).plan)
    }

    @Test
    fun backgroundModeWithoutUsageAccessFallsBack() {
        val p = profile(blockBackground = true, apps = mapOf("yt" to AppRule(AppMode.BLOCK)))
        val d = decide(p, fg = null, fgKnown = false)
        assertEquals(BlockPlan.Only(setOf("yt")), d.plan)
        assertTrue(d.note.isNotEmpty())
    }

    @Test
    fun ownAppIsNeverBlocked() {
        val p = profile(apps = mapOf(own to AppRule(AppMode.BLOCK)))
        assertEquals(BlockPlan.None, decide(p).plan)
    }
}

class MatcherTest {
    private val anyWifi = Profile("1", "any wifi", NetType.WIFI)
    private val home = Profile("2", "home", NetType.WIFI, ssid = "Home")
    private val mobile = Profile("3", "mobile", NetType.MOBILE)
    private val any = Profile("4", "any", NetType.ANY)
    private val all = listOf(anyWifi, home, mobile, any)

    @Test
    fun exactSsidWins() = assertEquals(home, ProfileMatcher.match(all, CurrentNet(NetKind.WIFI, "Home")))

    @Test
    fun otherWifiFallsToAnyWifi() = assertEquals(anyWifi, ProfileMatcher.match(all, CurrentNet(NetKind.WIFI, "Cafe")))

    @Test
    fun unknownSsidFallsToAnyWifi() = assertEquals(anyWifi, ProfileMatcher.match(all, CurrentNet(NetKind.WIFI, null)))

    @Test
    fun mobile() = assertEquals(mobile, ProfileMatcher.match(all, CurrentNet(NetKind.MOBILE)))

    @Test
    fun ethernetFallsToAny() = assertEquals(any, ProfileMatcher.match(all, CurrentNet(NetKind.OTHER)))

    @Test
    fun offline() = assertNull(ProfileMatcher.match(all, CurrentNet(NetKind.NONE)))

    @Test
    fun onlySpecificWifiDoesNotMatchOthers() =
        assertNull(ProfileMatcher.match(listOf(home), CurrentNet(NetKind.WIFI, "Cafe")))
}

class UsageAndJsonTest {
    @Test
    fun windowsRollIndependently() {
        val t = 1_700_000_000_000L
        val base = t - Math.floorMod(TimeKeys.local(t), TimeKeys.DAY) + 10 * TimeKeys.HOUR // 10:00 local
        var u = Usage().plus(100, base)
        u = u.plus(50, base + 30_000)
        assertEquals(150, u.minute)
        u = u.plus(10, base + 61_000)
        assertEquals(10, u.minute)
        assertEquals(160, u.hour)
        u = u.plus(1, base + TimeKeys.HOUR)
        assertEquals(1, u.hour)
        assertEquals(161, u.day)
        u = u.plus(1, base + TimeKeys.DAY)
        assertEquals(1, u.day)
        assertEquals(162, u.total)
        assertEquals(u, Usage.fromJson(u.toJson()))
    }

    @Test
    fun settingsRoundTrip() {
        val s = Settings(
            limitingEnabled = true,
            monitorEnabled = false,
            useRoot = true,
            rootProbed = true,
            profiles = listOf(
                Profile("a", "Дом", NetType.WIFI, "Home", true, 1, 2, 3, 4, true,
                    mapOf("x" to AppRule(AppMode.BLOCK), "y" to AppRule(AppMode.NORMAL, 99))),
            ),
        )
        assertEquals(s, settingsFromJson(s.toJson()))
    }

    @Test
    fun mbParsing() {
        assertEquals(0L, mbTextToBytes(""))
        assertEquals(MB / 2, mbTextToBytes("0,5"))
        assertEquals(null, mbTextToBytes("abc"))
        assertEquals("1.5", bytesToMbText(MB + MB / 2))
    }
}
