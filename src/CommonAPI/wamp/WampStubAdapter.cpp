#include <CommonAPI/Wamp/WampAddressTranslator.hpp>
#include <CommonAPI/Wamp/WampStubAdapter.hpp>

namespace CommonAPI {
namespace Wamp {

WampStubAdapter::WampStubAdapter(const WampAddress &_wampAddress,
                                 const std::shared_ptr<WampProxyConnection> &_connection,
                                 const bool _isManaging)
    : wampAddress_(_wampAddress),
      connection_(_connection),
      isManaging_(_isManaging) {
}

WampStubAdapter::~WampStubAdapter() {
    deinit();
}

void WampStubAdapter::init(std::shared_ptr<WampStubAdapter> _instance) {
    (void)_instance;
    WampAddressTranslator::get()->translate(wampAddress_, address_);
}

void WampStubAdapter::deinit() {
}

const WampAddress &WampStubAdapter::getWampAddress() const {
    return wampAddress_;
}

const std::shared_ptr<WampProxyConnection> &WampStubAdapter::getWampConnection() const {
    return connection_;
}

bool WampStubAdapter::isManaging() const {
    return isManaging_;
}

/*
bool WampStubAdapter::hasFreedesktopProperties() {
    return false;
}
*/

} // namespace Wamp
} // namespace CommonAPI
