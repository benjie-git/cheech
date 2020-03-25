/* GNet unit test
 * Copyright (C) 2007 Tim-Philipp Müller  <tim centricular net>
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
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include "gnetcheck.h"

#include <gnet.h>

GNET_START_TEST (test_versions)
{
  fail_unless (GNET_MAJOR_VERSION > 0);
  fail_unless (GNET_MINOR_VERSION >= 0);
  fail_unless (GNET_MICRO_VERSION >= 0);
  fail_unless (GNET_CHECK_VERSION (GNET_MAJOR_VERSION, GNET_MINOR_VERSION, GNET_MICRO_VERSION));
  fail_unless (GNET_CHECK_VERSION (GNET_MAJOR_VERSION - 1, GNET_MINOR_VERSION, GNET_MICRO_VERSION));
  if (GNET_MINOR_VERSION > 0) {
    fail_unless (GNET_CHECK_VERSION (GNET_MAJOR_VERSION, GNET_MINOR_VERSION - 1, GNET_MICRO_VERSION));
  }
  fail_unless (!GNET_CHECK_VERSION (GNET_MAJOR_VERSION, GNET_MINOR_VERSION, GNET_MICRO_VERSION + 1));
  fail_unless (!GNET_CHECK_VERSION (GNET_MAJOR_VERSION, GNET_MINOR_VERSION + 1, GNET_MICRO_VERSION));
  fail_unless (!GNET_CHECK_VERSION (GNET_MAJOR_VERSION + 1, GNET_MINOR_VERSION, GNET_MICRO_VERSION));

  fail_unless_equals_int (GNET_MAJOR_VERSION, gnet_major_version);
  fail_unless_equals_int (GNET_MINOR_VERSION, gnet_minor_version);
  fail_unless_equals_int (GNET_MICRO_VERSION, gnet_micro_version);
  fail_unless (GNET_CHECK_VERSION (gnet_major_version, gnet_minor_version, gnet_micro_version));
}

GNET_END_TEST;

static Suite *
gnetmisc_suite (void)
{
  Suite *s = suite_create ("GNetMisc");
  TCase *tc_chain = tcase_create ("init and version checks");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_versions);
  return s;
}

GNET_CHECK_MAIN (gnetmisc);

