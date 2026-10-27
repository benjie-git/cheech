package com.octopilot.cheech.model

import java.net.URI
import java.net.URLDecoder

data class InviteTarget(val host: String, val port: Int)

object InviteLink {
    private const val DEFAULT_PORT = 3838

    fun parse(raw: String): InviteTarget? {
        val uri = try {
            URI(raw.trim())
        } catch (_: Exception) {
            return null
        }
        if (!uri.scheme.equals("cheech", ignoreCase = true)) return null

        if (uri.host.equals("join", ignoreCase = true)) {
            val query = parseQuery(uri.rawQuery)
            val host = query["host"]?.trim().orEmpty()
            if (host.isEmpty()) return null
            return InviteTarget(host, validPort(query["port"]?.toIntOrNull()))
        }

        val host = uri.host?.trim().orEmpty()
        if (host.isEmpty()) return null
        return InviteTarget(host, validPort(uri.port))
    }

    private fun parseQuery(raw: String?): Map<String, String> {
        if (raw.isNullOrEmpty()) return emptyMap()
        return raw.split('&').mapNotNull { part ->
            val separator = part.indexOf('=')
            if (separator < 0) return@mapNotNull null
            val key = decode(part.substring(0, separator))
            val value = decode(part.substring(separator + 1))
            key to value
        }.toMap()
    }

    private fun decode(value: String): String =
        try {
            URLDecoder.decode(value, "UTF-8")
        } catch (_: Exception) {
            value
        }

    private fun validPort(port: Int?): Int =
        if (port != null && port in 1..65535) port else DEFAULT_PORT
}
