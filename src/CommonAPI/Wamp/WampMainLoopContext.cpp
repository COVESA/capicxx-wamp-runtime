#ifdef WIN32
#include <WinSock2.h>
#else
#include <poll.h>
#endif

#include <cassert>
#include <chrono>

#include <CommonAPI/Wamp/WampMainLoopContext.hpp>
#include <CommonAPI/Wamp/WampConnection.hpp>

namespace CommonAPI {
namespace Wamp {

WampDispatchSource::WampDispatchSource(WampConnection* wampConnection):
    wampConnection_(wampConnection) {
}

WampDispatchSource::~WampDispatchSource() {
}

bool WampDispatchSource::prepare(int64_t &_timeout) {
    (void)_timeout;
    return wampConnection_->isDispatchReady();
}

bool WampDispatchSource::check() {
    return wampConnection_->isDispatchReady();
}

bool WampDispatchSource::dispatch() {
    return wampConnection_->singleDispatch();
}


/*TG
DBusWatch::DBusWatch(::DBusWatch* libdbusWatch, std::weak_ptr<MainLoopContext>& mainLoopContext):
                libdbusWatch_(libdbusWatch),
                mainLoopContext_(mainLoopContext) {
    assert(libdbusWatch_);
}
*/

bool WampWatch::isReadyToBeWatched() {
	return false;   //TG: TODO
//TG    return 0 != dbus_watch_get_enabled(libdbusWatch_);
}

void WampWatch::startWatching() {
/*TG
    if(!dbus_watch_get_enabled(libdbusWatch_)) stopWatching();

    unsigned int channelFlags_ = dbus_watch_get_flags(libdbusWatch_);
*/
#ifdef WIN32
    short int pollFlags = 0;
#else
    short int pollFlags = POLLERR | POLLHUP;
#endif
/*TG
    if(channelFlags_ & DBUS_WATCH_READABLE) {
        pollFlags |= POLLIN;
    }
    if(channelFlags_ & DBUS_WATCH_WRITABLE) {
        pollFlags |= POLLOUT;
    }

#ifdef WIN32
    pollFileDescriptor_.fd = dbus_watch_get_socket(libdbusWatch_);
    wsaEvent_ = WSACreateEvent();
    WSAEventSelect(pollFileDescriptor_.fd, wsaEvent_, FD_READ);
#else
    pollFileDescriptor_.fd = dbus_watch_get_unix_fd(libdbusWatch_);
#endif
*/

    pollFileDescriptor_.events = pollFlags;
    pollFileDescriptor_.revents = 0;

    auto lockedContext = mainLoopContext_.lock();
    assert(lockedContext);
    lockedContext->registerWatch(this);
}

void WampWatch::stopWatching() {
    auto lockedContext = mainLoopContext_.lock();
    if (lockedContext) {
        lockedContext->deregisterWatch(this);
    }
}

const pollfd& WampWatch::getAssociatedFileDescriptor() {
    return pollFileDescriptor_;
}

#ifdef WIN32
const HANDLE& WampWatch::getAssociatedEvent() {
    return wsaEvent_;
}
#endif

void WampWatch::dispatch(unsigned int eventFlags) {
#ifdef WIN32
    unsigned int wampWatchFlags = 0;

    if (eventFlags & (POLLRDBAND | POLLRDNORM)) {
        wampWatchFlags |= WAMP_WATCH_READABLE;
    }
    if (eventFlags & POLLWRNORM) {
        wampWatchFlags |= WAMP_WATCH_WRITABLE;
    }
    if (eventFlags & (POLLERR | POLLNVAL)) {
        wampWatchFlags |= WAMP_WATCH_ERROR;
    }
    if (eventFlags & POLLHUP) {
        wampWatchFlags |= WAMP_WATCH_HANGUP;
    }
#else
    // Pollflags do not correspond directly to DBus watch flags
    unsigned int wampWatchFlags = (eventFlags & POLLIN) |
                            ((eventFlags & POLLOUT) >> 1) |
                            ((eventFlags & POLLERR) >> 1) |
                            ((eventFlags & POLLHUP) >> 1);
#endif
/*TG
    dbus_bool_t response = dbus_watch_handle(libdbusWatch_, dbusWatchFlags);

    if (!response) {
        printf("dbus_watch_handle returned FALSE!");
    }
*/
}

const std::vector<DispatchSource*>& WampWatch::getDependentDispatchSources() {
    return dependentDispatchSources_;
}

void WampWatch::addDependentDispatchSource(DispatchSource* dispatchSource) {
    dependentDispatchSources_.push_back(dispatchSource);
}


/*TG
DBusTimeout::DBusTimeout(::DBusTimeout* libdbusTimeout, std::weak_ptr<MainLoopContext>& mainLoopContext) :
                dueTimeInMs_(TIMEOUT_INFINITE),
                libdbusTimeout_(libdbusTimeout),
                mainLoopContext_(mainLoopContext) {
}
*/

bool WampTimeout::isReadyToBeMonitored() {
	return false;   //TG: TODO
//TG    return 0 != dbus_timeout_get_enabled(libdbusTimeout_);
}

void WampTimeout::startMonitoring() {
    auto lockedContext = mainLoopContext_.lock();
    assert(lockedContext);
    recalculateDueTime();
    lockedContext->registerTimeoutSource(this);
}

void WampTimeout::stopMonitoring() {
    dueTimeInMs_ = TIMEOUT_INFINITE;
    auto lockedContext = mainLoopContext_.lock();
    if (lockedContext) {
        lockedContext->deregisterTimeoutSource(this);
    }
}

bool WampTimeout::dispatch() {
    recalculateDueTime();
//TG: TODO    dbus_timeout_handle(libdbusTimeout_);
    return true;
}

int64_t WampTimeout::getTimeoutInterval() const {
    return 0;   //TG: TODO
//TG    return dbus_timeout_get_interval(libdbusTimeout_);
}

int64_t WampTimeout::getReadyTime() const {
    return dueTimeInMs_;
}

void WampTimeout::recalculateDueTime() {
/*TG: TODO
    if(dbus_timeout_get_enabled(libdbusTimeout_)) {
        unsigned int intervalInMs = dbus_timeout_get_interval(libdbusTimeout_);
        dueTimeInMs_ = getCurrentTimeInMs() + intervalInMs;
    } else {
        dueTimeInMs_ = TIMEOUT_INFINITE;
    }
*/
}

} // namespace Wamp
} // namespace CommonAPI
