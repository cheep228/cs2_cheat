package com.niri.recents.overview

import kotlin.math.max

/** A recent-task column as far as workspace bookkeeping cares. */
data class ColumnKeyInfo(val key: String, val lastActive: Long)

/** What is persisted between overview openings. */
data class WsState(
    /** Workspace ids, top to bottom. */
    val order: List<Int>,
    /** Column keys of each workspace, left to right. */
    val members: Map<Int, List<String>>,
    /** Workspace the user was last in; new apps open there, like in niri. */
    val lastActive: Int,
    val nextId: Int,
)

data class Arrangement(
    /** Non-empty workspaces top to bottom: id to column keys left to right. */
    val workspaces: List<Pair<Int, List<String>>>,
    val activeIndex: Int,
    val focusKey: String?,
    val state: WsState,
)

/**
 * Turns the flat list of recent tasks into niri-style workspaces.
 *
 * - Columns keep the workspace and position the user gave them (niri never reorders windows).
 * - Tasks seen for the first time go to the end of the workspace the user was last in,
 *   the way a new niri window opens on the focused workspace. With [perWorkspace] > 0 a full
 *   workspace hands over to a new one below it, so a phone's many apps fill several rows.
 * - Empty workspaces disappear; the view adds the always-empty one at the bottom itself.
 */
object WorkspaceArranger {

    fun arrange(
        columns: List<ColumnKeyInfo>,
        runningKey: String?,
        saved: WsState?,
        perWorkspace: Int = 0,
    ): Arrangement {
        val byKey = columns.associateBy { it.key }
        var nextId = max(saved?.nextId ?: 1, (saved?.order?.maxOrNull() ?: 0) + 1)
        val order = ArrayList<Int>()
        val members = LinkedHashMap<Int, MutableList<String>>()
        val placed = HashSet<String>()

        saved?.order?.forEach { id ->
            if (members.containsKey(id)) return@forEach
            val keys = saved.members[id].orEmpty().filter { it in byKey && placed.add(it) }
            order += id
            members[id] = keys.toMutableList()
        }

        val unknown = columns.sortedBy { it.lastActive }.map { it.key }.filter { it !in placed }
        if (unknown.isNotEmpty()) {
            val wanted = saved?.lastActive
            var target = when {
                wanted != null && members.containsKey(wanted) -> wanted
                wanted != null && wanted > 0 -> {
                    // The user was on the empty workspace at the bottom: it becomes a real one.
                    order += wanted
                    members[wanted] = mutableListOf()
                    nextId = max(nextId, wanted + 1)
                    wanted
                }
                order.isNotEmpty() -> order.maxBy { id ->
                    members.getValue(id).maxOfOrNull { byKey.getValue(it).lastActive } ?: Long.MIN_VALUE
                }
                else -> {
                    val id = nextId++
                    order += id
                    members[id] = mutableListOf()
                    id
                }
            }
            for (key in unknown) {
                if (perWorkspace > 0 && members.getValue(target).size >= perWorkspace) {
                    // That workspace is full: the next app starts a new one right below it.
                    val id = nextId++
                    order.add(order.indexOf(target) + 1, id)
                    members[id] = mutableListOf()
                    target = id
                }
                members.getValue(target).add(key)
            }
        }

        order.removeAll { members[it].isNullOrEmpty() }
        members.keys.retainAll(order.toSet())

        if (order.isEmpty()) {
            return Arrangement(
                emptyList(), 0, null,
                WsState(emptyList(), emptyMap(), saved?.lastActive ?: 0, nextId),
            )
        }

        fun recency(id: Int) = members.getValue(id).maxOf { byKey.getValue(it).lastActive }

        val activeId = runningKey?.let { k -> order.firstOrNull { members.getValue(it).contains(k) } }
            ?: saved?.lastActive?.takeIf { members.containsKey(it) }
            ?: order.maxBy { recency(it) }
        val focusKey = runningKey?.takeIf { members.getValue(activeId).contains(it) }
            ?: members.getValue(activeId).maxBy { byKey.getValue(it).lastActive }

        return Arrangement(
            workspaces = order.map { it to members.getValue(it).toList() },
            activeIndex = order.indexOf(activeId),
            focusKey = focusKey,
            state = WsState(order.toList(), members.mapValues { it.value.toList() }, activeId, nextId),
        )
    }
}

/** [WsState] as JSON, for SharedPreferences. */
object WsJson {
    fun encode(s: WsState): String {
        val o = org.json.JSONObject()
        o.put("order", org.json.JSONArray(s.order))
        val m = org.json.JSONObject()
        for ((id, keys) in s.members) m.put(id.toString(), org.json.JSONArray(keys))
        o.put("members", m)
        o.put("last", s.lastActive)
        o.put("next", s.nextId)
        return o.toString()
    }

    fun decode(text: String): WsState {
        val o = org.json.JSONObject(text)
        val orderArr = o.optJSONArray("order") ?: org.json.JSONArray()
        val order = (0 until orderArr.length()).map { orderArr.getInt(it) }
        val m = o.optJSONObject("members") ?: org.json.JSONObject()
        val members = HashMap<Int, List<String>>()
        for (k in m.keys()) {
            val arr = m.getJSONArray(k)
            members[k.toInt()] = (0 until arr.length()).map { arr.getString(it) }
        }
        return WsState(order, members, o.optInt("last", 0), o.optInt("next", 1))
    }
}
