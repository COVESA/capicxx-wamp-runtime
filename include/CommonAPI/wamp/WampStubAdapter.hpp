#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_WAMP_STUBADAPTER_HPP_
#define COMMONAPI_WAMP_STUBADAPTER_HPP_

#include <memory>

#include <CommonAPI/Export.hpp>
#include <CommonAPI/Stub.hpp>
#include <CommonAPI/Wamp/WampAddress.hpp>
// #include <CommonAPI/Wamp/WampInterfaceHandler.hpp>

namespace CommonAPI {
namespace Wamp {

class WampProxyConnection;

class WampStubAdapter
        : virtual public CommonAPI::StubAdapter
//        , public WampInterfaceHandler
{
 public:
     COMMONAPI_EXPORT WampStubAdapter(const WampAddress &_wampAddress,
                    const std::shared_ptr<WampProxyConnection> &_connection,
                    const bool isManagingInterface);

     COMMONAPI_EXPORT virtual ~WampStubAdapter();

     COMMONAPI_EXPORT virtual void init(std::shared_ptr<WampStubAdapter> _instance);
     COMMONAPI_EXPORT virtual void deinit();

     COMMONAPI_EXPORT const WampAddress &getWampAddress() const;
     COMMONAPI_EXPORT const std::shared_ptr<WampProxyConnection> &getWampConnection() const;

     COMMONAPI_EXPORT bool isManaging() const;

/*
     COMMONAPI_EXPORT virtual const char* getMethodsDBusIntrospectionXmlData() const = 0;
     COMMONAPI_EXPORT virtual bool onInterfaceDBusMessage(const DBusMessage &_message) = 0;

     COMMONAPI_EXPORT virtual void deactivateManagedInstances() = 0;
     COMMONAPI_EXPORT virtual bool hasFreedesktopProperties();
     COMMONAPI_EXPORT virtual bool onInterfaceDBusFreedesktopPropertiesMessage(const DBusMessage &_message) = 0;
*/

     COMMONAPI_EXPORT virtual void provideRemoteMethods() = 0;

 protected:
    WampAddress wampAddress_;
    const std::shared_ptr<WampProxyConnection> connection_;
    const bool isManaging_;
};

} // namespace Wamp
} // namespace CommonAPI

#endif // COMMONAPI_WAMP_STUBADAPTER_HPP_
