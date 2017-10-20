#include <CommonAPI/Wamp/WampAddress.hpp>

namespace CommonAPI {
namespace Wamp {

WampAddress::WampAddress(const std::string &_realm)
    : realm_(_realm) {
}

WampAddress::WampAddress(const WampAddress &_source)
    : realm_(_source.realm_) {
}

WampAddress::~WampAddress() {
}

bool
WampAddress::operator==(const WampAddress &_other) const {
    return (realm_ == _other.realm_);
}

bool
WampAddress::operator !=(const WampAddress &_other) const {
    return (realm_ != _other.realm_);
}

bool
WampAddress::operator<(const WampAddress &_other) const {
    if (realm_ < _other.realm_)
        return true;

    return false;
}

const std::string &
WampAddress::getRealm() const {
    return realm_;
}

void
WampAddress::setRealm(const std::string &_realm) {
    realm_ = _realm;
}

std::ostream &
operator<<(std::ostream &_out, const WampAddress &_wampAddress) {
    _out << "realm=" << _wampAddress.realm_.c_str();
    return _out;
}


} // namespace Wamp
} // namespace CommonAPI
