package com.niri.recents

import com.niri.recents.overview.ColumnKeyInfo
import com.niri.recents.overview.Spring
import com.niri.recents.overview.WorkspaceArranger
import com.niri.recents.overview.WsJson
import com.niri.recents.overview.WsState
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class ArrangerTest {
    private fun cols(vararg keys: Pair<String, Long>) = keys.map { ColumnKeyInfo(it.first, it.second) }

    @Test
    fun firstRunPutsEverythingInOneWorkspaceOldestFirst() {
        val a = WorkspaceArranger.arrange(cols("c" to 30, "a" to 10, "b" to 20), runningKey = "c", saved = null)
        assertEquals(1, a.workspaces.size)
        assertEquals(listOf("a", "b", "c"), a.workspaces[0].second)
        assertEquals(0, a.activeIndex)
        assertEquals("c", a.focusKey)
    }

    @Test
    fun savedPlacementSurvivesAndNewTasksJoinTheLastActiveWorkspace() {
        val saved = WsState(listOf(1, 2), mapOf(1 to listOf("a", "b"), 2 to listOf("c")), lastActive = 2, nextId = 3)
        val a = WorkspaceArranger.arrange(cols("a" to 1, "b" to 2, "c" to 3, "d" to 4), runningKey = "d", saved = saved)
        assertEquals(listOf(1 to listOf("a", "b"), 2 to listOf("c", "d")), a.workspaces)
        assertEquals(1, a.activeIndex)
        assertEquals("d", a.focusKey)
    }

    @Test
    fun niriNeverReordersWindowsWhenTheyAreUsed() {
        val saved = WsState(listOf(1), mapOf(1 to listOf("a", "b", "c")), lastActive = 1, nextId = 2)
        // "a" was used last, but keeps its place in the strip.
        val a = WorkspaceArranger.arrange(cols("a" to 99, "b" to 2, "c" to 3), runningKey = "a", saved = saved)
        assertEquals(listOf("a", "b", "c"), a.workspaces[0].second)
        assertEquals("a", a.focusKey)
    }

    @Test
    fun closedTasksAndEmptyWorkspacesDisappear() {
        val saved = WsState(listOf(1, 2, 3), mapOf(1 to listOf("a"), 2 to listOf("gone"), 3 to listOf("c")), 1, 4)
        val a = WorkspaceArranger.arrange(cols("a" to 1, "c" to 2), runningKey = null, saved = saved)
        assertEquals(listOf(1, 3), a.workspaces.map { it.first })
        assertEquals(listOf(1, 3), a.state.order)
    }

    @Test
    fun appOpenedFromTheEmptyWorkspaceMakesItReal() {
        // The user went to the empty workspace at the bottom (id 5) and opened an app from home.
        val saved = WsState(listOf(1), mapOf(1 to listOf("a")), lastActive = 5, nextId = 5)
        val a = WorkspaceArranger.arrange(cols("a" to 1, "new" to 2), runningKey = "new", saved = saved)
        assertEquals(listOf(1 to listOf("a"), 5 to listOf("new")), a.workspaces)
        assertEquals(1, a.activeIndex)
        assertTrue(a.state.nextId > 5)
    }

    @Test
    fun withoutRunningAppTheMostRecentWorkspaceIsActive() {
        val saved = WsState(listOf(1, 2), mapOf(1 to listOf("a"), 2 to listOf("b")), lastActive = 0, nextId = 3)
        val a = WorkspaceArranger.arrange(cols("a" to 5, "b" to 50), runningKey = null, saved = saved)
        assertEquals(1, a.activeIndex)
        assertEquals("b", a.focusKey)
    }

    @Test
    fun duplicatedKeysAreKeptOnce() {
        val saved = WsState(listOf(1, 2), mapOf(1 to listOf("a"), 2 to listOf("a", "b")), 1, 3)
        val a = WorkspaceArranger.arrange(cols("a" to 1, "b" to 2), runningKey = null, saved = saved)
        assertEquals(listOf(1 to listOf("a"), 2 to listOf("b")), a.workspaces)
    }

    @Test
    fun noTasks() {
        val a = WorkspaceArranger.arrange(emptyList(), runningKey = null, saved = null)
        assertTrue(a.workspaces.isEmpty())
        assertNull(a.focusKey)
    }

    @Test
    fun manyAppsFillSeveralWorkspaces() {
        val many = (1..8).map { ColumnKeyInfo("k$it", it.toLong()) }
        val a = WorkspaceArranger.arrange(many, runningKey = "k8", saved = null, perWorkspace = 3)
        assertEquals(
            listOf(listOf("k1", "k2", "k3"), listOf("k4", "k5", "k6"), listOf("k7", "k8")),
            a.workspaces.map { it.second },
        )
        assertEquals(2, a.activeIndex)
        assertEquals("k8", a.focusKey)
    }

    @Test
    fun aFullWorkspaceHandsOverToANewOneRightBelowIt() {
        val saved = WsState(listOf(1, 2), mapOf(1 to listOf("a", "b"), 2 to listOf("c")), lastActive = 1, nextId = 3)
        val a = WorkspaceArranger.arrange(
            cols("a" to 1, "b" to 2, "c" to 3, "d" to 4, "e" to 5), runningKey = "e", saved = saved, perWorkspace = 2,
        )
        assertEquals(listOf(1 to listOf("a", "b"), 3 to listOf("d", "e"), 2 to listOf("c")), a.workspaces)
        assertEquals(1, a.activeIndex)
    }

    @Test
    fun stateRoundTripsThroughJson() {
        val s = WsState(listOf(3, 1), mapOf(3 to listOf("12", "15+16"), 1 to listOf("7")), 3, 9)
        assertEquals(s, WsJson.decode(WsJson.encode(s)))
    }
}

class SpringTest {
    @Test
    fun criticallyDampedSpringSettlesWithoutOvershoot() {
        val s = Spring(0f, 800f, 1f, 0.01f)
        s.animateTo(100f)
        var t = 0f
        var max = 0f
        while (s.step(1f / 120f)) {
            t += 1f / 120f
            max = maxOf(max, s.value)
            assertTrue("did not settle", t < 2f)
        }
        assertEquals(100f, s.value, 0f)
        assertTrue("overshoot $max", max <= 100.01f)
        // niri-like stiffness 800 settles in a few hundred milliseconds.
        assertTrue("too slow: $t", t < 0.6f)
    }

    @Test
    fun snapStopsMotion() {
        val s = Spring(0f, 800f, 1f, 0.5f)
        s.animateTo(10f, 500f)
        s.snapTo(3f)
        assertFalse(s.isAnimating)
        assertFalse(s.step(0.016f))
        assertEquals(3f, s.value, 0f)
    }
}
