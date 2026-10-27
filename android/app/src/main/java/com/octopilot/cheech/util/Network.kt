package com.octopilot.cheech.util

import java.net.Inet4Address
import java.net.NetworkInterface

fun localIPAddress(): String? {
    val interfaces = try {
        NetworkInterface.getNetworkInterfaces()?.toList().orEmpty()
    } catch (_: Exception) {
        return null
    }
    val preferred = interfaces.filter { it.isUp && !it.isLoopback }
        .sortedBy {
            when {
                it.name.startsWith("wlan") -> 0
                it.name == "en0" -> 1
                it.name.startsWith("eth") -> 2
                else -> 3
            }
        }
    for (network in preferred) {
        for (address in network.inetAddresses.toList()) {
            if (address is Inet4Address && !address.isLoopbackAddress && !address.isLinkLocalAddress) {
                return address.hostAddress
            }
        }
    }
    return null
}
