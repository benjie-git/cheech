//
//    ClickCounter: a Gtk::GestureClick that counts successive clicks
//    itself.
//
//    The destructor lives here, out of line, so the class has a key
//    function: with MinGW+LTO the vague-linkage destructor/vtable/thunks
//    are not deduplicated by PE ld (GCC PR 94156).
//

#include "click_counter.hh"


ClickCounter::~ClickCounter()
{
}
