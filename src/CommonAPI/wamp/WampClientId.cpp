
#include <CommonAPI/Wamp/WampClientId.hpp>


namespace CommonAPI {
namespace Wamp {

WampClientId::WampClientId(uint32_t _clientNumber) : clientNumber_(_clientNumber) {
}

bool WampClientId::operator==(CommonAPI::ClientId &_clientIdToCompare) {
    try {
        WampClientId clientIdToCompareWamp = WampClientId(dynamic_cast<WampClientId&>(_clientIdToCompare));
        return (clientIdToCompareWamp == *this);
    }
    catch (...) {
        return false;
    }
}

bool WampClientId::operator==(WampClientId &_clientIdToCompare) {
    return (_clientIdToCompare.clientNumber_ == clientNumber_);
}

size_t WampClientId::hashCode()
{
    return (size_t)clientNumber_;
}

} // namespace Wamp
} // namespace CommonAPI
