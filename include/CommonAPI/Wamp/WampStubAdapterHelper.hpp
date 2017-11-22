#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_WAMP_STUBADAPTERHELPER_HPP_
#define COMMONAPI_WAMP_STUBADAPTERHELPER_HPP_

#include <initializer_list>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <map>

#include <CommonAPI/Variant.hpp>
#include <CommonAPI/Wamp/WampFunctionalHash.hpp>
#include <CommonAPI/Wamp/WampStubAdapter.hpp>
//TG   #include <CommonAPI/Wamp/WampInputStream.hpp>
//TG   #include <CommonAPI/Wamp/WampOutputStream.hpp>
#include <CommonAPI/Wamp/WampHelper.hpp>
#include <CommonAPI/Wamp/WampConnection.hpp>
//#include <CommonAPI/Wamp/WampSerializableArguments.hpp>
//TG   #include <CommonAPI/Wamp/WampClientId.hpp>

namespace CommonAPI {
namespace Wamp {

class StubDispatcherBase {
public:
   virtual ~StubDispatcherBase() { }
};




struct WampAttributeDispatcherStruct {
    StubDispatcherBase* getter;
    StubDispatcherBase* setter;

    WampAttributeDispatcherStruct(StubDispatcherBase* g, StubDispatcherBase* s) {
        getter = g;
        setter = s;
    }
};

typedef std::unordered_map<std::string, WampAttributeDispatcherStruct> StubAttributeTable;

template <typename StubClass_>
class WampStubAdapterHelper: public virtual WampStubAdapter {
 public:
    typedef typename StubClass_::StubAdapterType StubAdapterType;
    typedef typename StubClass_::RemoteEventHandlerType RemoteEventHandlerType;

    class StubDispatcher: public StubDispatcherBase {
    public:
        virtual ~StubDispatcher() {}
/*TG
        virtual bool dispatchDBusMessage(const DBusMessage &_message,
                                         const std::shared_ptr<StubClass_> &_stub,
                                         DBusStubAdapterHelper<StubClass_> &_helper) = 0;
        virtual void appendGetAllReply(const DBusMessage &_message,
                                       const std::shared_ptr<StubClass_> &_stub,
                                       DBusStubAdapterHelper<StubClass_> &_helper,
                                       DBusOutputStream &_output) {
            (void)_message;
            (void)_stub;
            (void)_helper;
            (void)_output;
        }
*/
    };
    // interfaceMemberName, interfaceMemberSignature
    typedef std::pair<const char*, const char*> WampInterfaceMemberPath;
    typedef std::unordered_map<WampInterfaceMemberPath, StubDispatcherBase*> StubDispatcherTable;

    WampStubAdapterHelper(const WampAddress &_address,
                          const std::shared_ptr<WampProxyConnection> &_connection,
                          const std::shared_ptr<StubClass_> &_stub,
                          const bool _isManaging):
                    WampStubAdapter(_address, _connection, _isManaging),
                    stub_(_stub),
                    remoteEventHandler_(nullptr) {
    }

    virtual ~WampStubAdapterHelper() {
        WampStubAdapter::deinit();
        stub_.reset();
    }

    virtual void init(std::shared_ptr<WampStubAdapter> instance) {
    	std::cout << "WampStubAdapterHelper::init()" << std::endl;
        WampStubAdapter::init(instance);
        std::shared_ptr<StubAdapterType> stubAdapter = std::dynamic_pointer_cast<StubAdapterType>(instance);
        remoteEventHandler_ = stub_->initStubAdapter(stubAdapter);
    }

    virtual void deinit() {
        WampStubAdapter::deinit();
        stub_.reset();
    }

    inline RemoteEventHandlerType* getRemoteEventHandler() {
        return remoteEventHandler_;
    }

 protected:

    virtual const StubDispatcherTable& getStubDispatcherTable() = 0;
    virtual const StubAttributeTable& getStubAttributeTable() = 0;

    std::shared_ptr<StubClass_> stub_;
    RemoteEventHandlerType* remoteEventHandler_;

 private:
};


//template<class>
//struct WampStubTopicHelper;

//template<template<class ...> class In_, class... InArgs_>
//struct WampStubTopicHelper<In_<InArgs_...>> {
struct WampStubTopicHelper {

    template <typename WampStub_ = WampStubAdapter, typename PayloadTuple>
    static inline bool publishTopic(
    		const WampStub_ &_stub,
			const std::string topicName,
			const PayloadTuple payload
	) {
    	std::cout << "publishTopic '" << topicName << "'" << std::endl;

    	// busy waiting until the session is started and joined
    	// TODO: this should be changed - see https://github.com/GENIVI/capicxx-wamp-runtime/issues/7
    	while(!_stub.getWampConnection()->isConnected());

    	CommonAPI::Wamp::WampConnection* connection = (CommonAPI::Wamp::WampConnection*)(_stub.getWampConnection().get());
    	connection->ioMutex_.lock();
    	connection->session_->publish(topicName, payload);
    	connection->ioMutex_.unlock();

    	return true;
    }
};



template< class, class, class >
class WampMethodStubDispatcher;

template <
    typename StubClass_,
    template <class...> class In_, class... InArgs_,
    template <class...> class DeplIn_, class... DeplIn_Args>

class WampMethodStubDispatcher<StubClass_, In_<InArgs_...>, DeplIn_<DeplIn_Args...> >: public WampStubAdapterHelper<StubClass_>::StubDispatcher {
 public:
    typedef WampStubAdapterHelper<StubClass_> WampStubAdapterHelperType;
    typedef void (StubClass_::*StubFunctor_)(std::shared_ptr<CommonAPI::ClientId>, InArgs_...);

    WampMethodStubDispatcher(StubFunctor_ stubFunctor, std::tuple<DeplIn_Args*...> _in):
            stubFunctor_(stubFunctor) {
            initialize(typename make_sequence_range<sizeof...(DeplIn_Args), 0>::type(), _in);
    }

/*TG
    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<StubClass_>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        return handleDBusMessage(dbusMessage, stub, dbusStubAdapterHelper, typename make_sequence_range<sizeof...(InArgs_), 0>::type());
    }
*/

 private:
    template <int... DeplIn_ArgIndices>
    inline void initialize(index_sequence<DeplIn_ArgIndices...>, std::tuple<DeplIn_Args*...> &_in) {
        in_ = std::make_tuple(std::get<DeplIn_ArgIndices>(_in)...);
    }

/*TG
    template <int... InArgIndices_>
    inline bool handleDBusMessage(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<StubClass_>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  index_sequence<InArgIndices_...>) {
        (void)dbusStubAdapterHelper;

        if (sizeof...(InArgs_) > 0) {
            DBusInputStream dbusInputStream(dbusMessage);
            const bool success = DBusSerializableArguments<CommonAPI::Deployable<InArgs_, DeplIn_Args>...>::deserialize(dbusInputStream, std::get<InArgIndices_>(in_)...);
            if (!success)
                return false;
        }

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));

        (stub.get()->*stubFunctor_)(clientId, std::move(std::get<InArgIndices_>(in_).getValue())...);

        return true;
    }
*/

    StubFunctor_ stubFunctor_;
    std::tuple<CommonAPI::Deployable<InArgs_, DeplIn_Args>...> in_;
};


template<class>
class WampMethodWithReplyStubDispatcher;

template <typename StubClass_>
class WampMethodWithReplyStubDispatcher {
public:
	typedef void (StubClass_::*MethodWrapper)(autobahn::wamp_invocation invocation);

    template <typename WampStub_ = WampStubAdapter>
	static inline bool provideRemoteMethod(
			const WampStub_ &_stub,
			const char* methodName,
			MethodWrapper wrapper
	) {
		// busy waiting until the session is started and joined
		while(!_stub.getWampConnection()->isConnected());

		CommonAPI::Wamp::WampConnection* connection = (CommonAPI::Wamp::WampConnection*)(_stub.getWampConnection().get());
		connection->ioMutex_.lock();

		std::string identifier = _stub.getWampAddress().getRealm() + "." + methodName;
		boost::future<void> provide_future = connection->session_->provide(identifier,
				std::bind(wrapper, _stub, std::placeholders::_1))
			.then([&](boost::future<autobahn::wamp_registration> registration) {
			try {
				std::cerr << "registered procedure " << identifier << ": id=" << registration.get().id() << std::endl;
			} catch (const std::exception& e) {
				std::cerr << e.what() << std::endl;
				connection->io_.stop();
				return;
			}
		});
		provide_future.get();

		connection->ioMutex_.unlock();
		return true;
	}
};


/*
template< class>
class WampMethodWithReplyStubDispatcher;

template <
    typename StubClass_,
    template <class...> class In_, class... InArgs_,
    template <class...> class Out_, class... OutArgs_>
class WampMethodWithReplyStubDispatcher<
       StubClass_,
       In_<InArgs_...>,
       Out_<OutArgs_...> > :
            public WampStubAdapterHelper<StubClass_>::StubDispatcher {
 public:
    typedef WampStubAdapterHelper<StubClass_> WampStubAdapterHelperType;
    typedef std::function<void (OutArgs_...)> ReplyType_t;
    typedef void (StubClass_::*StubFunctor_)(
                std::shared_ptr<CommonAPI::ClientId>, InArgs_..., ReplyType_t);

    WampMethodWithReplyStubDispatcher(StubFunctor_ stubFunctor) :
            stubFunctor_(stubFunctor),
            out_(_outDepArgs),
            currentCall_(0) {

        initialize(typename make_sequence_range<sizeof...(DeplIn_Args), 0>::type(), _inDepArgs);

    }

    bool sendReply(CommonAPI::CallId_t _call, 
                       std::tuple<CommonAPI::Deployable<OutArgs_, DeplOutArgs_>...> args = std::make_tuple()) {
        return sendReplyInternal(_call, typename make_sequence_range<sizeof...(OutArgs_), 0>::type(), args);
    }

private:

    template <int... DeplIn_ArgIndices>
    inline void initialize(index_sequence<DeplIn_ArgIndices...>, std::tuple<DeplIn_Args*...> &_in) {
        in_ = std::make_tuple(std::get<DeplIn_ArgIndices>(_in)...);
    }

*/
/*TG
    template <int... InArgIndices_, int... OutArgIndices_>
    inline bool handleDBusMessage(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<StubClass_>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  index_sequence<InArgIndices_...>,
                                  index_sequence<OutArgIndices_...>) {
        (void)dbusStubAdapterHelper;
        if (sizeof...(DeplIn_Args) > 0) {
            DBusInputStream dbusInputStream(dbusMessage);
            const bool success = DBusSerializableArguments<CommonAPI::Deployable<InArgs_, DeplIn_Args>...>::deserialize(dbusInputStream, std::get<InArgIndices_>(in_)...);
            if (!success)
                return false;
        }

        std::shared_ptr<DBusClientId> clientId
            = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));
        DBusMessage reply = dbusMessage.createMethodReturn(dbusReplySignature_);

        CommonAPI::CallId_t call;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            call = currentCall_++;
            pending_[call] = reply;
        }

        (stub.get()->*stubFunctor_)(
            clientId,
            std::move(std::get<InArgIndices_>(in_).getValue())...,
            [call, this](OutArgs_... _args){
                this->sendReply(call, std::make_tuple(CommonAPI::Deployable<OutArgs_, DeplOutArgs_>(
                            _args, std::get<OutArgIndices_>(out_)
                        )...));
            }
        );

           return true;
    }
*/

/*
    template<int... OutArgIndices_>
    bool sendReplyInternal(CommonAPI::CallId_t _call,
                           index_sequence<OutArgIndices_...>,
                           std::tuple<CommonAPI::Deployable<OutArgs_, DeplOutArgs_>...> args) {
        std::lock_guard<std::mutex> lock(mutex_);
*/
/*TG
        auto reply = pending_.find(_call);
        if (reply != pending_.end()) {
            if (sizeof...(DeplOutArgs_) > 0) {
                DBusOutputStream output(reply->second);
                if (!DBusSerializableArguments<CommonAPI::Deployable<OutArgs_, DeplOutArgs_>...>::serialize(
                        output, std::get<OutArgIndices_>(args)...)) {
                    (void)args;
                    pending_.erase(_call);
                    return false;
                }
                output.flush();
            }
            bool isSuccessful = connection_->sendDBusMessage(reply->second);
            pending_.erase(_call);
            return isSuccessful;
        }
*/
 /*       return false;
    }

    StubFunctor_ stubFunctor_;
    const char* wampReplySignature_;

    std::tuple<CommonAPI::Deployable<InArgs_, DeplIn_Args>...> in_;
    std::tuple<DeplOutArgs_*...> out_;
    CommonAPI::CallId_t currentCall_;
//TG    std::map<CommonAPI::CallId_t, DBusMessage> pending_;
    std::mutex mutex_; // protects pending_

    std::shared_ptr<WampProxyConnection> connection_;
};
*/


template< class, class, class, class >
class WampMethodWithReplyAdapterDispatcher;

template <
    typename StubClass_,
    typename StubAdapterClass_,
    template <class...> class In_, class... InArgs_,
    template <class...> class Out_, class... OutArgs_>
class WampMethodWithReplyAdapterDispatcher<StubClass_, StubAdapterClass_, In_<InArgs_...>, Out_<OutArgs_...> >:
            public WampStubAdapterHelper<StubClass_>::StubDispatcher {
 public:
    typedef WampStubAdapterHelper<StubClass_> WampStubAdapterHelperType;
    typedef void (StubAdapterClass_::*StubFunctor_)(std::shared_ptr<CommonAPI::ClientId>, InArgs_..., OutArgs_&...);
    typedef typename CommonAPI::Stub<typename WampStubAdapterHelperType::StubAdapterType, typename StubClass_::RemoteEventType> StubType;

    WampMethodWithReplyAdapterDispatcher(StubFunctor_ stubFunctor, const char* wampReplySignature):
            stubFunctor_(stubFunctor),
            wampReplySignature_(wampReplySignature) {
    }

/*TG
    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<StubClass_>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        std::tuple<InArgs_..., OutArgs_...> argTuple;
        return handleDBusMessage(
                        dbusMessage,
                        stub,
                        dbusStubAdapterHelper,
                        typename make_sequence_range<sizeof...(InArgs_), 0>::type(),
                        typename make_sequence_range<sizeof...(OutArgs_), sizeof...(InArgs_)>::type(),argTuple);
    }
*/

 private:
/*TG
    template <int... InArgIndices_, int... OutArgIndices_>
    inline bool handleDBusMessage(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<StubClass_>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  index_sequence<InArgIndices_...>,
                                  index_sequence<OutArgIndices_...>,
                                  std::tuple<InArgs_..., OutArgs_...> argTuple) const {
        (void)argTuple;

        if (sizeof...(InArgs_) > 0) {
            DBusInputStream dbusInputStream(dbusMessage);
            const bool success = DBusSerializableArguments<InArgs_...>::deserialize(dbusInputStream, std::get<InArgIndices_>(argTuple)...);
            if (!success)
                return false;
        }

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));

        (stub->StubType::getStubAdapter().get()->*stubFunctor_)(clientId, std::move(std::get<InArgIndices_>(argTuple))..., std::get<OutArgIndices_>(argTuple)...);
        DBusMessage dbusMessageReply = dbusMessage.createMethodReturn(dbusReplySignature_);

        if (sizeof...(OutArgs_) > 0) {
            DBusOutputStream dbusOutputStream(dbusMessageReply);
            const bool success = DBusSerializableArguments<OutArgs_...>::serialize(dbusOutputStream, std::get<OutArgIndices_>(argTuple)...);
            if (!success)
                return false;

            dbusOutputStream.flush();
        }

        return dbusStubAdapterHelper.getDBusConnection()->sendDBusMessage(dbusMessageReply);
    }
*/

    StubFunctor_ stubFunctor_;
    const char* wampReplySignature_;
};


template <typename StubClass_, typename AttributeType_, typename AttributeDepl_ = EmptyDeployment>
class WampGetAttributeStubDispatcher: public virtual WampStubAdapterHelper<StubClass_>::StubDispatcher {
 public:
    typedef WampStubAdapterHelper<StubClass_> WampStubAdapterHelperType;
    typedef const AttributeType_& (StubClass_::*GetStubFunctor)(std::shared_ptr<CommonAPI::ClientId>);

    WampGetAttributeStubDispatcher(GetStubFunctor _getStubFunctor, const char *_signature, AttributeDepl_ *_depl = nullptr):
        getStubFunctor_(_getStubFunctor),
        signature_(_signature),
        depl_(_depl) {
    }

    virtual ~WampGetAttributeStubDispatcher() {};

/*TG
    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<StubClass_>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        return sendAttributeValueReply(dbusMessage, stub, dbusStubAdapterHelper);
    }

    void appendGetAllReply(const DBusMessage& dbusMessage, const std::shared_ptr<StubClass_>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper, DBusOutputStream &_output) {
        (void)dbusStubAdapterHelper;

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));
        auto varDepl = CommonAPI::DBus::VariantDeployment<AttributeDepl_>(true, depl_); // presuming FreeDesktop variant deployment, as support for "legacy" service only
        _output << CommonAPI::Deployable<CommonAPI::Variant<AttributeType_>, CommonAPI::DBus::VariantDeployment<AttributeDepl_>>((stub.get()->*getStubFunctor_)(clientId), &varDepl);
        _output.flush();
    }
*/

 protected:
/*TG
    virtual bool sendAttributeValueReply(const DBusMessage& dbusMessage, const std::shared_ptr<StubClass_>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        DBusMessage dbusMessageReply = dbusMessage.createMethodReturn(signature_);
        DBusOutputStream dbusOutputStream(dbusMessageReply);

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));

        dbusOutputStream << CommonAPI::Deployable<AttributeType_, AttributeDepl_>((stub.get()->*getStubFunctor_)(clientId), depl_);
        dbusOutputStream.flush();

        return dbusStubAdapterHelper.getDBusConnection()->sendDBusMessage(dbusMessageReply);
    }
*/

    GetStubFunctor getStubFunctor_;
    const char* signature_;
    AttributeDepl_ *depl_;
};

template <typename StubClass_, typename AttributeType_, typename AttributeDepl_ = EmptyDeployment>
class WampSetAttributeStubDispatcher: public virtual WampGetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_> {
 public:
    typedef typename WampGetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_>::WampStubAdapterHelperType WampStubAdapterHelperType;
    typedef typename WampStubAdapterHelperType::RemoteEventHandlerType RemoteEventHandlerType;

    typedef typename WampGetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_>::GetStubFunctor GetStubFunctor;
    typedef bool (RemoteEventHandlerType::*OnRemoteSetFunctor)(std::shared_ptr<CommonAPI::ClientId>, AttributeType_);
    typedef void (RemoteEventHandlerType::*OnRemoteChangedFunctor)();

    WampSetAttributeStubDispatcher(GetStubFunctor getStubFunctor,
                                   OnRemoteSetFunctor onRemoteSetFunctor,
                                   OnRemoteChangedFunctor onRemoteChangedFunctor,
                                   const char* wampSignature,
                                   AttributeDepl_ *_depl = nullptr) :
                    WampGetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_>(getStubFunctor, wampSignature, _depl),
                    onRemoteSetFunctor_(onRemoteSetFunctor),
                    onRemoteChangedFunctor_(onRemoteChangedFunctor) {
    }

    virtual ~WampSetAttributeStubDispatcher() {};

/*TG
    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<StubClass_>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        bool attributeValueChanged;

        if (!setAttributeValue(dbusMessage, stub, dbusStubAdapterHelper, attributeValueChanged))
            return false;

        if (attributeValueChanged)
            notifyOnRemoteChanged(dbusStubAdapterHelper);

        return true;
    }
*/

 protected:
/*TG
    virtual AttributeType_ retrieveAttributeValue(const DBusMessage& dbusMessage, bool& errorOccured) {
        errorOccured = false;

        DBusInputStream dbusInputStream(dbusMessage);
        CommonAPI::Deployable<AttributeType_, AttributeDepl_> attributeValue(this->depl_);
        dbusInputStream >> attributeValue;

        if (dbusInputStream.hasError()) {
            errorOccured = true;
        }

        return attributeValue.getValue();
    }

    inline bool setAttributeValue(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<StubClass_>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  bool& attributeValueChanged) {
        bool errorOccured;
        CommonAPI::Deployable<AttributeType_, AttributeDepl_> attributeValue(
             retrieveAttributeValue(dbusMessage, errorOccured), this->depl_);

        if(errorOccured) {
            return false;
        }

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));

        attributeValueChanged = (dbusStubAdapterHelper.getRemoteEventHandler()->*onRemoteSetFunctor_)(clientId, std::move(attributeValue.getValue()));

        return this->sendAttributeValueReply(dbusMessage, stub, dbusStubAdapterHelper);
    }
*/

    inline void notifyOnRemoteChanged(WampStubAdapterHelperType& wampStubAdapterHelper) {
        (wampStubAdapterHelper.getRemoteEventHandler()->*onRemoteChangedFunctor_)();
    }

    inline const AttributeType_& getAttributeValue(std::shared_ptr<CommonAPI::ClientId> clientId, const std::shared_ptr<StubClass_>& stub) {
        return (stub.get()->*(this->getStubFunctor_))(clientId);
    }

    const OnRemoteSetFunctor onRemoteSetFunctor_;
    const OnRemoteChangedFunctor onRemoteChangedFunctor_;
};

template <typename StubClass_, typename AttributeType_, typename AttributeDepl_ = EmptyDeployment>
class WampSetObservableAttributeStubDispatcher: public virtual WampSetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_> {
 public:
    typedef typename WampSetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_>::WampStubAdapterHelperType WampStubAdapterHelperType;
    typedef typename WampStubAdapterHelperType::StubAdapterType StubAdapterType;
    typedef typename WampSetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_>::GetStubFunctor GetStubFunctor;
    typedef typename WampSetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_>::OnRemoteSetFunctor OnRemoteSetFunctor;
    typedef typename WampSetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_>::OnRemoteChangedFunctor OnRemoteChangedFunctor;
    typedef typename CommonAPI::Stub<StubAdapterType, typename StubClass_::RemoteEventType> StubType;
    typedef void (StubAdapterType::*FireChangedFunctor)(const AttributeType_&);

    WampSetObservableAttributeStubDispatcher(GetStubFunctor getStubFunctor,
                                             OnRemoteSetFunctor onRemoteSetFunctor,
                                             OnRemoteChangedFunctor onRemoteChangedFunctor,
                                             FireChangedFunctor fireChangedFunctor,
                                             const char* wampSignature,
                                             AttributeDepl_ *_depl = nullptr)
        : WampGetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_>(
                getStubFunctor, wampSignature, _depl),
          WampSetAttributeStubDispatcher<StubClass_, AttributeType_, AttributeDepl_>(
                getStubFunctor, onRemoteSetFunctor, onRemoteChangedFunctor, wampSignature, _depl),
          fireChangedFunctor_(fireChangedFunctor) {
    }

    virtual ~WampSetObservableAttributeStubDispatcher() {};

/*TG
    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<StubClass_>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        bool attributeValueChanged;
        if (!this->setAttributeValue(dbusMessage, stub, dbusStubAdapterHelper, attributeValueChanged))
            return false;

        if (attributeValueChanged) {
            std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));
            fireAttributeValueChanged(clientId, dbusStubAdapterHelper, stub);
            this->notifyOnRemoteChanged(dbusStubAdapterHelper);
        }
        return true;
    }
*/
protected:
    virtual void fireAttributeValueChanged(std::shared_ptr<CommonAPI::ClientId> _client,
                                           WampStubAdapterHelperType &_helper,
                                           const std::shared_ptr<StubClass_> _stub) {
        (void)_helper;
        (_stub->StubType::getStubAdapter().get()->*fireChangedFunctor_)(this->getAttributeValue(_client, _stub));
    }

    const FireChangedFunctor fireChangedFunctor_;
};

} // namespace Wamp
} // namespace CommonAPI

#endif // COMMONAPI_WAMP_STUBADAPTERHELPER_HPP_
