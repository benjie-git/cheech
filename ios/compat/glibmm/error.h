#ifndef INCL_COMPAT_GLIBMM_ERROR_H
#define INCL_COMPAT_GLIBMM_ERROR_H

#include <exception>
#include <string>

namespace Glib {

class Error : public std::exception
{
public:
	explicit Error(const std::string& message = std::string())
		: _message(message)
	{
	}

	const char* what() const noexcept override
	{
		return _message.c_str();
	}

private:
	std::string _message;
};

} // namespace Glib

#endif   // #ifndef INCL_COMPAT_GLIBMM_ERROR_H
