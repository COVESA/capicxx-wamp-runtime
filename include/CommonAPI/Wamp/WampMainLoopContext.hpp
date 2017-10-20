#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_WAMP_MAINLOOPCONTEXT_HPP_
#define COMMONAPI_WAMP_MAINLOOPCONTEXT_HPP_

#include <list>
#include <memory>

#include <CommonAPI/MainLoopContext.hpp>

namespace CommonAPI {
namespace Wamp {

class WampConnection;

class WampDispatchSource: public DispatchSource {
 public:
    WampDispatchSource(WampConnection  *wampConnection);
    ~WampDispatchSource();

    bool prepare(int64_t& timeout);
    bool check();
    bool dispatch();

 private:
    WampConnection *wampConnection_;
};

class WampWatch: public Watch {
 public:
/*
    WampWatch(::DBusWatch* libdbusWatch, std::weak_ptr<MainLoopContext>& mainLoopContext);
*/

    bool isReadyToBeWatched();
    void startWatching();
    void stopWatching();

    void dispatch(unsigned int eventFlags);

    const pollfd& getAssociatedFileDescriptor();

#ifdef WIN32
    const HANDLE& getAssociatedEvent();
#endif

    const std::vector<DispatchSource*>& getDependentDispatchSources();
    void addDependentDispatchSource(DispatchSource* dispatchSource);
 private:
    bool isReady();

/*
    ::DBusWatch* libdbusWatch_;
*/
    pollfd pollFileDescriptor_;
    std::vector<DispatchSource*> dependentDispatchSources_;

    std::weak_ptr<MainLoopContext> mainLoopContext_;

#ifdef WIN32
    HANDLE wsaEvent_;
#endif
};


class WampTimeout: public Timeout {
 public:
/*
    WampTimeout(::DBusTimeout* libdbusTimeout, std::weak_ptr<MainLoopContext>& mainLoopContext);
*/

    bool isReadyToBeMonitored();
    void startMonitoring();
    void stopMonitoring();

    bool dispatch();

    int64_t getTimeoutInterval() const;
    int64_t getReadyTime() const;
 private:
    void recalculateDueTime();

    int64_t dueTimeInMs_;
/*
    ::DBusTimeout* libdbusTimeout_;
*/
    std::weak_ptr<MainLoopContext> mainLoopContext_;
};


} // namespace Wamp
} // namespace CommonAPI

#endif // COMMONAPI_WAMP_MAINLOOPCONTEXT_HPP_
