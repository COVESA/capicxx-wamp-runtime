#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_WAMP_ADDRESSTRANSLATOR_HPP_
#define COMMONAPI_WAMP_ADDRESSTRANSLATOR_HPP_

#include <map>
#include <memory>
#include <mutex>

#include <CommonAPI/Types.hpp>
#include <CommonAPI/Address.hpp>
#include <CommonAPI/Wamp/WampAddress.hpp>
#include <CommonAPI/Wamp/WampTypes.hpp>

namespace CommonAPI {
namespace Wamp {

class WampAddressTranslator {
public:
    COMMONAPI_EXPORT static std::shared_ptr<WampAddressTranslator> get();

    COMMONAPI_EXPORT WampAddressTranslator();

    COMMONAPI_EXPORT void init();

    COMMONAPI_EXPORT bool translate(const std::string &_key, WampAddress &_value);
    COMMONAPI_EXPORT bool translate(const CommonAPI::Address &_key, WampAddress &_value);

    COMMONAPI_EXPORT bool translate(const WampAddress &_key, std::string &_value);
    COMMONAPI_EXPORT bool translate(const WampAddress &_key, CommonAPI::Address &_value);

/*TG
    COMMONAPI_EXPORT void insert(const std::string &_address,
        const std::string &_realm, const bool _objPathStartWithDigits = false);
*/

    COMMONAPI_EXPORT WampType_t getWampBusType(const ConnectionId_t &_connectionId) const;

private:
/*TG
    COMMONAPI_EXPORT bool readConfiguration();

    COMMONAPI_EXPORT bool isValid(const std::string &, const char,
                                  bool = false, bool = false, bool = false) const;
*/

private:
    bool isDefault_;

    std::string defaultConfig_;
    std::string defaultDomain_;

    std::map<CommonAPI::Address, WampAddress> forwards_;
    std::map<WampAddress, CommonAPI::Address> backwards_;

    std::mutex mutex_;

    std::map<ConnectionId_t, WampType_t> wampTypes_;
};

} // namespace Wamp
} // namespace CommonAPI

#endif // COMMONAPI_WAMP_ADDRESSTRANSLATOR_HPP_
