#include <cassert>
#include <cstring>

#include <CommonAPI/Wamp/WampError.hpp>

namespace CommonAPI {
namespace Wamp {

WampError::WampError() {
/*
    dbus_error_init(&libdbusError_);
*/
}

WampError::~WampError() {
/*
    dbus_error_free(&libdbusError_);
*/
}

WampError::operator bool() const {
/*
    return 0 != dbus_error_is_set(&libdbusError_);
*/
	return false;
}

void WampError::clear() {
/*
    dbus_error_free(&libdbusError_);
*/
}

std::string WampError::getName() const {
/*
    return std::string(libdbusError_.name);
*/
	return "";
}

std::string WampError::getMessage() const {
/*
    return std::string(libdbusError_.message);
*/
	return "";
}

} // namespace Wamp
} // namespace CommonAPI
