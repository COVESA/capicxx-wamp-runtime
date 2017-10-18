#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_WAMP_ADDRESS_HPP_
#define COMMONAPI_WAMP_ADDRESS_HPP_

#include <iostream>

#include <CommonAPI/Export.hpp>

namespace CommonAPI {
namespace Wamp {

class WampAddress {
public:
    COMMONAPI_EXPORT WampAddress(const std::string &_realm = "");
    COMMONAPI_EXPORT WampAddress(const WampAddress &_source);
    COMMONAPI_EXPORT virtual ~WampAddress();

    COMMONAPI_EXPORT bool operator==(const WampAddress &_other) const;
    COMMONAPI_EXPORT bool operator!=(const WampAddress &_other) const;
    COMMONAPI_EXPORT bool operator<(const WampAddress &_other) const;

    COMMONAPI_EXPORT const std::string &getRealm() const;
    COMMONAPI_EXPORT void setRealm(const std::string &_realm);

private:
    std::string realm_;
friend std::ostream &operator<<(std::ostream &_out, const WampAddress &_wampAddress);
};

} // namespace Wamp
} // namespace CommonAPI

#endif // COMMONAPI_WAMP_ADDRESS_HPP_
