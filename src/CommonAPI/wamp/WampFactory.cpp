#include <iostream>
#include <sstream>

#include <CommonAPI/Logger.hpp>
#include <CommonAPI/Runtime.hpp>
#include <CommonAPI/Wamp/WampAddressTranslator.hpp>
#include <CommonAPI/Wamp/WampConnection.hpp>
#include <CommonAPI/Wamp/WampFactory.hpp>
// #include <CommonAPI/Wamp/WampProxy.hpp>
#include <CommonAPI/Wamp/WampStubAdapter.hpp>
#include <CommonAPI/Wamp/WampTypes.hpp>

namespace CommonAPI {
namespace Wamp {

INITIALIZER(FactoryInit) {
    Runtime::get()->registerFactory("wamp", Factory::get());
}

std::shared_ptr<CommonAPI::Wamp::Factory>
Factory::get() {
    static std::shared_ptr<Factory> theFactory = std::make_shared<Factory>();
    return theFactory;
}

Factory::Factory() {
}

Factory::~Factory() {
}

void
Factory::registerProxyCreateMethod(
    const std::string &_interface, ProxyCreateFunction _function) {
    proxyCreateFunctions_[_interface] = _function;
}

void
Factory::registerStubAdapterCreateMethod(
    const std::string &_interface, StubAdapterCreateFunction _function) {
    stubAdapterCreateFunctions_[_interface] = _function;
}

std::shared_ptr<Proxy>
Factory::createProxy(
        const std::string &_domain, const std::string &_interface, const std::string &_instance,
        const ConnectionId_t &_connectionId) {
/*TG
    auto proxyCreateFunctionsIterator = proxyCreateFunctions_.find(_interface);
    if (proxyCreateFunctionsIterator != proxyCreateFunctions_.end()) {
        CommonAPI::Address address(_domain, _interface, _instance);
        WampAddress wampAddress;

        if (WampAddressTranslator::get()->translate(address, wampAddress)) {
            std::shared_ptr<WampProxy> proxy
                = proxyCreateFunctionsIterator->second(wampAddress, getConnection(_connectionId));
            if (proxy)
                proxy->init();
            return proxy;
        }
    }
*/
    return nullptr;
}

std::shared_ptr<Proxy>
Factory::createProxy(
    const std::string &_domain, const std::string &_interface, const std::string &_instance,
    std::shared_ptr<MainLoopContext> _context) {

/*TG
    auto proxyCreateFunctionsIterator = proxyCreateFunctions_.find(_interface);
    if (proxyCreateFunctionsIterator != proxyCreateFunctions_.end()) {
        CommonAPI::Address address(_domain, _interface, _instance);
        WampAddress wampAddress;

        if (WampAddressTranslator::get()->translate(address, wampAddress)) {
            std::shared_ptr<WampProxy> proxy
                = proxyCreateFunctionsIterator->second(wampAddress, getConnection(_context));
            if (proxy)
                proxy->init();
            return proxy;
        }
    }
*/

    return nullptr;
}

bool
Factory::registerStub(
        const std::string &_domain, const std::string &_interface, const std::string &_instance,
        std::shared_ptr<StubBase> _stub, const ConnectionId_t &_connectionId) {
	std::cout << "Factory::registerStub called" << std::endl;
	std::cout << "Factory::registerStub: _domain=" << _domain << ", _interface=" << _interface << ", _instance=" << _instance << ", _connectionId=" << _connectionId << std::endl;
    auto stubAdapterCreateFunctionsIterator = stubAdapterCreateFunctions_.find(_interface);
    if (stubAdapterCreateFunctionsIterator != stubAdapterCreateFunctions_.end()) {
    	std::cout << "Factory::registerStub: new stub" << std::endl;
        CommonAPI::Address address(_domain, _interface, _instance);
        WampAddress wampAddress;
        if (WampAddressTranslator::get()->translate(address, wampAddress)) {
        	std::cout << "WAMP address: " << wampAddress << std::endl;
            std::shared_ptr<WampStubAdapter> adapter
                = stubAdapterCreateFunctionsIterator->second(wampAddress, getConnection(_connectionId), _stub);
            if (adapter) {
            	std::cout << "Factory::registerStub: stub adapter created" << std::endl;
                adapter->init(adapter);
                adapter->provideRemoteMethods();
                return registerStubAdapter(adapter);
            }
        }
    }

    return false;
}

bool
Factory::registerStub(
        const std::string &_domain, const std::string &_interface, const std::string &_instance,
        std::shared_ptr<StubBase> _stub, std::shared_ptr<MainLoopContext> _context) {
    auto stubAdapterCreateFunctionsIterator = stubAdapterCreateFunctions_.find(_interface);
    if (stubAdapterCreateFunctionsIterator != stubAdapterCreateFunctions_.end()) {
        CommonAPI::Address address(_domain, _interface, _instance);
        WampAddress wampAddress;
        if (WampAddressTranslator::get()->translate(address, wampAddress)) {
            std::shared_ptr<WampStubAdapter> adapter
                = stubAdapterCreateFunctionsIterator->second(wampAddress, getConnection(_context), _stub);
            if (adapter) {
                adapter->init(adapter);
                adapter->provideRemoteMethods();
                return registerStubAdapter(adapter);
            }
        }
    }
    return false;
}

bool
Factory::unregisterStub(const std::string &_domain, const std::string &_interface, const std::string &_instance) {
    CommonAPI::Address address(_domain, _interface, _instance);
    const auto &adapterResult = services_.find(address.getAddress());
    if (adapterResult != services_.end()) {
        const auto _adapter = adapterResult->second;
        const auto &connection = _adapter->getWampConnection();
/*
        const auto objectManager = connection->getWampObjectManager();

        if (!objectManager->unregisterWampStubAdapter(_adapter)) {
            return false;
        }
*/

        if (!connection->releaseServiceName(_adapter->getWampAddress().getRealm())) {
//TG        if (!connection->releaseServiceName(_adapter->getWampAddress().getService())) {
            return false;
        }

        if (!unregisterStubAdapter(_adapter)) {
            return false;
        }

        services_.erase(adapterResult->first);

        return true;
    }

    return false;
}

bool
Factory::registerStubAdapter(std::shared_ptr<WampStubAdapter> _adapter) {
	std::cout << "Factory::registerStubAdapter called" << std::endl;
    CommonAPI::Address address;
    WampAddress wampAddress = _adapter->getWampAddress();
    if (WampAddressTranslator::get()->translate(wampAddress, address)) {
        const auto &insertResult = services_.insert( { address.getAddress(), _adapter } );

        const auto &connection = _adapter->getWampConnection();

/*TG
        std::shared_ptr<WampObjectManagerStub> root
            = connection->getWampObjectManager()->getRootWampObjectManagerStub();
        if (!root->exportManagedWampStubAdapter(_adapter)) {
            (void)unregisterManagedService(address.getAddress());
            return false;
        }

        const auto objectManager = connection->getWampObjectManager();
        if (!objectManager->registerWampStubAdapter(_adapter)) {
            (void)root->unexportManagedWampStubAdapter(_adapter);
            (void)unregisterManagedService(address.getAddress());
            services_.erase(insertResult.first);
            return false;
        }

        const bool isServiceNameAcquired
            = connection->requestServiceNameAndBlock(wampAddress.getRealm());
//TG            = connection->requestServiceNameAndBlock(wampAddress.getService());
        if (!isServiceNameAcquired) {
            (void)root->unexportManagedWampStubAdapter(_adapter);
            (void)objectManager->unregisterWampStubAdapter(_adapter);
            (void)unregisterManagedService(address.getAddress());
            services_.erase(insertResult.first);
            return false;
        }
*/

        return true;
    }

    return false;
}

bool
Factory::unregisterStubAdapter(std::shared_ptr<WampStubAdapter> _adapter) {
    CommonAPI::Address address;
    WampAddress wampAddress = _adapter->getWampAddress();
    if (WampAddressTranslator::get()->translate(wampAddress, address)) {
        const auto &connection = _adapter->getWampConnection();

/*TG
        std::shared_ptr<WampObjectManagerStub> root
            = connection->getWampObjectManager()->getRootWampObjectManagerStub();
        if (!root->unexportManagedWampStubAdapter(_adapter)) {
            //(void)unregisterManagedService(address.getAddress());
            return false;
        }
*/

        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// Connections
///////////////////////////////////////////////////////////////////////////////
std::shared_ptr<WampConnection>
Factory::getConnection(const ConnectionId_t &_connectionId) {
	std::cout << "Factory::getConnection called (_connectionId=" << _connectionId << ")" << std::endl;

    auto itsConnectionIterator = connections_.find(_connectionId);
    if (itsConnectionIterator != connections_.end()) {
        return itsConnectionIterator->second;
    }

    // No connection found, lets create and initialize one
    WampType_t wampType = WampAddressTranslator::get()->getWampBusType(_connectionId);
    std::shared_ptr<WampConnection> itsConnection
        = std::make_shared<WampConnection>(wampType, _connectionId);
    connections_.insert({ _connectionId, itsConnection });
	std::cout << "Factory::getConnection: connection created" << std::endl;

    itsConnection->connect(true);
	std::cout << "Factory::getConnection: connection connected" << std::endl;
    return itsConnection;
}

std::shared_ptr<WampConnection>
Factory::getConnection(std::shared_ptr<MainLoopContext> _context) {
    if (!_context)
        return getConnection(DEFAULT_CONNECTION_ID);

    auto itsConnectionIterator = contextConnections_.find(_context.get());
    if (itsConnectionIterator != contextConnections_.end()) {
        return itsConnectionIterator->second;
    }

    // No connection found, lets create and initialize one
    std::shared_ptr<WampConnection> itsConnection
        = std::make_shared<WampConnection>(WampType_t::SESSION, _context->getName());
    contextConnections_.insert({ _context.get(), itsConnection } );

    itsConnection->connect(false);
    if (_context)
        itsConnection->attachMainLoopContext(_context);

    return itsConnection;
}

///////////////////////////////////////////////////////////////////////////////
// Service registration
///////////////////////////////////////////////////////////////////////////////
std::shared_ptr<WampStubAdapter>
Factory::getRegisteredService(const std::string &_address)  {
    auto serviceIterator = services_.find(_address);
    if (serviceIterator != services_.end()) {
        return serviceIterator->second;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// Managed Services
///////////////////////////////////////////////////////////////////////////////
std::shared_ptr<WampStubAdapter>
Factory::createWampStubAdapter(
        const std::shared_ptr<StubBase> &_stub,
        const std::string &_interface,
        const WampAddress &_wampAddress,
        const std::shared_ptr<WampProxyConnection> &_connection) {

    std::shared_ptr<WampStubAdapter> stubAdapter;
    auto stubAdapterCreateFunctionsIterator = stubAdapterCreateFunctions_.find(_interface);
    if (stubAdapterCreateFunctionsIterator != stubAdapterCreateFunctions_.end()) {
        stubAdapter = stubAdapterCreateFunctionsIterator->second(
                        _wampAddress, _connection, _stub);
        if (stubAdapter)
            stubAdapter->init(stubAdapter);
    }
    return stubAdapter;
}

bool
Factory::registerManagedService(const std::shared_ptr<WampStubAdapter> &_stubAdapter) {
    auto itsAddress = _stubAdapter->getAddress().getAddress();

    const auto &insertResult = services_.insert( { itsAddress, _stubAdapter} );
    if (insertResult.second) {
/*TG
        const auto &connection = _stubAdapter->getWampConnection();
        const auto objectManager = connection->getWampObjectManager();
        const bool isRegistered = objectManager->registerWampStubAdapter(_stubAdapter);
        if (!isRegistered) {
            services_.erase(insertResult.first);
            return false;
        }

        const auto &serviceName = _stubAdapter->getWampAddress().getService();
        const bool isAcquired = connection->requestServiceNameAndBlock(serviceName);
        if (!isAcquired) {
            const bool isDeregistered = objectManager->unregisterWampStubAdapter(_stubAdapter);
            assert(isDeregistered);
            (void)isDeregistered;

            services_.erase(insertResult.first);
        }

        return isAcquired;
*/
    	return true;   //TG
    }

    return false;
}


bool
Factory::unregisterManagedService(const std::string &_address) {
    return unregisterManagedService(services_.find(_address));
}

bool
Factory::unregisterManagedService(const ServicesMap::iterator &iterator) {
    if (iterator == services_.end())
        return true;

    const auto &stubAdapter = iterator->second;
    const auto &connection = stubAdapter->getWampConnection();
//TG    const auto objectManager = connection->getWampObjectManager();
    const auto &serviceName = stubAdapter->getWampAddress().getRealm();   //TG
//TG    const auto &serviceName = stubAdapter->getWampAddress().getService();

    const bool isUnregistered
	    = true;   //TG
//TG        = objectManager->unregisterWampStubAdapter(stubAdapter);
    if (isUnregistered) {
        connection->releaseServiceName(serviceName);
        services_.erase(iterator);
    }
    // TODO: log error
    return isUnregistered;
}

} // namespace Wamp
} // namespace CommonAPI
