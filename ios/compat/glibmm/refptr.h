#ifndef INCL_COMPAT_GLIBMM_REFPTR_H
#define INCL_COMPAT_GLIBMM_REFPTR_H

#include <memory>

namespace Glib {

// Real glibmm defines RefPtr as an alias for std::shared_ptr; mirror that so
// the shared networking code (including std::dynamic_pointer_cast) compiles.
template <class T_CppObject>
using RefPtr = std::shared_ptr<T_CppObject>;

} // namespace Glib

#endif   // #ifndef INCL_COMPAT_GLIBMM_REFPTR_H
