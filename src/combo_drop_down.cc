//
//    ComboDropDown: a Gtk::DropDown wearing Gtk::ComboBoxText's API.
//
//    The destructor lives here, out of line, so the class has a key
//    function: with MinGW+LTO the vague-linkage destructor/vtable/thunks
//    are not deduplicated by PE ld (GCC PR 94156).
//

#include "combo_drop_down.hh"


ComboDropDown::~ComboDropDown()
{
}
