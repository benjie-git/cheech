/*
 *  Stand-in for the autotools-generated config.h for the iOS build.
 *
 *  The game core includes "config.h" but only relies on it for build metadata;
 *  the iOS target supplies its own values here rather than running configure.
 */

#ifndef CHEECH_COMPAT_CONFIG_H
#define CHEECH_COMPAT_CONFIG_H

#define PACKAGE          "cheech"
#define PACKAGE_NAME     "cheech"
#define PACKAGE_VERSION  "0.1"
#define PACKAGE_STRING   "cheech 0.1"
#define PACKAGE_TARNAME  "cheech"
#define PACKAGE_BUGREPORT "http://cheech.sourceforge.net/"
#define PACKAGE_URL      "http://cheech.sourceforge.net/"
#define GETTEXT_PACKAGE  "cheech"
#define VERSION          "0.1"

#define CHEECH_IOS 1

#endif /* CHEECH_COMPAT_CONFIG_H */
