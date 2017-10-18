#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_WAMP_CONFIG_HPP_
#define COMMONAPI_WAMP_CONFIG_HPP_

#include <CommonAPI/CallInfo.hpp>

namespace CommonAPI {
namespace Wamp {

static const Timeout_t DEFAULT_SEND_TIMEOUT_MS = 5000;
static CommonAPI::CallInfo defaultCallInfo(DEFAULT_SEND_TIMEOUT_MS);

} // namespace Wamp
} // namespace CommonAPI

#endif // COMMONAPI_WAMP_CONFIG_HPP_
