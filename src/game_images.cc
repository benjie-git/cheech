/*
 *  Contains all the static images.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 */

#ifdef WIN32
#include <windows.h>
#include <shlwapi.h>
#endif

#include "config.h"
#include "game_images.hh"

#ifdef MACOS_APP
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace std;
using Gdk::Pixbuf;

Glib::RefPtr<Pixbuf> GameImages::_highlight;
Glib::RefPtr<Pixbuf> GameImages::_pegs[NUM_COLORS+1];

Glib::RefPtr<Pixbuf> GameImages::_logos[2];

Gdk::Point GameImages::_highlight_size;
Gdk::Point GameImages::_peg_size;

const char* GameImages::color_names[GameImages::NUM_COLORS] =
	{"red", "orange", "yellow", "green", "blue", "purple", "black", "white"};

void GameImages::init()
{
	Glib::ustring pixmaps_dir;

#ifdef WIN32
	char module_path[PATH_MAX];
	char *program_dir;

	GetModuleFileName(NULL, module_path, PATH_MAX);
	PathRemoveFileSpec(module_path);
	program_dir = g_locale_to_utf8(module_path, -1, NULL, NULL, NULL);
	pixmaps_dir = g_strdup_printf("%s\\pixmaps\\", program_dir);
#elif MACOS_APP
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef burl = CFBundleCopyBundleURL(mainBundle);
	CFURLRef rurl = CFBundleCopyResourcesDirectoryURL(mainBundle);
	CFStringRef bstr = CFURLCopyFileSystemPath(burl, kCFURLPOSIXPathStyle);
	CFStringRef rstr = CFURLCopyFileSystemPath(rurl, kCFURLPOSIXPathStyle);
	pixmaps_dir = CFStringGetCStringPtr(bstr, kCFStringEncodingUTF8);
	pixmaps_dir += "/";
	pixmaps_dir += CFStringGetCStringPtr(rstr, kCFStringEncodingUTF8);
	pixmaps_dir += "/";
	CFRelease(rstr);
	CFRelease(bstr);
	CFRelease(rurl);
	CFRelease(burl);
#else
	pixmaps_dir = PACKAGE_PIXMAPS_DIR "/" PACKAGE "/";
#endif

	Glib::ustring hstr = pixmaps_dir + "highlight.png";
	_highlight = Pixbuf::create_from_file(hstr);
	_highlight_size = Gdk::Point(_highlight->get_width(),
								 _highlight->get_height());
	
	for (unsigned int i = 0; i < NUM_COLORS+1; i++)
	{
		Glib::RefPtr<Pixbuf> pixbuf =
			Pixbuf::create_from_file(pixmaps_dir + "pegs.png");
		Gdk::Point size(pixbuf->get_width() / (NUM_COLORS+1),
						pixbuf->get_height());

		_pegs[i] = Pixbuf::create_subpixbuf(pixbuf, size.get_x() * i, 0,
											size.get_x(), size.get_y());
		_peg_size = size;
	}

	_logos[0] = Pixbuf::create_from_file(pixmaps_dir + "cheech.png");
	_logos[1] = Pixbuf::create_from_file(pixmaps_dir + "smiley.png");
}


Glib::RefPtr<Pixbuf> GameImages::get_highlight()
{
	return _highlight;
}


Glib::RefPtr<Pixbuf> GameImages::get_peg(unsigned int id)
{
	g_assert(id >= 0 && id <= NUM_COLORS);

	return _pegs[id];
}


Glib::RefPtr<Pixbuf> GameImages::get_logo(bool win)
{
	if (win)
		return _logos[1];
	else
		return _logos[0];
}


Gdk::Point GameImages::get_highlight_size()
{
	return _highlight_size;
}


Gdk::Point GameImages::get_peg_size()
{
	return _peg_size;
}


unsigned int GameImages::get_num_colors()
{
	return NUM_COLORS;
}


const char *GameImages::get_color_name(unsigned int color)
{
	g_assert(color >= 1 && color <= NUM_COLORS);
	return color_names[color-1];
}
