#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_WAMP_DBUSTYPES_HPP_
#define COMMONAPI_WAMP_DBUSTYPES_HPP_

#include <string>
#include <unordered_map>

namespace CommonAPI {
namespace Wamp {

/*
typedef std::unordered_map<std::string, bool> WampPropertiesChangedDict;
typedef std::unordered_map<std::string, WampPropertiesChangedDict> WampInterfacesAndPropertiesDict;
typedef std::unordered_map<std::string, WampInterfacesAndPropertiesDict> WampObjectPathAndInterfacesDict;
*/

enum class WampType_t {
    SESSION,
    SYSTEM,
    STARTER,
    WRAPPED
};

} // namespace Wamp
} // namespace CommonAPI

#endif // COMMONAPI_WAMP_DBUSTYPES_HPP_
