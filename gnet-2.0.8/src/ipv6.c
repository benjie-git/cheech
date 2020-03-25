/* GNet - Networking library
 * Copyright (C) 2002  David Helder
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#include "ipv6.h"

G_LOCK_DEFINE_STATIC (ipv6);
static GIPv6Policy   ipv6_policy = GIPV6_POLICY_IPV4_ONLY;


/**
 *  gnet_ipv6_get_policy
 *
 *  Gets the IPv6 policy.
 *
 *  Returns: IPv6 policy.
 **/
GIPv6Policy
gnet_ipv6_get_policy (void)
{
  GIPv6Policy policy;

  G_LOCK (ipv6);

  policy = ipv6_policy;

  G_UNLOCK (ipv6);

  return policy;
}


/**
 *  gnet_ipv6_set_policy
 *  @policy: IPv6 policy
 *
 *  Sets the IPv6 policy.
 *
 **/
void
gnet_ipv6_set_policy (GIPv6Policy policy)
{
  G_LOCK (ipv6);

  ipv6_policy = policy;

  G_UNLOCK (ipv6);
}

