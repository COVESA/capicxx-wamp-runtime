#include <algorithm>
#include <cassert>
#include <chrono>
#include <future>
#include <sstream>
#include <thread>

#include <CommonAPI/Logger.hpp>
#include <CommonAPI/Wamp/WampConnection.hpp>
//TG   #include <CommonAPI/Wamp/WampInputStream.hpp>
#include <CommonAPI/Wamp/WampMainLoop.hpp>
//TG   #include <CommonAPI/Wamp/WampProxy.hpp>
#include <CommonAPI/Wamp/WampAddressTranslator.hpp>

#include <boost/version.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <tuple>


namespace CommonAPI {
namespace Wamp {

WampConnectionStatusEvent::WampConnectionStatusEvent(WampConnection* wampConnection):
                wampConnection_(wampConnection) {
}

void WampConnectionStatusEvent::onListenerAdded(const Listener &_listener, const Subscription _subscription) {
    (void)_subscription;
    if (wampConnection_->isConnected())
        _listener(AvailabilityStatus::AVAILABLE);
}


/*TG
const DBusObjectPathVTable* WampConnection::getDBusObjectPathVTable() {
    static const DBusObjectPathVTable libdbusObjectPathVTable = {
                    NULL, // no need to handle unregister callbacks
                    &WampConnection::onLibdbusObjectPathMessageThunk,
                    NULL, // dbus_internal_pad1 reserved for future expansion
                    NULL, // dbus_internal_pad2 reserved for future expansion
                    NULL, // dbus_internal_pad3 reserved for future expansion
                    NULL  // dbus_internal_pad4 reserved for future expansion
    };
    return &libdbusObjectPathVTable;
}
*/

//std::bind used to start the dispatch thread holds one reference, and the selfReference
//created within the thread is the second. If only those two remain, no one but the
//dispatch thread references the connection, which therefore can be finished.
const int32_t ownUseCount = 2;

void WampConnection::dispatch() {
	std::cout << "WampConnection::dispatch called" << std::endl;
	io_.run();
	//TG	loop_->run();
	std::cout << "WampConnection::dispatch: finished" << std::endl;
}

WampConnection::WampConnection(WampType_t busType,
                               const ConnectionId_t& _connectionId) :
                dispatchThread_(NULL),
                stopDispatching_(false),
                dispatchSource_(),
                watchContext_(NULL),
                pauseDispatching_(false),
//TG                connection_(NULL),
                busType_(busType),
                wampConnectionStatusEvent_(this),
//TG                libdbusSignalMatchRulesCount_(0),
//TG                dbusObjectMessageHandler_(),
                connectionNameCount_(),
//TG                enforcerThread_(NULL),
//TG                enforcerThreadCancelled_(false),
                connectionId_(_connectionId) {
}

/*TG
WampConnection::WampConnection(::WampConnection *_connection,
                               const ConnectionId_t& _connectionId) :
                dispatchThread_(NULL),
                stopDispatching_(false),
                dispatchSource_(),
                watchContext_(NULL),
                pauseDispatching_(false),
                connection_(_connection),
                busType_(DBusType_t::WRAPPED),
                dbusConnectionStatusEvent_(this),
                libdbusSignalMatchRulesCount_(0),
                dbusObjectMessageHandler_(),
                connectionNameCount_(),
                enforcerThread_(NULL),
                enforcerThreadCancelled_(false),
                connectionId_(_connectionId) {
}

void WampConnection::setObjectPathMessageHandler(DBusObjectPathMessageHandler handler) {
    dbusObjectMessageHandler_ = handler;
}

bool WampConnection::isObjectPathMessageHandlerSet() {
    return dbusObjectMessageHandler_.operator bool();
}
*/

WampConnection::~WampConnection() {
    if (auto lockedContext = mainLoopContext_.lock()) {
/*TG
        if (isConnected()) {
            dbus_connection_set_watch_functions(connection_, NULL, NULL, NULL, NULL, NULL);
            dbus_connection_set_timeout_functions(connection_, NULL, NULL, NULL, NULL, NULL);
        }
*/

        lockedContext->deregisterDispatchSource(dispatchSource_);
        delete watchContext_;
        delete dispatchSource_;
    }

    // ensure, the registry survives until disconnecting is done...
    //std::shared_ptr<DBusServiceRegistry> itsRegistry = DBusServiceRegistry::get(shared_from_this());
    disconnect();

/*TG
    //Assert that the enforcerThread_ is in a position to finish itself correctly even after destruction
    //of the WampConnection. Also assert all resources are cleaned up.
    auto it = timeoutMap_.begin();
    while (it != timeoutMap_.end()) {
        DBusMessageReplyAsyncHandler* asyncHandler = std::get<1>(it->second);
        DBusPendingCall* libdbusPendingCall = it->first;

        asyncHandler->lock();
        bool executionStarted = asyncHandler->getExecutionStarted();
        bool executionFinished = asyncHandler->getExecutionFinished();
        if (executionStarted && !executionFinished) {
            asyncHandler->setHasToBeDeleted();
            it = timeoutMap_.erase(it);
            asyncHandler->unlock();
            continue;
        }
        if (!executionStarted && !executionFinished && !dbus_pending_call_get_completed(libdbusPendingCall)) {
            dbus_pending_call_cancel(libdbusPendingCall);
        }
        asyncHandler->unlock();

        if (!executionStarted && !executionFinished) {
            DBusMessage& dbusMessageCall = std::get<2>(it->second);

            asyncHandler->onDBusMessageReply(CallStatus::REMOTE_ERROR, dbusMessageCall.createMethodError(DBUS_ERROR_TIMEOUT));

            dbus_pending_call_unref(libdbusPendingCall);
        }
        it = timeoutMap_.erase(it);
        delete asyncHandler;
    }

    auto itTimeoutInf = timeoutInfiniteAsyncHandlers_.begin();
    while (itTimeoutInf != timeoutInfiniteAsyncHandlers_.end()) {
        DBusMessageReplyAsyncHandler* asyncHandler = (*itTimeoutInf);

        asyncHandler->lock();
        bool executionStarted = asyncHandler->getExecutionStarted();
        bool executionFinished = asyncHandler->getExecutionFinished();
        if (executionStarted && !executionFinished) {
            asyncHandler->setHasToBeDeleted();
            itTimeoutInf = timeoutInfiniteAsyncHandlers_.erase(itTimeoutInf);
            asyncHandler->unlock();
            continue;
        }
        asyncHandler->unlock();

        itTimeoutInf = timeoutInfiniteAsyncHandlers_.erase(itTimeoutInf);
        delete asyncHandler;
    }
*/
}


bool WampConnection::attachMainLoopContext(std::weak_ptr<MainLoopContext> mainLoopContext) {
    mainLoopContext_ = mainLoopContext;

    if (auto lockedContext = mainLoopContext_.lock()) {
        dispatchSource_ = new WampDispatchSource(this);
        watchContext_ = new WatchContext(mainLoopContext_, dispatchSource_);
        lockedContext->registerDispatchSource(dispatchSource_);

/*TG
        dbus_connection_set_wakeup_main_function(
                        connection_,
                        &WampConnection::onWakeupMainContext,
                        &mainLoopContext_,
                        NULL);

        bool success = 0 != dbus_connection_set_watch_functions(
                connection_,
                &WampConnection::onAddWatch,
                &WampConnection::onRemoveWatch,
                &WampConnection::onToggleWatch,
                watchContext_,
                NULL);

        if (!success) {
            return false;
        }

        success = 0 != dbus_connection_set_timeout_functions(
                connection_,
                &WampConnection::onAddTimeout,
                &WampConnection::onRemoveTimeout,
                &WampConnection::onToggleTimeout,
                &mainLoopContext_,
                NULL);

        if (!success) {
            dbus_connection_set_watch_functions(connection_, NULL, NULL, NULL, NULL, NULL);
            return false;
        }
*/

        return true;
    }
    return false;
}

void WampConnection::onWakeupMainContext(void* data) {
    std::weak_ptr<MainLoopContext>* mainloop = static_cast<std::weak_ptr<MainLoopContext>*>(data);
    assert(mainloop);

    if(auto lockedContext = mainloop->lock()) {
        lockedContext->wakeup();
    }
}

/*TG
dbus_bool_t WampConnection::onAddWatch(::DBusWatch* libdbusWatch, void* data) {
    WatchContext* watchContext = static_cast<WatchContext*>(data);
    assert(watchContext);

    DBusWatch* dbusWatch = new DBusWatch(libdbusWatch, watchContext->mainLoopContext_);
    dbusWatch->addDependentDispatchSource(watchContext->dispatchSource_);
    dbus_watch_set_data(libdbusWatch, dbusWatch, NULL);

    if (dbusWatch->isReadyToBeWatched()) {
        dbusWatch->startWatching();
    }

    return TRUE;
}

void WampConnection::onRemoveWatch(::DBusWatch* libdbusWatch, void* data) {
    assert(static_cast<WatchContext*>(data));
    (void)data;

    DBusWatch* dbusWatch = static_cast<DBusWatch*>(dbus_watch_get_data(libdbusWatch));
    if (dbusWatch != NULL) {
        if(dbusWatch->isReadyToBeWatched()) {
            dbusWatch->stopWatching();
        }
        dbus_watch_set_data(libdbusWatch, NULL, NULL);
    }
    // DBusWatch will be deleted in Mainloop
}

void WampConnection::onToggleWatch(::DBusWatch* libdbusWatch, void* data) {
    WatchContext* watchContext = static_cast<WatchContext*>(data);
    assert(watchContext);

    DBusWatch* dbusWatch = static_cast<DBusWatch*>(dbus_watch_get_data(libdbusWatch));

    if (dbusWatch == NULL) {
        DBusWatch* dbusWatch = new DBusWatch(libdbusWatch, watchContext->mainLoopContext_);
        dbusWatch->addDependentDispatchSource(watchContext->dispatchSource_);
        dbus_watch_set_data(libdbusWatch, dbusWatch, NULL);

        if (dbusWatch->isReadyToBeWatched()) {
            dbusWatch->startWatching();
        }
    } else {
        if (!dbusWatch->isReadyToBeWatched()) {
            dbusWatch->stopWatching();
            dbus_watch_set_data(libdbusWatch, NULL, NULL);
        }
    }
}


dbus_bool_t WampConnection::onAddTimeout(::DBusTimeout* libdbusTimeout, void* data) {
    std::weak_ptr<MainLoopContext>* mainloop = static_cast<std::weak_ptr<MainLoopContext>*>(data);
    assert(mainloop);

    DBusTimeout* dbusTimeout = new DBusTimeout(libdbusTimeout, *mainloop);
    dbus_timeout_set_data(libdbusTimeout, dbusTimeout, NULL);

    if (dbusTimeout->isReadyToBeMonitored()) {
        dbusTimeout->startMonitoring();
    }

    return TRUE;
}

void WampConnection::onRemoveTimeout(::DBusTimeout* libdbusTimeout, void* data) {
    assert(static_cast<std::weak_ptr<MainLoopContext>*>(data));
    (void)data;

    DBusTimeout* dbusTimeout = static_cast<DBusTimeout*>(dbus_timeout_get_data(libdbusTimeout));
    if (dbusTimeout) {
        dbusTimeout->stopMonitoring();
    }
    dbus_timeout_set_data(libdbusTimeout, NULL, NULL);
    // DBusTimeout will be deleted in Mainloop
}

void WampConnection::onToggleTimeout(::DBusTimeout* dbustimeout, void* data) {
    assert(static_cast<std::weak_ptr<MainLoopContext>*>(data));
    (void)data;

    DBusTimeout* timeout = static_cast<DBusTimeout*>(dbus_timeout_get_data(dbustimeout));
    if (timeout->isReadyToBeMonitored()) {
        timeout->startMonitoring();
    } else {
        timeout->stopMonitoring();
    }
}
*/


bool WampConnection::connect(bool startDispatchThread) {
    WampError wampError;
    return connect(wampError, startDispatchThread);
}


static const bool debug = false;
static const std::string realm = "realm1";
static const boost::asio::ip::tcp::endpoint rawsocketEndpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8000);

bool WampConnection::connect(WampError &wampError, bool startDispatchThread) {
    assert(!wampError);
    if (isConnected()) {
        return true;
    }

    bool connected = false;
    try {
        std::cerr << "Boost: " << BOOST_VERSION << std::endl;
        std::cerr << "Connecting to realm: " << realm << std::endl;

        ioMutex_.lock();
        auto transport = std::make_shared<autobahn::wamp_tcp_transport>(
                io_, rawsocketEndpoint, debug);

        session_ = std::make_shared<autobahn::wamp_session>(io_, debug);

        transport->attach(std::static_pointer_cast<autobahn::wamp_transport_handler>(session_));

        connect_future_ = transport->connect().then([&](boost::future<void> connected) {
            try {
                connected.get();
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                io_.stop();
                return;
            }

            std::cerr << "transport connected" << std::endl;

            start_future_ = session_->start().then([&](boost::future<void> started) {
                try {
                    started.get();
                } catch (const std::exception& e) {
                    std::cerr << e.what() << std::endl;
                    io_.stop();
                    return;
                }

                std::cerr << "session started" << std::endl;

                join_future_ = session_->join(realm).then([&](boost::future<uint64_t> joined) {
                    try {
                        ioMutex_.unlock();
                    	connected_ = true;
                        std::cerr << "joined realm: " << joined.get() << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << e.what() << std::endl;
                        io_.stop();
                        return;
                    }
                });

            });
        });

        /*TG
            const ::DBusBusType libdbusType = static_cast<DBusBusType>(busType_);

            connection_ = dbus_bus_get_private(libdbusType, &dbusError.libdbusError_);
            if (dbusError) {
                #ifdef _MSC_VER
                        COMMONAPI_ERROR(std::string(__FUNCTION__) +
                            ": Name: " + dbusError.getName() +
                            " Message: " + dbusError.getMessage())
                #else
                        COMMONAPI_ERROR(std::string(__PRETTY_FUNCTION__) +
                            ": Name: " + dbusError.getName() +
                            " Message: " + dbusError.getMessage())
                #endif
                return false;
            }

            assert(connection_);
            dbus_connection_set_exit_on_disconnect(connection_, false);

            initLibdbusObjectPathHandlerAfterConnect();

            initLibdbusSignalFilterAfterConnect();

            enforcerThread_ = std::make_shared<std::thread>(
                                    std::bind(&WampConnection::enforceAsynchronousTimeouts, shared_from_this()));

            dbusConnectionStatusEvent_.notifyListeners(AvailabilityStatus::AVAILABLE);
        */

        stopDispatching_ = !startDispatchThread;
        if (startDispatchThread) {
            std::shared_ptr<MainLoopContext> itsContext = std::make_shared<MainLoopContext>();
            loop_ = std::make_shared<WampMainLoop>(itsContext);
         	std::cout << "WampConnection::connect: WampMainLoop created" << std::endl;
            attachMainLoopContext(itsContext);
            dispatchThread_ = new std::thread(std::bind(&WampConnection::dispatch, shared_from_this()));
            std::cout << "WampConnection::connect: dispatch thread created" << std::endl;
        }

        connected = true;

/*
        boost::asio::deadline_timer t(io_, boost::posix_time::seconds(5));
        t.wait();

        join_future_.get();
*/

/*
        // busy waiting until the session is started and joined
        while(!connected_);

        boost::future<void> provide_future_add = session_->provide("com.examples.calculator.add2", &add2).then(
            [&](boost::future<autobahn::wamp_registration> registration) {
            try {
                std::cerr << "registered procedure:" << registration.get().id() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                io_.stop();
                return;
            }
        });
*/

    }
    catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        return false;
    }

    return connected;
}

void WampConnection::disconnect() {
    std::lock_guard<std::mutex> wampConnectionLock(connectionGuard_);
    if (isConnected()) {
        wampConnectionStatusEvent_.notifyListeners(AvailabilityStatus::NOT_AVAILABLE);

/*TG
        if (libdbusSignalMatchRulesCount_ > 0) {
            dbus_connection_remove_filter(connection_, &onLibdbusSignalFilterThunk, this);
            libdbusSignalMatchRulesCount_ = 0;
        }
*/

        connectionNameCount_.clear();

        stopDispatching_ = true;

//TG        dbus_connection_close(connection_);

        if(dispatchThread_) {
            loop_->stop();
            //It is possible for the disconnect to be called from within a callback, i.e. from within the dispatch
            //thread. Self-join is prevented this way.
            if (dispatchThread_->joinable() && std::this_thread::get_id() != dispatchThread_->get_id()) {
                dispatchThread_->join();
            } else {
                dispatchThread_->detach();
            }
            delete dispatchThread_;
            dispatchThread_ = NULL;
        }

/*TG
        enforcerThreadCancelled_ = true;
        enforceTimeoutCondition_.notify_one();
        if (enforcerThread_->joinable()) {
            enforcerThread_->join();
        }
        enforcerThreadCancelled_ = false;

        // remote mainloop watchers
        dbus_connection_set_watch_functions(connection_, NULL, NULL, NULL, NULL, NULL);
        dbus_connection_set_timeout_functions(connection_, NULL, NULL, NULL, NULL, NULL);

        dbus_connection_unref(connection_);
        connection_ = nullptr;
*/
    }
}

bool WampConnection::isConnected() const {
	return connected_;
}

WampProxyConnection::ConnectionStatusEvent& WampConnection::getConnectionStatusEvent() {
    return wampConnectionStatusEvent_;
}

/*TG
//Does this need to be a weak pointer?
const std::shared_ptr<DBusObjectManager> WampConnection::getDBusObjectManager() {
    if (!dbusObjectManager_) {
        objectManagerGuard_.lock();
        if (!dbusObjectManager_) {
            dbusObjectManager_ = std::make_shared<DBusObjectManager>(shared_from_this());
        }
        objectManagerGuard_.unlock();
    }
    return dbusObjectManager_;
}
*/

bool WampConnection::requestServiceNameAndBlock(const std::string& serviceName) const {
    WampError wampError;
    bool isServiceNameAcquired = false;

    std::lock_guard<std::mutex> wampConnectionLock(connectionGuard_);
    auto conIter = connectionNameCount_.find(serviceName);
    if (conIter == connectionNameCount_.end()) {
/*TG
        const int libdbusStatus = dbus_bus_request_name(connection_,
                        serviceName.c_str(),
                        DBUS_NAME_FLAG_DO_NOT_QUEUE,
                        &dbusError.libdbusError_);

        isServiceNameAcquired = (libdbusStatus == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);
        if (isServiceNameAcquired) {
            connectionNameCount_.insert( { serviceName, (uint16_t)1 } );
        }
        else {
            if (libdbusStatus == -1) {
                #ifdef _MSC_VER // Visual Studio
                COMMONAPI_ERROR(std::string(__FUNCTION__) +
                    ": Name: " + dbusError.getName() +
                    " Message: " + dbusError.getMessage())
                #else
                COMMONAPI_ERROR(std::string(__PRETTY_FUNCTION__) +
                                ": Name: " + dbusError.getName() +
                                " Message: " + dbusError.getMessage())
                #endif
            }
        }
*/
    } else {
        conIter->second++;
        isServiceNameAcquired = true;
    }

    return isServiceNameAcquired;
}

bool WampConnection::releaseServiceName(const std::string& serviceName) const {
    WampError wampError;
    bool isServiceNameReleased = false;
    std::lock_guard<std::mutex> wampConnectionLock(connectionGuard_);
    auto conIter = connectionNameCount_.find(serviceName);
    if (conIter != connectionNameCount_.end()) {
        if (conIter->second == 1) {
/*TG
            const int libdbusStatus = dbus_bus_release_name(connection_,
                            serviceName.c_str(),
                            &dbusError.libdbusError_);
            isServiceNameReleased = (libdbusStatus == DBUS_RELEASE_NAME_REPLY_RELEASED);
            if (isServiceNameReleased) {
                connectionNameCount_.erase(conIter);
            }
*/
        } else {
            conIter->second--;
            isServiceNameReleased = true;
        }
    }
    return isServiceNameReleased;
}

/*TG
bool WampConnection::sendDBusMessage(const DBusMessage &_message) const {
    assert(_message);
    assert(isConnected());

    dbus_uint32_t dbusSerial;
    bool result = 0 != dbus_connection_send(connection_, _message.message_, &dbusSerial);
    return result;
}

DBusMessage WampConnection::convertToDBusMessage(::DBusPendingCall* _libdbusPendingCall,
        CallStatus& _callStatus) {
    assert(_libdbusPendingCall);

    ::DBusMessage* libdbusMessage = dbus_pending_call_steal_reply(_libdbusPendingCall);
    const bool increaseLibdbusMessageReferenceCount = false;
    DBusMessage dbusMessage(libdbusMessage, increaseLibdbusMessageReferenceCount);
    _callStatus = CallStatus::SUCCESS;

    if (!dbusMessage.isMethodReturnType()) {
        _callStatus = CallStatus::REMOTE_ERROR;
    }

    return dbusMessage;
}

void WampConnection::onLibdbusPendingCallNotifyThunk(::DBusPendingCall* _libdbusPendingCall, void *_userData) {
    assert(_userData);
    assert(_libdbusPendingCall);

    auto dbusMessageReplyAsyncHandler = reinterpret_cast<DBusMessageReplyAsyncHandler*>(_userData);

    dbusMessageReplyAsyncHandler->lock();
    bool processAsyncHandler = !dbusMessageReplyAsyncHandler->getTimeoutOccurred();
    dbusMessageReplyAsyncHandler->setExecutionStarted();
    dbusMessageReplyAsyncHandler->unlock();

    if (processAsyncHandler) {
        DBusMessage dbusMessage;
        CallStatus callStatus;
        dbusMessage = WampConnection::convertToDBusMessage(_libdbusPendingCall, callStatus);

        dbusMessageReplyAsyncHandler->onDBusMessageReply(callStatus, dbusMessage);
    }

    dbusMessageReplyAsyncHandler->lock();
    // libdbus calls the cleanup method below
    dbus_pending_call_unref(_libdbusPendingCall);

    dbusMessageReplyAsyncHandler->setExecutionFinished();
    if (dbusMessageReplyAsyncHandler->hasToBeDeleted()) {
        dbusMessageReplyAsyncHandler->unlock();
        delete dbusMessageReplyAsyncHandler;
        return;
    }
    dbusMessageReplyAsyncHandler->unlock();
}

void WampConnection::onLibdbusDataCleanup(void *_data) {
    // Dummy method -> deleting of userData is not executed in this method. Deleting is
    // executed by handling of the timeouts.
    (void)_data;
}

//Would not be needed if libdbus would actually handle its timeouts for pending calls.
void WampConnection::enforceAsynchronousTimeouts() const {
    std::unique_lock<std::mutex> itsLock(enforcerThreadMutex_);

    while (!enforcerThreadCancelled_) {
        enforceTimeoutMutex_.lock();

        int timeout = std::numeric_limits<int>::max(); // not really, but nearly "forever"
        if (timeoutMap_.size() > 0) {
            auto minTimeoutElement = std::min_element(timeoutMap_.begin(), timeoutMap_.end(),
                    [] (const TimeoutMapElement& lhs, const TimeoutMapElement& rhs) {
                        return std::get<0>(lhs.second) < std::get<0>(rhs.second);
            });

            auto minTimeout = std::get<0>(minTimeoutElement->second);

            std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

            timeout = (int)std::chrono::duration_cast<std::chrono::milliseconds>(minTimeout - now).count();
        }

        enforceTimeoutMutex_.unlock();

        if (std::cv_status::timeout ==
            enforceTimeoutCondition_.wait_for(itsLock, std::chrono::milliseconds(timeout))) {

            //Do not access members if the WampConnection was destroyed during the unlocked phase.
            enforceTimeoutMutex_.lock();
            auto it = timeoutMap_.begin();
            while (it != timeoutMap_.end()) {
                std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

                DBusMessageReplyAsyncHandler* asyncHandler = std::get<1>(it->second);
                DBusPendingCall* libdbusPendingCall = it->first;

                if (now > std::get<0>(it->second)) {

                    asyncHandler->lock();
                    bool executionStarted = asyncHandler->getExecutionStarted();
                    bool executionFinished = asyncHandler->getExecutionFinished();
                    if (!executionStarted && !executionFinished) {
                        asyncHandler->setTimeoutOccurred();
                        if (!dbus_pending_call_get_completed(libdbusPendingCall)) {
                            dbus_pending_call_cancel(libdbusPendingCall);
                        }
                    }
                    asyncHandler->unlock();

                    if (executionStarted && !executionFinished) {
                        // execution of asyncHandler is still running
                        // ==> add 100 ms for next timeout check
                        std::get<0>(it->second) = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(100);
                    } else {
                        if (!executionFinished) {
                            // execution of asyncHandler was not finished (and not started)
                            // => add asyncHandler to mainloopTimeouts list
                            DBusMessage& dbusMessageCall = std::get<2>(it->second);

                            assert(mainLoopContext_.lock());
                            {
                                std::lock_guard<std::mutex> itsLock(mainloopTimeoutsMutex_);
                                mainloopTimeouts_.push_back(std::make_tuple(asyncHandler,
                                    dbusMessageCall.createMethodError(DBUS_ERROR_TIMEOUT),
                                    CallStatus::REMOTE_ERROR,
                                    nullptr));
                            }
                            mainLoopContext_.lock()->wakeup();
                            it = timeoutMap_.erase(it);

                            //This unref MIGHT cause the destruction of the last callback object that references the WampConnection.
                            //So after this unref has been called, it has to be ensured that continuation of the loop is an option.
                            dbus_pending_call_unref(libdbusPendingCall);
                        } else {
                            // execution of asyncHandler was finished
                            it = timeoutMap_.erase(it);
                            delete asyncHandler;
                        }
                    }
                } else {
                    asyncHandler->lock();
                    bool executionFinished = asyncHandler->getExecutionFinished();
                    asyncHandler->unlock();
                    if (executionFinished) {
                        // execution of asyncHandler was finished but timeout is not expired
                        it = timeoutMap_.erase(it);
                        delete asyncHandler;
                    } else {
                        ++it;
                    }
                }
            }
            enforceTimeoutMutex_.unlock();
        } else {
            std::lock_guard<std::mutex> itsLock(enforceTimeoutMutex_);

            auto it = timeoutMap_.begin();
            while (it != timeoutMap_.end()) {
                DBusMessageReplyAsyncHandler* asyncHandler = std::get<1>(it->second);
                asyncHandler->lock();
                bool executionFinished = asyncHandler->getExecutionFinished();
                asyncHandler->unlock();
                if (executionFinished) {
                    // execution of asyncHandler was finished but timeout is not expired
                    it = timeoutMap_.erase(it);
                    delete asyncHandler;
                } else {
                    ++it;
                }
            }
        }

        {
            std::lock_guard<std::mutex> itsLock(timeoutInfiniteAsyncHandlersMutex_);
            // check for asyncHandler with infinite timeout whose execution is finished
            auto it = timeoutInfiniteAsyncHandlers_.begin();
            while (it != timeoutInfiniteAsyncHandlers_.end()) {
                DBusMessageReplyAsyncHandler* asyncHandler = (*it);
                asyncHandler->lock();
                bool executionFinished = asyncHandler->getExecutionFinished();
                asyncHandler->unlock();
                if ( executionFinished ) {
                    it = timeoutInfiniteAsyncHandlers_.erase(it);
                    delete asyncHandler;
                } else {
                    it++;
                }
            }
        }

    }
}

std::future<CallStatus> WampConnection::sendDBusMessageWithReplyAsync(
        const DBusMessage& dbusMessage,
        std::unique_ptr<DBusMessageReplyAsyncHandler> dbusMessageReplyAsyncHandler,
        const CommonAPI::CallInfo *_info) const {

    assert(dbusMessage);
    assert(isConnected());

    DBusPendingCall* libdbusPendingCall;
    dbus_bool_t libdbusSuccess;

    DBusMessageReplyAsyncHandler* replyAsyncHandler = dbusMessageReplyAsyncHandler.release();

    std::future<CallStatus> callStatusFuture = replyAsyncHandler->getFuture();

    libdbusSuccess = dbus_connection_send_with_reply_set_notify(connection_,
            dbusMessage.message_,
            &libdbusPendingCall,
            onLibdbusPendingCallNotifyThunk,
            replyAsyncHandler,
            onLibdbusDataCleanup,
            _info->timeout_);

    if (_info->sender_ != 0) {
        COMMONAPI_DEBUG("Message sent: SenderID: ", _info->sender_, " - Serial number: ", dbusMessage.getSerial());
    }

    if (!libdbusSuccess || !libdbusPendingCall) {
        #ifdef _MSC_VER // Visual Studio
            COMMONAPI_ERROR(std::string(__FUNCTION__) +
                ": (!libdbusSuccess || !libdbusPendingCall) == true")
        #else
            COMMONAPI_ERROR(std::string(__PRETTY_FUNCTION__) +
                            ": (!libdbusSuccess || !libdbusPendingCall) == true")
        #endif
        if (libdbusPendingCall) {
            dbus_pending_call_unref(libdbusPendingCall);
        }
        {
            std::lock_guard<std::mutex> itsLock(mainloopTimeoutsMutex_);
            mainloopTimeouts_.push_back(std::make_tuple(replyAsyncHandler,
                dbusMessage.createMethodError(DBUS_ERROR_DISCONNECTED),
                CallStatus::CONNECTION_FAILED,
                nullptr));
        }
        mainLoopContext_.lock()->wakeup();
        return callStatusFuture;
    }

    if (_info->timeout_ != DBUS_TIMEOUT_INFINITE) {
        auto timeoutPoint = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(_info->timeout_);
        std::tuple<
            std::chrono::time_point<std::chrono::high_resolution_clock>,
            DBusMessageReplyAsyncHandler*,
            DBusMessage> toInsert {
                timeoutPoint,
                replyAsyncHandler,
                dbusMessage
            };

        enforceTimeoutMutex_.lock();
        timeoutMap_.insert( { libdbusPendingCall, toInsert } );
        enforceTimeoutMutex_.unlock();

        enforcerThreadMutex_.lock();
        enforceTimeoutCondition_.notify_all();
        enforcerThreadMutex_.unlock();
    } else {
        // add asyncHandler with infinite timeout to corresponding list
        std::lock_guard<std::mutex> itsLock(timeoutInfiniteAsyncHandlersMutex_);
        timeoutInfiniteAsyncHandlers_.insert(replyAsyncHandler);
    }

    return callStatusFuture;
}

DBusMessage WampConnection::sendDBusMessageWithReplyAndBlock(const DBusMessage& dbusMessage,
                                                             DBusError& dbusError,
                                                             const CommonAPI::CallInfo *_info) const {
    assert(dbusMessage);
    assert(!dbusError);
    assert(isConnected());

    ::DBusMessage* libdbusMessageReply = dbus_connection_send_with_reply_and_block(connection_,
                                                                                   dbusMessage.message_,
                                                                                   _info->timeout_,
                                                                                   &dbusError.libdbusError_);

    if (_info->sender_ != 0) {
        COMMONAPI_DEBUG("Message sent: SenderID: ", _info->sender_, " - Serial number: ", dbusMessage.getSerial());
    }

    if (dbusError) {
        return DBusMessage();
    }

    const bool increaseLibdbusMessageReferenceCount = false;
    return DBusMessage(libdbusMessageReply, increaseLibdbusMessageReferenceCount);
}
*/

bool WampConnection::singleDispatch() {
    {
        std::lock_guard<std::mutex> itsLock(mainloopTimeoutsMutex_);
        for (auto t : mainloopTimeouts_) {
/*TG
            std::get<0>(t)->onDBusMessageReply(std::get<2>(t), std::get<1>(t));
            if (std::get<3>(t) != nullptr) {
                dbus_pending_call_unref(std::get<3>(t));
            }
            delete std::get<0>(t);
*/
        }
        mainloopTimeouts_.clear();
    }

    return true;   //TG: TODO
//TG    return (dbus_connection_dispatch(connection_) == DBUS_DISPATCH_DATA_REMAINS);
}

bool WampConnection::isDispatchReady() {
    std::lock_guard<std::mutex> itsLock(mainloopTimeoutsMutex_);
    return false;   //TG: TODO
/*TG
    return (dbus_connection_get_dispatch_status(connection_) == DBUS_DISPATCH_DATA_REMAINS ||
            !mainloopTimeouts_.empty());
*/
}

/*TG
bool WampConnection::hasDispatchThread() {
    return (dispatchThread_ != NULL);
}
*/

const ConnectionId_t& WampConnection::getConnectionId() const {
    return connectionId_;
}

/*TG
bool WampConnection::sendPendingSelectiveSubscription(DBusProxy* proxy, std::string methodName) {
    bool subscriptionAccepted;
    CommonAPI::CallStatus callStatus;
        DBusProxyHelper<CommonAPI::DBus::DBusSerializableArguments<>,
                        CommonAPI::DBus::DBusSerializableArguments<bool>>::callMethodWithReply(
                                *proxy, methodName.c_str(), "", &CommonAPI::DBus::defaultCallInfo, callStatus, subscriptionAccepted);

    return subscriptionAccepted;
}

DBusProxyConnection::DBusSignalHandlerToken WampConnection::subscribeForSelectiveBroadcast(
                    bool& subscriptionAccepted,
                    const std::string& objectPath,
                    const std::string& interfaceName,
                    const std::string& interfaceMemberName,
                    const std::string& interfaceMemberSignature,
                    DBusSignalHandler* dbusSignalHandler,
                    DBusProxy* callingProxy) {

    std::string methodName = "subscribeFor" + interfaceMemberName + "Selective";

    subscriptionAccepted = false;

    CommonAPI::CallStatus callStatus;
    DBusProxyHelper<CommonAPI::DBus::DBusSerializableArguments<>,
                    CommonAPI::DBus::DBusSerializableArguments<bool>>::callMethodWithReply(
                    *callingProxy, methodName.c_str(), "", &CommonAPI::DBus::defaultCallInfo, callStatus, subscriptionAccepted);

    DBusProxyConnection::DBusSignalHandlerToken subscriptionToken;
    if ((callStatus == CommonAPI::CallStatus::SUCCESS && subscriptionAccepted) || !callingProxy->isAvailable()) {
        subscriptionToken = addSignalMemberHandler(
                                objectPath,
                                interfaceName,
                                interfaceMemberName,
                                interfaceMemberSignature,
                                dbusSignalHandler,
                                true
                            );
        subscriptionAccepted = true;
    }

    return (subscriptionToken);
}

void WampConnection::unsubscribeFromSelectiveBroadcast(const std::string& eventName,
                                                      DBusProxyConnection::DBusSignalHandlerToken subscription,
                                                      DBusProxy* callingProxy,
                                                      const DBusSignalHandler* dbusSignalHandler) {
    bool lastListenerOnConnectionRemoved = removeSignalMemberHandler(subscription, dbusSignalHandler);

    if (lastListenerOnConnectionRemoved) {
        // send unsubscribe message to stub
        std::string methodName = "unsubscribeFrom" + eventName + "Selective";
        CommonAPI::CallStatus callStatus;
        DBusProxyHelper<CommonAPI::DBus::DBusSerializableArguments<>,
                        CommonAPI::DBus::DBusSerializableArguments<>>::callMethodWithReply(
                        *callingProxy, methodName.c_str(), "", &CommonAPI::DBus::defaultCallInfo, callStatus);
    }
}

DBusProxyConnection::DBusSignalHandlerToken WampConnection::addSignalMemberHandler(const std::string& objectPath,
                                                                                   const std::string& interfaceName,
                                                                                   const std::string& interfaceMemberName,
                                                                                   const std::string& interfaceMemberSignature,
                                                                                   DBusSignalHandler* dbusSignalHandler,
                                                                                   const bool justAddFilter) {
    DBusSignalHandlerPath dbusSignalHandlerPath(
                    objectPath,
                    interfaceName,
                    interfaceMemberName,
                    interfaceMemberSignature);
    std::lock_guard < std::mutex > dbusSignalLock(signalGuard_);

    auto signalEntry = dbusSignalHandlerTable_.find(dbusSignalHandlerPath);
    const bool isFirstSignalMemberHandler = (signalEntry == dbusSignalHandlerTable_.end());
    if (isFirstSignalMemberHandler) {
        addLibdbusSignalMatchRule(objectPath, interfaceName, interfaceMemberName, justAddFilter);
        std::set<DBusSignalHandler*> handlerList;
        handlerList.insert(dbusSignalHandler);

        dbusSignalHandlerTable_.insert( {
            dbusSignalHandlerPath,
            std::make_pair(std::make_shared<std::recursive_mutex>(), std::move(handlerList))
        } );
    } else {
        signalEntry->second.first->lock();
        signalEntry->second.second.insert(dbusSignalHandler);
        signalEntry->second.first->unlock();
    }

    return dbusSignalHandlerPath;
}

bool WampConnection::removeSignalMemberHandler(const DBusSignalHandlerToken &dbusSignalHandlerToken,
                                               const DBusSignalHandler *dbusSignalHandler) {
    bool lastHandlerRemoved = false;
    std::lock_guard < std::mutex > dbusSignalLock(signalGuard_);

    auto signalEntry = dbusSignalHandlerTable_.find(dbusSignalHandlerToken);
    if (signalEntry != dbusSignalHandlerTable_.end()) {

        signalEntry->second.first->lock();
        auto selectedHandler = signalEntry->second.second.find(const_cast<DBusSignalHandler*>(dbusSignalHandler));
        if (selectedHandler != signalEntry->second.second.end()) {
            signalEntry->second.second.erase(selectedHandler);
            lastHandlerRemoved = (signalEntry->second.second.empty());
        }
        signalEntry->second.first->unlock();
    }

    if (lastHandlerRemoved) {
        dbusSignalHandlerTable_.erase(signalEntry);
        removeLibdbusSignalMatchRule(std::get<0>(dbusSignalHandlerToken),
                std::get<1>(dbusSignalHandlerToken),
                std::get<2>(dbusSignalHandlerToken));
    }

    return lastHandlerRemoved;
}

bool WampConnection::addObjectManagerSignalMemberHandler(const std::string& dbusBusName,
                                                         DBusSignalHandler* dbusSignalHandler) {
    if (dbusBusName.length() < 2) {
        return false;
    }

    std::lock_guard<std::mutex> dbusSignalLock(dbusObjectManagerSignalGuard_);

    auto dbusSignalMatchRuleIterator = dbusObjectManagerSignalMatchRulesMap_.find(dbusBusName);
    const bool isDBusSignalMatchRuleFound = (dbusSignalMatchRuleIterator != dbusObjectManagerSignalMatchRulesMap_.end());

    if (!isDBusSignalMatchRuleFound) {
        if (isConnected() && !addObjectManagerSignalMatchRule(dbusBusName)) {
            return false;
        }

        auto insertResult = dbusObjectManagerSignalMatchRulesMap_.insert({ dbusBusName, 0 });
        const bool isInsertSuccessful = insertResult.second;

        if (!isInsertSuccessful) {
            if (isConnected()) {
                const bool isRemoveSignalMatchRuleSuccessful = removeObjectManagerSignalMatchRule(dbusBusName);
                assert(isRemoveSignalMatchRuleSuccessful);
                (void)isRemoveSignalMatchRuleSuccessful;
            }
            return false;
        }

        dbusSignalMatchRuleIterator = insertResult.first;
    }

    size_t &dbusSignalMatchRuleReferenceCount = dbusSignalMatchRuleIterator->second;
    dbusSignalMatchRuleReferenceCount++;
    dbusObjectManagerSignalHandlerTable_.insert( { dbusBusName, dbusSignalHandler } );

    return true;
}

bool WampConnection::removeObjectManagerSignalMemberHandler(const std::string& dbusBusName,
                                                            DBusSignalHandler* dbusSignalHandler) {
    assert(!dbusBusName.empty());

    std::lock_guard<std::mutex> dbusSignalLock(dbusObjectManagerSignalGuard_);

    auto dbusSignalMatchRuleIterator = dbusObjectManagerSignalMatchRulesMap_.find(dbusBusName);
    const bool isDBusSignalMatchRuleFound = (dbusSignalMatchRuleIterator != dbusObjectManagerSignalMatchRulesMap_.end());

    if (!isDBusSignalMatchRuleFound) {
        return true;
    }

    auto dbusObjectManagerSignalHandlerRange = dbusObjectManagerSignalHandlerTable_.equal_range(dbusBusName);
    auto dbusObjectManagerSignalHandlerIterator = std::find_if(
        dbusObjectManagerSignalHandlerRange.first,
        dbusObjectManagerSignalHandlerRange.second,
        [&](decltype(*dbusObjectManagerSignalHandlerRange.first)& it) { return it.second == dbusSignalHandler; });
    const bool isDBusSignalHandlerFound = (dbusObjectManagerSignalHandlerIterator != dbusObjectManagerSignalHandlerRange.second);
    if (!isDBusSignalHandlerFound) {
        return false;
    }

    size_t& dbusSignalMatchRuleReferenceCount = dbusSignalMatchRuleIterator->second;

    assert(dbusSignalMatchRuleReferenceCount > 0);
    dbusSignalMatchRuleReferenceCount--;

    const bool isLastDBusSignalMatchRuleReference = (dbusSignalMatchRuleReferenceCount == 0);
    if (isLastDBusSignalMatchRuleReference) {
        if (isConnected() && !removeObjectManagerSignalMatchRule(dbusBusName)) {
            return false;
        }

        dbusObjectManagerSignalMatchRulesMap_.erase(dbusSignalMatchRuleIterator);
    }

    dbusObjectManagerSignalHandlerTable_.erase(dbusObjectManagerSignalHandlerIterator);

    return true;
}

bool WampConnection::addObjectManagerSignalMatchRule(const std::string& dbusBusName) {
    std::ostringstream dbusMatchRuleStringStream;
    dbusMatchRuleStringStream << "type='signal'"
                                << ",sender='" << dbusBusName << "'"
                                << ",interface='org.freedesktop.DBus.ObjectManager'";
    return addLibdbusSignalMatchRule(dbusMatchRuleStringStream.str());
}

bool WampConnection::removeObjectManagerSignalMatchRule(const std::string& dbusBusName) {
    std::ostringstream dbusMatchRuleStringStream;
    dbusMatchRuleStringStream << "type='signal'"
                                << ",sender='" << dbusBusName << "'"
                                << ",interface='org.freedesktop.DBus.ObjectManager'";
    return removeLibdbusSignalMatchRule(dbusMatchRuleStringStream.str());
}
*/

/**
 * Called only if connected
 *
 * @param dbusMatchRule
 * @return
 */
/*TG
bool WampConnection::addLibdbusSignalMatchRule(const std::string& dbusMatchRule) {
    bool libdbusSuccess = true;

    // add the libdbus message signal filter
    if (!libdbusSignalMatchRulesCount_) {
        libdbusSuccess = 0 != dbus_connection_add_filter(
            connection_,
            &onLibdbusSignalFilterThunk,
            this,
            NULL
        );
    }

    // finally add the match rule
    if (libdbusSuccess) {
        DBusError dbusError;
        dbus_bus_add_match(connection_, dbusMatchRule.c_str(), &dbusError.libdbusError_);
        libdbusSuccess = !dbusError;
    }

    if (libdbusSuccess) {
        libdbusSignalMatchRulesCount_++;
    }

    return libdbusSuccess;
}
*/

/**
 * Called only if connected
 *
 * @param dbusMatchRule
 * @return
 */
/*TG
bool WampConnection::removeLibdbusSignalMatchRule(const std::string& dbusMatchRule) {
    if(libdbusSignalMatchRulesCount_ == 0)
        return true;

    dbus_bus_remove_match(connection_, dbusMatchRule.c_str(), NULL);

    libdbusSignalMatchRulesCount_--;
    if (libdbusSignalMatchRulesCount_ == 0) {
        dbus_connection_remove_filter(connection_, &onLibdbusSignalFilterThunk, this);
    }

    return true;
}

void WampConnection::registerObjectPath(const std::string& objectPath) {
    assert(!objectPath.empty());
    assert(objectPath[0] == '/');

    auto handlerIterator = libdbusRegisteredObjectPaths_.find(objectPath);
    const bool foundRegisteredObjectPathHandler = handlerIterator != libdbusRegisteredObjectPaths_.end();

    if (foundRegisteredObjectPathHandler) {
        uint32_t& referenceCount = handlerIterator->second;
        referenceCount++;
        return;
    }

    libdbusRegisteredObjectPaths_.insert(LibdbusRegisteredObjectPathHandlersTable::value_type(objectPath, 1));

    if (isConnected()) {
        DBusError dbusError;
        const dbus_bool_t libdbusSuccess = dbus_connection_try_register_object_path(connection_,
                                                                                    objectPath.c_str(),
                                                                                    getDBusObjectPathVTable(),
                                                                                    this,
                                                                                    &dbusError.libdbusError_);
        assert(libdbusSuccess);
        assert(!dbusError);
        (void)libdbusSuccess;
        (void)dbusError;
    }
}

void WampConnection::unregisterObjectPath(const std::string& objectPath) {
    assert(!objectPath.empty());
    assert(objectPath[0] == '/');

    auto handlerIterator = libdbusRegisteredObjectPaths_.find(objectPath);
    const bool foundRegisteredObjectPathHandler = handlerIterator != libdbusRegisteredObjectPaths_.end();

    assert(foundRegisteredObjectPathHandler);
    (void)foundRegisteredObjectPathHandler;

    uint32_t& referenceCount = handlerIterator->second;
    if (referenceCount > 1) {
        referenceCount--;
        return;
    }

    libdbusRegisteredObjectPaths_.erase(handlerIterator);

    if (isConnected()) {
        dbus_bool_t libdbusSuccess
            = dbus_connection_unregister_object_path(connection_, objectPath.c_str());
        assert(libdbusSuccess);
        (void)libdbusSuccess;
    }
}

void WampConnection::addLibdbusSignalMatchRule(const std::string& objectPath,
                                               const std::string& interfaceName,
                                               const std::string& interfaceMemberName,
                                               const bool justAddFilter) {
    DBusSignalMatchRuleTuple dbusSignalMatchRuleTuple(objectPath, interfaceName, interfaceMemberName);
    auto matchRuleIterator = dbusSignalMatchRulesMap_.find(dbusSignalMatchRuleTuple);
    const bool matchRuleFound = matchRuleIterator != dbusSignalMatchRulesMap_.end();

    if (matchRuleFound) {
        uint32_t& matchRuleReferenceCount = matchRuleIterator->second.first;
        matchRuleReferenceCount++;
        return;
    }

    const bool isFirstMatchRule = dbusSignalMatchRulesMap_.empty();

    // generate D-Bus match rule string
    std::ostringstream matchRuleStringStream;

    matchRuleStringStream << "type='signal'";
    matchRuleStringStream << ",path='" << objectPath << "'";
    matchRuleStringStream << ",interface='" << interfaceName << "'";
    matchRuleStringStream << ",member='" << interfaceMemberName << "'";

    // add the match rule string to the map with reference count set to 1
    std::string matchRuleString = matchRuleStringStream.str();
    auto success = dbusSignalMatchRulesMap_.insert(
                    DBusSignalMatchRulesMap::value_type(dbusSignalMatchRuleTuple,
                                    DBusSignalMatchRuleMapping(1, matchRuleString)));
    assert(success.second);
    (void)success;

    if (isConnected()) {
        bool libdbusSuccess = true;

        // add the libdbus message signal filter
        if (isFirstMatchRule) {

            libdbusSuccess = 0 != dbus_connection_add_filter(
                                connection_,
                                &onLibdbusSignalFilterThunk,
                                this,
                                NULL);
            assert(libdbusSuccess);
        }

        if (!justAddFilter)
        {
            // finally add the match rule
            DBusError dbusError;
            dbus_bus_add_match(connection_, matchRuleString.c_str(), &dbusError.libdbusError_);
            assert(!dbusError);
            (void)dbusError;
        }

        if (libdbusSuccess) {
            libdbusSignalMatchRulesCount_++;
        }
    }
}

void WampConnection::removeLibdbusSignalMatchRule(const std::string& objectPath,
                                                  const std::string& interfaceName,
                                                  const std::string& interfaceMemberName) {
    auto selfReference = this->shared_from_this();
    DBusSignalMatchRuleTuple dbusSignalMatchRuleTuple(objectPath, interfaceName, interfaceMemberName);
    auto matchRuleIterator = dbusSignalMatchRulesMap_.find(dbusSignalMatchRuleTuple);
    const bool matchRuleFound = matchRuleIterator != dbusSignalMatchRulesMap_.end();

    assert(matchRuleFound);
    (void)matchRuleFound;

    uint32_t& matchRuleReferenceCount = matchRuleIterator->second.first;
    if (matchRuleReferenceCount > 1) {
        matchRuleReferenceCount--;
        return;
    }

    if (isConnected()) {
        const std::string& matchRuleString = matchRuleIterator->second.second;
        const bool libdbusSuccess = removeLibdbusSignalMatchRule(matchRuleString);
        assert(libdbusSuccess);
        (void)libdbusSuccess;
    }

    dbusSignalMatchRulesMap_.erase(matchRuleIterator);
}

void WampConnection::initLibdbusObjectPathHandlerAfterConnect() {
    assert(isConnected());

    // nothing to do if there aren't any registered object path handlers
    if (libdbusRegisteredObjectPaths_.empty()) {
        return;
    }

    DBusError dbusError;
    dbus_bool_t libdbusSuccess;

    for (auto handlerIterator = libdbusRegisteredObjectPaths_.begin();
         handlerIterator != libdbusRegisteredObjectPaths_.end();
         handlerIterator++) {
        const std::string& objectPath = handlerIterator->first;

        dbusError.clear();

        libdbusSuccess = dbus_connection_try_register_object_path(connection_,
                                                                  objectPath.c_str(),
                                                                  getDBusObjectPathVTable(),
                                                                  this,
                                                                  &dbusError.libdbusError_);
        assert(libdbusSuccess);
        (void)libdbusSuccess;

        assert(!dbusError);
    }
}

void WampConnection::initLibdbusSignalFilterAfterConnect() {
    assert(isConnected());

    // proxy/stub match rules
    for (const auto& dbusSignalMatchRuleIterator : dbusSignalMatchRulesMap_) {
        const auto& dbusSignalMatchRuleMapping = dbusSignalMatchRuleIterator.second;
        const std::string& dbusMatchRuleString = dbusSignalMatchRuleMapping.second;
        const bool libdbusSuccess = addLibdbusSignalMatchRule(dbusMatchRuleString);
        assert(libdbusSuccess);
        (void)libdbusSuccess;
    }

    // object manager match rules (see DBusServiceRegistry)
    for (const auto& dbusObjectManagerSignalMatchRuleIterator : dbusObjectManagerSignalMatchRulesMap_) {
        const std::string& dbusBusName = dbusObjectManagerSignalMatchRuleIterator.first;
        const bool libdbusSuccess = addObjectManagerSignalMatchRule(dbusBusName);
        assert(libdbusSuccess);
        (void)libdbusSuccess;
    }
}

::DBusHandlerResult WampConnection::onLibdbusObjectPathMessage(::DBusMessage* libdbusMessage) {
    assert(libdbusMessage);
    (void)libdbusMessage;

    // handle only method call messages
    if (dbus_message_get_type(libdbusMessage) != DBUS_MESSAGE_TYPE_METHOD_CALL) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    bool isDBusMessageHandled = dbusObjectMessageHandler_(DBusMessage(libdbusMessage));
    return isDBusMessageHandled ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

template<typename DBusSignalHandlersTable>
void notifyDBusSignalHandlers(DBusSignalHandlersTable& dbusSignalHandlerstable,
                              typename DBusSignalHandlersTable::iterator& signalEntry,
                              const CommonAPI::DBus::DBusMessage& dbusMessage,
                              ::DBusHandlerResult& dbusHandlerResult) {
    if (signalEntry == dbusSignalHandlerstable.end() || signalEntry->second.second.empty()) {
        dbusHandlerResult = DBUS_HANDLER_RESULT_HANDLED;
        return;
    }

    signalEntry->second.first->lock();
    auto handlerEntry = signalEntry->second.second.begin();
    while (handlerEntry != signalEntry->second.second.end()) {
        DBusProxyConnection::DBusSignalHandler* dbusSignalHandler = *handlerEntry;
        dbusSignalHandler->onSignalDBusMessage(dbusMessage);
        handlerEntry++;
    }
    dbusHandlerResult = DBUS_HANDLER_RESULT_HANDLED;
    signalEntry->second.first->unlock();
}

template<typename DBusSignalHandlersTable>
void notifyDBusOMSignalHandlers(DBusSignalHandlersTable& dbusSignalHandlerstable,
                              std::pair<typename DBusSignalHandlersTable::iterator,
                                        typename DBusSignalHandlersTable::iterator>& equalRange,
                              const CommonAPI::DBus::DBusMessage &dbusMessage,
                              ::DBusHandlerResult &dbusHandlerResult) {
    (void)dbusSignalHandlerstable;

    if (equalRange.first != equalRange.second) {
        dbusHandlerResult = DBUS_HANDLER_RESULT_HANDLED;
    }
    while (equalRange.first != equalRange.second) {
        DBusProxyConnection::DBusSignalHandler* dbusSignalHandler = equalRange.first->second;
        dbusSignalHandler->onSignalDBusMessage(dbusMessage);
        equalRange.first++;
    }
}

::DBusHandlerResult WampConnection::onLibdbusSignalFilter(::DBusMessage* libdbusMessage) {
    assert(libdbusMessage);

    auto selfReference = this->shared_from_this();

    // handle only signal messages
    if (dbus_message_get_type(libdbusMessage) != DBUS_MESSAGE_TYPE_SIGNAL) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    const char* objectPath = dbus_message_get_path(libdbusMessage);
    const char* interfaceName = dbus_message_get_interface(libdbusMessage);
    const char* interfaceMemberName = dbus_message_get_member(libdbusMessage);
    const char* interfaceMemberSignature = dbus_message_get_signature(libdbusMessage);

    assert(objectPath);
    assert(interfaceName);
    assert(interfaceMemberName);
    assert(interfaceMemberSignature);

    DBusMessage dbusMessage(libdbusMessage);
    ::DBusHandlerResult dbusHandlerResult = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    signalGuard_.lock();
    auto signalEntry = dbusSignalHandlerTable_.find(DBusSignalHandlerPath(
        objectPath,
        interfaceName,
        interfaceMemberName,
        interfaceMemberSignature));
    signalGuard_.unlock();

    notifyDBusSignalHandlers(dbusSignalHandlerTable_,
                             signalEntry, dbusMessage, dbusHandlerResult);

    if (dbusMessage.hasInterfaceName("org.freedesktop.DBus.ObjectManager")) {
        const char* dbusSenderName = dbusMessage.getSender();
        assert(dbusSenderName);

        dbusObjectManagerSignalGuard_.lock();
        auto dbusObjectManagerSignalHandlerIteratorPair = dbusObjectManagerSignalHandlerTable_.equal_range(dbusSenderName);
        notifyDBusOMSignalHandlers(dbusObjectManagerSignalHandlerTable_,
                                 dbusObjectManagerSignalHandlerIteratorPair,
                                 dbusMessage,
                                 dbusHandlerResult);
        dbusObjectManagerSignalGuard_.unlock();
    }

    return dbusHandlerResult;
}

::DBusHandlerResult WampConnection::onLibdbusSignalFilterThunk(::WampConnection *_dbusConnection,
                                                               ::DBusMessage* libdbusMessage,
                                                               void* userData) {
    assert(_dbusConnection);
    assert(libdbusMessage);
    assert(userData);
    (void)_dbusConnection;

    WampConnection* dbusConnection = reinterpret_cast<WampConnection*>(userData);
    assert(dbusConnection->connection_ == _dbusConnection);
    return dbusConnection->onLibdbusSignalFilter(libdbusMessage);
}

::DBusHandlerResult WampConnection::onLibdbusObjectPathMessageThunk(::WampConnection *_dbusConnection,
                                                                    ::DBusMessage* libdbusMessage,
                                                                    void* userData) {
    assert(_dbusConnection);
    assert(libdbusMessage);
    assert(userData);
    (void)_dbusConnection;

    WampConnection* dbusConnection = reinterpret_cast<WampConnection*>(userData);
    assert(dbusConnection->connection_ == _dbusConnection);
    return dbusConnection->onLibdbusObjectPathMessage(libdbusMessage);
}
*/

std::shared_ptr<WampConnection> WampConnection::getBus(const WampType_t &_type, const ConnectionId_t& _connectionId) {
    return std::make_shared<WampConnection>(_type, _connectionId);
}

/*
std::shared_ptr<WampConnection> WampConnection::wrap(::WampConnection *_connection, const ConnectionId_t& _connectionId) {
    return std::make_shared<WampConnection>(_connection, _connectionId);
}
*/

} // namespace Wamp
} // namespace CommonAPI
