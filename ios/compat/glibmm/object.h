/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_COMPAT_GLIBMM_OBJECT_H
#define INCL_COMPAT_GLIBMM_OBJECT_H

#include <glibmm/refptr.h>

namespace Glib {

class Object
{
public:
	Object() = default;
	virtual ~Object() = default;
};

} // namespace Glib

#endif   // #ifndef INCL_COMPAT_GLIBMM_OBJECT_H
