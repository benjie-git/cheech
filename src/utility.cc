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

#include <glibmm.h>

#ifdef WIN32
#include <windows.h>
#include <shlwapi.h>
#endif

#include "utility.hh"


Glib::ustring& util::ltrim(Glib::ustring& str)
{
	if(!str.empty())
	{
		Glib::ustring::iterator it;
		for(it = str.begin(); it < str.end() && isspace(*it); it++);
		str.erase(str.begin(), it);
	}

	return str;
}


Glib::ustring& util::rtrim(Glib::ustring& str)
{
	if(!str.empty())
	{
        Glib::ustring::iterator it;
        Glib::ustring::iterator end = --(str.end());
		for(it = end; it > str.begin() && isspace(*it); it--);
		it++;
        str.erase(it, str.end());
	}

	return str;
}


Glib::ustring& util::trim(Glib::ustring& str)
{
	return ltrim(rtrim(str));
}


bool util::even(int number)
{
	return (number % 2) == 0;
}


bool util::even(unsigned int number)
{
	return (number % 2) == 0;
}


bool util::odd(int number)
{
	return (number % 2) != 0;
}


bool util::odd(unsigned int number)
{
	return (number % 2) != 0;
}


void util::delay_ms(int ms)
{
	for (int i = 0; i < ms; i += 10)
	{
		while (Glib::MainContext::get_default()->iteration(false));
#ifndef WIN32
		Glib::usleep(10000);
#else
		Sleep(10);
#endif
	}
	while (Glib::MainContext::get_default()->iteration(false));
}


int util::hex_decode(char hex)
{
	if ((hex >= 'A') && (hex <= 'F'))
		return (hex - 'A');

	if ((hex >= 'a') && (hex <= 'f'))
		return (hex - 'a');

	if ((hex >= '0') && (hex <= '9'))
		return (hex - '0');

	return -1;
}


Glib::ustring util::url_decode(Glib::ustring encoded)
{
	Glib::ustring decoded(encoded);

	unsigned int encPos, decPos;

	for(encPos = 0, decPos = 0; encoded[encPos]; encPos++, decPos++)
	{
		if(encoded[encPos] != '%')
		{
			if(encoded[encPos] == '+')
				decoded.replace(decPos, 1, 1, ' ');
		}
		else
		{
			if(encoded.length() > encPos+2)
			{
				int d1 = hex_decode(encoded[encPos+1]);
				int d2 = hex_decode(encoded[encPos+2]);

				if (d1 >=0 && d2 >= 0)
				{
					decoded.replace(decPos, 3, 1, (char)((d1 * 16) + d2));
					encPos += 2;
				}
			}
		}
	}

	return decoded;
}
