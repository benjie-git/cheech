/*
 *  Minimal GLib type shim for the iOS build of cheech.
 *
 *  The game core only needs a handful of GLib scalar typedefs; this header
 *  provides them without pulling in the real GLib.  It is only used when
 *  building with -DCHEECH_IOS, via the include path pointing at ios/compat.
 */

#ifndef CHEECH_COMPAT_GLIB_H
#define CHEECH_COMPAT_GLIB_H

#include <cstddef>
#include <cstdint>

typedef char                gchar;
typedef unsigned char       guchar;
typedef int                 gint;
typedef unsigned int        guint;
typedef short               gshort;
typedef unsigned short      gushort;
typedef long                glong;
typedef unsigned long       gulong;
typedef int                 gboolean;
typedef void*               gpointer;
typedef const void*         gconstpointer;
typedef std::size_t         gsize;
typedef std::ptrdiff_t      gssize;
typedef std::int8_t         gint8;
typedef std::uint8_t        guint8;
typedef std::int16_t        gint16;
typedef std::uint16_t       guint16;
typedef std::int32_t        gint32;
typedef std::uint32_t       guint32;
typedef std::int64_t        gint64;
typedef std::uint64_t       guint64;
typedef float               gfloat;
typedef double              gdouble;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif /* CHEECH_COMPAT_GLIB_H */
