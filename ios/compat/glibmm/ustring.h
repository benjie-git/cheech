/*
 *  Minimal Glib::ustring shim for the iOS build of cheech.
 *
 *  The game core only treats ustring as a UTF-8 std::string: it never
 *  performs UTF-8 aware indexing or iteration that would change behaviour
 *  for the short ASCII names/IPs the game exchanges.  We therefore model it
 *  as a std::string with the extra methods the core actually calls.
 */

#ifndef CHEECH_COMPAT_GLIBMM_USTRING_H
#define CHEECH_COMPAT_GLIBMM_USTRING_H

#include <string>
#include <cctype>

namespace Glib {

class ustring : public std::string
{
public:
	typedef std::string::size_type size_type;
	typedef std::string::iterator iterator;
	typedef std::string::const_iterator const_iterator;

	ustring() = default;
	ustring(const std::string& s) : std::string(s) {}
	ustring(std::string&& s) : std::string(std::move(s)) {}
	ustring(const char* s) : std::string(s ? s : "") {}
	ustring(const ustring&) = default;
	ustring(ustring&&) noexcept = default;

	ustring& operator=(const ustring&) = default;
	ustring& operator=(ustring&&) noexcept = default;
	ustring& operator=(const std::string& s)
	{
		std::string::operator=(s);
		return *this;
	}
	ustring& operator=(const char* s)
	{
		std::string::operator=(s ? s : "");
		return *this;
	}

	ustring lowercase() const
	{
		ustring result(*this);
		for (char& c : result)
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		return result;
	}

	ustring uppercase() const
	{
		ustring result(*this);
		for (char& c : result)
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		return result;
	}
};

} // namespace Glib

#endif /* CHEECH_COMPAT_GLIBMM_USTRING_H */
