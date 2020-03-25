//
// Needed to work around 2 shortcomings of libglade--
//
// 1) It gets confused about a Gtk::Image with no predefined pixmap in it
// 2) To work around that, I use Custom Widgets, of type GtkImage, but it
//    looks for a file called GtkImage.hh.  Hence this file.
//
//    (If you know/figure out a way to clean this up, please tell me!!)
//

#include <gtkmm/image.h>


#define GtkImage Gtk::Image
