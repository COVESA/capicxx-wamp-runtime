#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_WAMP_FACTORY_HPP_
#define COMMONAPI_WAMP_FACTORY_HPP_

#include <list>
#include <map>
#include <unordered_map>

#include <CommonAPI/Export.hpp>
#include <CommonAPI/Factory.hpp>

namespace CommonAPI {

class Runtime;

namespace Wamp {

class WampAddress;
class WampConnection;
class WampProxy;
class WampProxyConnection;
class WampStubAdapter;

typedef void (*InterfaceInitFunction)(void);

typedef std::shared_ptr<WampProxy>
(*ProxyCreateFunction)(const WampAddress &_address,
                       const std::shared_ptr<WampProxyConnection> &_connection);

typedef std::shared_ptr<WampStubAdapter>
(*StubAdapterCreateFunction) (const WampAddress &_address,
                              const std::shared_ptr<WampProxyConnection> &_connection,
                              const std::shared_ptr<StubBase> &_stub);

class Factory : public CommonAPI::Factory {
public:
    COMMONAPI_EXPORT static std::shared_ptr<Factory> get();

    COMMONAPI_EXPORT Factory();
    COMMONAPI_EXPORT virtual ~Factory();

    COMMONAPI_EXPORT void init();

    COMMONAPI_EXPORT void registerProxyCreateMethod(const std::string &_address,
                                    ProxyCreateFunction _function);

    COMMONAPI_EXPORT void registerStubAdapterCreateMethod(const std::string &_address,
                                         StubAdapterCreateFunction _function);


    COMMONAPI_EXPORT std::shared_ptr<Proxy> createProxy(const std::string &_domain,
                                       const std::string &_interface,
                                       const std::string &_instance,
                                       const ConnectionId_t &_connectionId);

    COMMONAPI_EXPORT std::shared_ptr<Proxy> createProxy(const std::string &_domain,
                                       const std::string &_interface,
                                       const std::string &_instance,
                                       std::shared_ptr<MainLoopContext> _context);

    COMMONAPI_EXPORT bool registerStub(const std::string &_domain,
                                const std::string &_interface,
                                const std::string &_instance,
                          std::shared_ptr<StubBase> _stub,
                          const ConnectionId_t &_connectionId);

    COMMONAPI_EXPORT bool registerStub(const std::string &_domain,
                            const std::string &_interface,
                            const std::string &_instance,
                      std::shared_ptr<StubBase> _stub,
                      std::shared_ptr<MainLoopContext> _context);

    COMMONAPI_EXPORT bool unregisterStub(const std::string &_domain,
                        const std::string &_interface,
                        const std::string &_instance);

    // Services
    COMMONAPI_EXPORT std::shared_ptr<WampStubAdapter> getRegisteredService(const std::string &_address);

    // Managed services
    COMMONAPI_EXPORT std::shared_ptr<WampStubAdapter> createWampStubAdapter(const std::shared_ptr<StubBase> &_stub,
                                                           const std::string &_interface,
                                                           const WampAddress &_address,
                                                           const std::shared_ptr<WampProxyConnection> &_connection);
    COMMONAPI_EXPORT bool registerManagedService(const std::shared_ptr<WampStubAdapter> &_adapter);
    COMMONAPI_EXPORT bool unregisterManagedService(const std::string &_address);


    // Initialization
    COMMONAPI_EXPORT void registerInterface(InterfaceInitFunction _function);

    static std::weak_ptr<CommonAPI::Runtime> runtime_;

private:
    COMMONAPI_EXPORT std::shared_ptr<WampConnection> getConnection(const ConnectionId_t &);
    COMMONAPI_EXPORT std::shared_ptr<WampConnection> getConnection(std::shared_ptr<MainLoopContext>);
    COMMONAPI_EXPORT bool registerStubAdapter(std::shared_ptr<WampStubAdapter>);
    COMMONAPI_EXPORT bool unregisterStubAdapter(std::shared_ptr<WampStubAdapter>);

    // Managed services
    typedef std::unordered_map<std::string, std::shared_ptr<WampStubAdapter>> ServicesMap;
    COMMONAPI_EXPORT bool unregisterManagedService(const ServicesMap::iterator &);

private:
    static std::shared_ptr<Factory> theFactory;

    std::map<ConnectionId_t, std::shared_ptr<WampConnection>> connections_;
    std::map<MainLoopContext *, std::shared_ptr<WampConnection>> contextConnections_;

    std::map<std::string, ProxyCreateFunction> proxyCreateFunctions_;
    std::map<std::string, StubAdapterCreateFunction> stubAdapterCreateFunctions_;

    ServicesMap services_;

    std::list<InterfaceInitFunction> initializers_;
    std::mutex initializerMutex_;
    bool isInitialized_;
};

} // namespace Wamp
} // namespace CommonAPI

#endif // COMMONAPI_WAMP_FACTORY_HPP_
