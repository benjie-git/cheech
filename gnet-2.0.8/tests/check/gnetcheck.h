/* Common code for GNet unittests (based on GStreamer's libgstcheck)
 *
 * Copyright (C) 2004 Thomas Vander Stichele <thomas at apestaart dot org>
 * Copyright (C) 2007 Tim-Philipp Müller <tim centricular net>
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

#ifndef __GNET_CHECK_H__
#define __GNET_CHECK_H__

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <check.h>

#include <gnet.h>

G_BEGIN_DECLS

/* GNET_DEBUG_CATEGORY_EXTERN (check_debug);
#define GST_CAT_DEFAULT check_debug
*/

/* logging function for tests
 * a test uses g_message() to log a debug line
 * a gst unit test can be run with GNET_TEST_DEBUG env var set to see the
 * messages
 */
extern gboolean _gnet_check_threads_running;
extern gboolean _gnet_check_raised_critical;
extern gboolean _gnet_check_raised_warning;
extern gboolean _gnet_check_expecting_log;

/* global variables used in test methods */
GMutex *check_mutex;
GCond *check_cond;


gint gnet_check_run_suite (Suite *suite, const gchar *name, const gchar *fname);

/***
 * wrappers for START_TEST and END_TEST
 */
#define GNET_START_TEST(__testname) \
static void __testname (int __i__)\
{\
  /* GNET_DEBUG ("test start"); */ \
  tcase_fn_start (""# __testname, __FILE__, __LINE__);

#define GNET_END_TEST END_TEST

/* additional fail macros */
/**
 * fail_unless_equals_int:
 * @a: a #gint value or expression
 * @b: a #gint value or expression
 *
 * This macro checks that @a and @b are equal and aborts if this is not the
 * case, printing both expressions and the values they evaluated to. This
 * macro is for use in unit tests.
 */
#define fail_unless_equals_int(a, b)					\
G_STMT_START {								\
  int first = a;							\
  int second = b;							\
  fail_unless(first == second,						\
    "'" #a "' (%d) is not equal to '" #b"' (%d)", first, second);	\
} G_STMT_END;
/**
 * assert_equals_int:
 * @a: a #gint value or expression
 * @b: a #gint value or expression
 *
 * This macro checks that @a and @b are equal and aborts if this is not the
 * case, printing both expressions and the values they evaluated to. This
 * macro is for use in unit tests.
 */
#define assert_equals_int(a, b) fail_unless_equals_int(a, b)

/**
 * fail_unless_equals_uint64:
 * @a: a #guint64 value or expression
 * @b: a #guint64 value or expression
 *
 * This macro checks that @a and @b are equal and aborts if this is not the
 * case, printing both expressions and the values they evaluated to. This
 * macro is for use in unit tests.
 */
#define fail_unless_equals_uint64(a, b)					\
G_STMT_START {								\
  guint64 first = a;							\
  guint64 second = b;							\
  fail_unless(first == second,						\
    "'" #a "' (%" G_GUINT64_FORMAT ") is not equal to '" #b"' (%"	\
    G_GUINT64_FORMAT ")", first, second);				\
} G_STMT_END;
/**
 * assert_equals_uint64:
 * @a: a #guint64 value or expression
 * @b: a #guint64 value or expression
 *
 * This macro checks that @a and @b are equal and aborts if this is not the
 * case, printing both expressions and the values they evaluated to. This
 * macro is for use in unit tests.
 */
#define assert_equals_uint64(a, b) fail_unless_equals_uint64(a, b)

/**
 * fail_unless_equals_string:
 * @a: a string literal or expression
 * @b: a string literal or expression
 *
 * This macro checks that @a and @b are equal (as per strcmp) and aborts if
 * this is not the case, printing both expressions and the values they
 * evaluated to. This macro is for use in unit tests.
 */
#define fail_unless_equals_string(a, b)                             \
G_STMT_START {                                                      \
  const gchar * first = a;                                          \
  const gchar * second = b;                                         \
  fail_unless(strcmp (first, second) == 0,                          \
    "'" #a "' (%s) is not equal to '" #b"' (%s)", first, second);   \
} G_STMT_END;
/**
 * assert_equals_string:
 * @a: a string literal or expression
 * @b: a string literal or expression
 *
 * This macro checks that @a and @b are equal (as per strcmp) and aborts if
 * this is not the case, printing both expressions and the values they
 * evaluated to. This macro is for use in unit tests.
 */
#define assert_equals_string(a, b) fail_unless_equals_string(a, b)

/**
 * fail_unless_equals_float:
 * @a: a #gdouble or #gfloat value or expression
 * @b: a #gdouble or #gfloat value or expression
 *
 * This macro checks that @a and @b are (almost) equal and aborts if this
 * is not the case, printing both expressions and the values they evaluated
 * to. This macro is for use in unit tests.
 *
 * Since: 0.10.14
 */
#define fail_unless_equals_float(a, b)                            \
G_STMT_START {                                                    \
  double first = a;                                               \
  double second = b;                                              \
  /* This will only work for 'normal' values and values around 0, \
   * which should be good enough for our purposes here */         \
  fail_unless(fabs (first - second) < 0.0000001,                  \
    "'" #a "' (%g) is not equal to '" #b "' (%g)", first, second);\
} G_STMT_END;

/**
 * assert_equals_float:
 * @a: a #gdouble or #gfloat value or expression
 * @b: a #gdouble or #gfloat value or expression
 *
 * This macro checks that @a and @b are (almost) equal and aborts if this
 * is not the case, printing both expressions and the values they evaluated
 * to. This macro is for use in unit tests.
 *
 * Since: 0.10.14
 */
#define assert_equals_float(a, b) fail_unless_equals_float(a, b)


/***
 * thread test macros and variables
 */
extern GList *thread_list;
extern GMutex *mutex;
extern GCond *start_cond;	/* used to notify main thread of thread startups */
extern GCond *sync_cond;	/* used to synchronize all threads and main thread */

#define MAIN_START_THREADS(count, function, data)		\
MAIN_INIT();							\
MAIN_START_THREAD_FUNCTIONS(count, function, data);		\
MAIN_SYNCHRONIZE();

#define MAIN_INIT()			\
G_STMT_START {				\
  _gnet_check_threads_running = TRUE;	\
					\
  mutex = g_mutex_new ();		\
  start_cond = g_cond_new ();		\
  sync_cond = g_cond_new ();		\
} G_STMT_END;

#define MAIN_START_THREAD_FUNCTIONS(count, function, data)	\
G_STMT_START {							\
  int i;							\
  for (i = 0; i < count; ++i) {					\
    MAIN_START_THREAD_FUNCTION (i, function, data);		\
  }								\
} G_STMT_END;

#define MAIN_START_THREAD_FUNCTION(i, function, data)		\
G_STMT_START {							\
    GThread *thread = NULL;					\
    /* GNET_DEBUG ("MAIN: creating thread %d", i); */   	\
    g_mutex_lock (mutex);					\
    thread = g_thread_create ((GThreadFunc) function, data,	\
	TRUE, NULL);						\
    /* wait for thread to signal us that it's ready */		\
    /* GNET_DEBUG ("MAIN: waiting for thread %d", i); */	\
    g_cond_wait (start_cond, mutex);				\
    g_mutex_unlock (mutex);					\
								\
    thread_list = g_list_append (thread_list, thread);		\
} G_STMT_END;


#define MAIN_SYNCHRONIZE()			\
G_STMT_START {					\
  /* GNET_DEBUG ("MAIN: synchronizing"); */	\
  g_cond_broadcast (sync_cond);			\
  /* GNET_DEBUG ("MAIN: synchronized"); */	\
} G_STMT_END;

#define MAIN_STOP_THREADS()					\
G_STMT_START {							\
  _gnet_check_threads_running = FALSE;				\
								\
  /* join all threads */					\
  /* GNET_DEBUG ("MAIN: joining"); */				\
  g_list_foreach (thread_list, (GFunc) g_thread_join, NULL);	\
  /* GNET_DEBUG ("MAIN: joined"); */				\
} G_STMT_END;

#define THREAD_START()						\
THREAD_STARTED();						\
THREAD_SYNCHRONIZE();

#define THREAD_STARTED()					\
G_STMT_START {							\
  /* signal main thread that we started */			\
  /* GNET_DEBUG ("THREAD %p: started", g_thread_self ()); */	\
  g_mutex_lock (mutex);						\
  g_cond_signal (start_cond);					\
} G_STMT_END;

#define THREAD_SYNCHRONIZE()					\
G_STMT_START {							\
  /* synchronize everyone */					\
  /* GNET_DEBUG ("THREAD %p: syncing", g_thread_self ()); */	\
  g_cond_wait (sync_cond, mutex);				\
  /* GNET_DEBUG ("THREAD %p: synced", g_thread_self ()); */	\
  g_mutex_unlock (mutex);					\
} G_STMT_END;

#define THREAD_SWITCH()						\
G_STMT_START {							\
  /* a minimal sleep is a context switch */			\
  g_usleep (1);							\
} G_STMT_END;

#define THREAD_TEST_RUNNING()	(_gnet_check_threads_running == TRUE)

/* additional assertions */
#define ASSERT_CRITICAL(code)					\
G_STMT_START {							\
  _gnet_check_expecting_log = TRUE;				\
  _gnet_check_raised_critical = FALSE;				\
  code;								\
  _fail_unless (_gnet_check_raised_critical, __FILE__, __LINE__, \
                "Expected g_critical, got nothing", NULL);      \
  _gnet_check_expecting_log = FALSE;				\
} G_STMT_END

#define ASSERT_WARNING(code)					\
G_STMT_START {							\
  _gnet_check_expecting_log = TRUE;				\
  _gnet_check_raised_warning = FALSE;				\
  code;								\
  _fail_unless (_gnet_check_raised_warning, __FILE__, __LINE__,  \
                "Expected g_warning, got nothing", NULL);       \
  _gnet_check_expecting_log = FALSE;				\
} G_STMT_END


#define GNET_CHECK_MAIN(name)					\
int main (int argc, char **argv)				\
{								\
  Suite *s = name ## _suite ();					\
  return gnet_check_run_suite (s, # name, __FILE__);		\
}

/* This is a bit of a hack to allow filtering of which tests to run
 * at run-time, without recompiling, via the GNET_CHECKS environment
 * variable (specify test case function names, comma-separated) */

gboolean _gnet_check_run_test_func (const gchar * func_name);

static inline void
__gnet_tcase_add_test (TCase * tc, TFun tf, const gchar * func_name,
    int sig, int start, int end)
{
  if (_gnet_check_run_test_func (func_name)) {
    _tcase_add_test (tc, tf, func_name, sig, start, end);
  }
}

#define _tcase_add_test  __gnet_tcase_add_test

G_END_DECLS

#endif /* __GNET_CHECK_H__ */

