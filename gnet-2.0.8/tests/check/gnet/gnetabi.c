/* GNet unit test for ABI compatibility
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

typedef struct {
  char *name;
  int size;
  int abi_size;
} GNetCheckABIStruct;

#ifdef HAVE_CPU_X86_64
#include "struct_x86_64.h"
#define HAVE_ABI_SIZES TRUE
#else
#ifdef __powerpc__
#include "struct_ppc32.h"
#define HAVE_ABI_SIZES TRUE
/*
#else
#ifdef HAVE_CPU_I386
#include "struct_i386.h"
#define HAVE_ABI_SIZES TRUE
#else
#ifdef __powerpc64__
#include "struct_ppc64.h"
#define HAVE_ABI_SIZES TRUE
#else
#ifdef HAVE_CPU_HPPA
#include "struct_hppa.h"
#define HAVE_ABI_SIZES TRUE
*/
#else
/* in case someone wants to generate a new arch */
#include "struct_x86_64.h"
#define HAVE_ABI_SIZES FALSE
#endif
#endif
/*
#endif
#endif
#endif
*/

static void
gnet_check_abi_list (GNetCheckABIStruct list[], gboolean have_abi_sizes)
{
  if (have_abi_sizes) {
    gboolean ok = TRUE;
    gint i;

    for (i = 0; list[i].name; i++) {
      if (list[i].size != list[i].abi_size) {
        ok = FALSE;
        g_print ("sizeof(%s) is %d, expected %d\n",
            list[i].name, list[i].size, list[i].abi_size);
      }
    }
    fail_unless (ok, "failed ABI check");
  } else {
    const gchar *fn;

    if ((fn = g_getenv ("GNET_ABI"))) {
      GError *err = NULL;
      GString *s;
      gint i;

      s = g_string_new ("\nGNetCheckABIStruct list[] = {\n");
      for (i = 0; list[i].name; i++) {
        g_string_append_printf (s, "  {\"%s\", sizeof (%s), %d},\n",
            list[i].name, list[i].name, list[i].size);
      }
      g_string_append (s, "  {NULL, 0, 0}\n");
      g_string_append (s, "};\n");
      if (!g_file_set_contents (fn, s->str, s->len, &err)) {
        g_print ("%s", s->str);
        g_printerr ("\nFailed to write ABI information: %s\n", err->message);
      } else {
        g_print ("\nWrote ABI information to '%s'.\n", fn);
      }
      g_string_free (s, TRUE);
    } else {
      g_print ("No structure size list was generated for this architecture.\n");
      g_print ("Run with GNET_ABI environment variable set to output header.\n");
    }
  }
}

GNET_START_TEST (test_ABI)
{
  gnet_check_abi_list (list, HAVE_ABI_SIZES);
}

GNET_END_TEST;

static Suite *
gnetabi_suite (void)
{
  Suite *s = suite_create ("GNetABI");
  TCase *tc_chain = tcase_create ("size check");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_ABI);
  return s;
}

GNET_CHECK_MAIN (gnetabi);

