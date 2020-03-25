/*
 *  Miscellaneous utility functions and extensions in the
 *  std namespace.
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

#ifndef INCL_UTILITY_HH
#define INCL_UTILITY_HH

#include <sstream>
#include <glibmm/ustring.h>


namespace util {
	template <typename T> Glib::ustring to_str(const T& value);
	template <typename T> T from_str(const Glib::ustring& str);

	Glib::ustring& ltrim(Glib::ustring& str);
	Glib::ustring& rtrim(Glib::ustring& str);
	Glib::ustring& trim(Glib::ustring& str);

	bool even(int number);
	bool even(unsigned int number);
	bool odd(int number);
	bool odd(unsigned int number);

	void delay_ms(int ms);

	int hex_decode(char hex);
	Glib::ustring url_decode(Glib::ustring encoded);
}


template <typename T>
Glib::ustring
util::to_str(const T& value)
{
	std::ostringstream stream;
	stream << value;
	return stream.str();
}


template <typename T>
T util::from_str(const Glib::ustring& str)
{
	std::istringstream stream(str);
	T value;
	stream >> value;

	return value;
}

#endif	//#ifndef INCL_UTILITY_HH
