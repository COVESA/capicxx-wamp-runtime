
#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_WAMP_CLIENTID_HPP_
#define COMMONAPI_WAMP_CLIENTID_HPP_

#include <CommonAPI/Export.hpp>
#include <CommonAPI/Types.hpp>

namespace CommonAPI {
namespace Wamp {

class WampMessage;

/**
 * \brief Implementation of CommonAPI::ClientId for WAMP
 *
 * This class represents the WAMP specific implementation of CommonAPI::ClientId.
 * It internally uses a unique number to identify clients. This number has to be provided by the WAMP sender uniquely.
 */
class WampClientId : public CommonAPI::ClientId {

public:
    COMMONAPI_EXPORT WampClientId(uint32_t _clientNumber);

    COMMONAPI_EXPORT bool operator==(CommonAPI::ClientId& _clientIdToCompare);
    COMMONAPI_EXPORT bool operator==(WampClientId &_clientIdToCompare);
    COMMONAPI_EXPORT size_t hashCode();

protected:
    uint32_t clientNumber_;
};

} // namespace Wamp
} // namespace CommonAPI

#endif // COMMONAPI_WAMP_CLIENTID_HPP_
