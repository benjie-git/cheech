/* Common code for GNet unittests (based on GStreamer's libgstcheck)
 *
 * Copyright (C) 2004,2006 Thomas Vander Stichele <thomas at apestaart dot org>
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

#include "gnetcheck.h"

#include <gnet.h>

/* GST_DEBUG_CATEGORY (check_debug); */

/* logging function for tests
 * a test uses g_message() to log a debug line
 * a gst unit test can be run with GNET_TEST_DEBUG env var set to see the
 * messages
 */

gboolean _gnet_check_threads_running = FALSE;
GList *thread_list = NULL;
GMutex *mutex;
GCond *start_cond;              /* used to notify main thread of thread startups */
GCond *sync_cond;               /* used to synchronize all threads and main thread */

gboolean _gnet_check_debug = FALSE;
gboolean _gnet_check_raised_critical = FALSE;
gboolean _gnet_check_raised_warning = FALSE;
gboolean _gnet_check_expecting_log = FALSE;

static void
gnet_check_log_message_func (const gchar * log_domain,
    GLogLevelFlags log_level, const gchar * message, gpointer user_data)
{
  if (_gnet_check_debug) {
    g_print ("%s", message);
  }
}

static void
gnet_check_log_critical_func (const gchar * log_domain,
    GLogLevelFlags log_level, const gchar * message, gpointer user_data)
{
  if (!_gnet_check_expecting_log) {
    g_print ("\n\nUnexpected critical/warning: %s\n", message);
    fail ("Unexpected critical/warning: %s", message);
  }

  if (_gnet_check_debug) {
    g_print ("\nExpected critical/warning: %s\n", message);
  }

  if (log_level & G_LOG_LEVEL_CRITICAL)
    _gnet_check_raised_critical = TRUE;
  if (log_level & G_LOG_LEVEL_WARNING)
    _gnet_check_raised_warning = TRUE;
}

/* initialize GNet testing */
static void
gnet_check_init (int *argc, char **argv[])
{
  gnet_init ();

  /* GST_DEBUG_CATEGORY_INIT (check_debug, "check", 0, "check regression tests"); */

  if (g_getenv ("GNET_TEST_DEBUG"))
    _gnet_check_debug = TRUE;

  if (g_getenv ("SOCKS_SERVER")) {
    GInetAddr *ia;

    ia = gnet_socks_get_server ();
    if (ia) {
      gchar *name;

      name = gnet_inetaddr_get_canonical_name (ia);
      g_print ("\nUsing SOCKS %u proxy: %s\n", gnet_socks_get_version(), name);
      g_free (name);
      gnet_inetaddr_unref (ia);
      gnet_socks_set_enabled (TRUE);
    }
  }

  g_log_set_handler (NULL, G_LOG_LEVEL_MESSAGE, gnet_check_log_message_func,
      NULL);
  g_log_set_handler (NULL, G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING,
      gnet_check_log_critical_func, NULL);
  g_log_set_handler ("GNet", G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING,
      gnet_check_log_critical_func, NULL);
  g_log_set_handler ("GLib", G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING,
      gnet_check_log_critical_func, NULL);
  g_log_set_handler ("GLib-GObject", G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING,
      gnet_check_log_critical_func, NULL);

  check_cond = g_cond_new ();
  check_mutex = g_mutex_new ();
}


gint
gnet_check_run_suite (Suite * suite, const gchar * name, const gchar * fname)
{
  gint nf;

  SRunner *sr = srunner_create (suite);

  gnet_check_init (NULL, NULL);

  if (g_getenv ("GNET_CHECK_XML")) {
    /* how lucky we are to have __FILE__ end in .c */
    gchar *xmlfilename = g_strdup_printf ("%sheck.xml", fname);

    srunner_set_xml (sr, xmlfilename);
  }

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);
  return nf;
}

gboolean
_gnet_check_run_test_func (const gchar * func_name)
{
  const gchar *gnet_checks;
  gboolean res = FALSE;
  gchar **funcs, **f;

  gnet_checks = g_getenv ("GNET_CHECKS");

  /* no filter specified => run all checks */
  if (gnet_checks == NULL || *gnet_checks == '\0')
    return TRUE;

  /* only run specified functions */
  funcs = g_strsplit (gnet_checks, ",", -1);
  for (f = funcs; f != NULL && *f != NULL; ++f) {
    if (strcmp (*f, func_name) == 0) {
      res = TRUE;
      break;
    }
  }
  g_strfreev (funcs);
  return res;
}


