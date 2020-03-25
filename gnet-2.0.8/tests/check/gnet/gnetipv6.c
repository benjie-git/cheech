/* GNet IPv6 unit test
 * Copyright (C) 2002  David Helder
 * Copyright (C) 2007 Tim-Philipp Müller  <tim centricular net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "gnetcheck.h"

GNET_START_TEST (test_ipv6_policy)
{
  GIPv6Policy ipv6_policy;
  const gchar *policy_str = NULL;

  ipv6_policy = gnet_ipv6_get_policy();

  switch (ipv6_policy) {
    case GIPV6_POLICY_IPV4_THEN_IPV6:
      policy_str = "GIPV6_POLICY_IPV4_THEN_IPV6";
      break;
    case GIPV6_POLICY_IPV6_THEN_IPV4:
      policy_str = "GIPV6_POLICY_IPV6_THEN_IPV4";
      break;
    case GIPV6_POLICY_IPV4_ONLY:
      policy_str = "GIPV6_POLICY_IPV4_ONLY";
      break;
    case GIPV6_POLICY_IPV6_ONLY:
      policy_str = "GIPV6_POLICY_IPV6_ONLY";
      break;
    default:
      /* this aborts */
      g_error ("Invalid IPv6 policy %d", ipv6_policy);
      break;
  }

  g_print ("GNet IPv6 default policy: %s\n", policy_str);

  /* not that it will do anything, but just try to set all of these */
  gnet_ipv6_set_policy (GIPV6_POLICY_IPV4_THEN_IPV6);
  gnet_ipv6_set_policy (GIPV6_POLICY_IPV6_THEN_IPV4);
  gnet_ipv6_set_policy (GIPV6_POLICY_IPV4_ONLY);
  gnet_ipv6_set_policy (GIPV6_POLICY_IPV6_ONLY);
  gnet_ipv6_set_policy (ipv6_policy);
}

GNET_END_TEST;

static Suite *
gnetipv6_suite (void)
{
  Suite *s = suite_create ("GNetIPv6");
  TCase *tc_chain = tcase_create ("ipv6");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_ipv6_policy);
  /* TODO: more tests */
  return s;
}

GNET_CHECK_MAIN (gnetipv6);

