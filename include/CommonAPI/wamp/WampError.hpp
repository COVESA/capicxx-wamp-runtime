#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_WAMP_ERROR_HPP_
#define COMMONAPI_WAMP_ERROR_HPP_

#include <string>

#include <CommonAPI/Export.hpp>

namespace CommonAPI {
namespace Wamp {

class WampConnection;


class COMMONAPI_EXPORT WampError {
 public:
    WampError();
    ~WampError();

    operator bool() const;

    void clear();

    std::string getName() const;
    std::string getMessage() const;

 private:
/*
    ::DBusError libdbusError_;
*/

    friend class WampConnection;
};

} // namespace Wamp
} // namespace CommonAPI

#endif // COMMONAPI_WAMP_ERROR_HPP_
