#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <sys/stat.h>

#include <algorithm>

#include <CommonAPI/IniFileReader.hpp>
#include <CommonAPI/Logger.hpp>
#include <CommonAPI/Runtime.hpp>
#include <CommonAPI/Wamp/WampAddressTranslator.hpp>

namespace CommonAPI {
namespace Wamp {

const char *COMMONAPI_WAMP_DEFAULT_CONFIG_FILE = "commonapi-wamp.ini";
const char *COMMONAPI_WAMP_DEFAULT_CONFIG_FOLDER = "/etc/";


std::shared_ptr<WampAddressTranslator> WampAddressTranslator::get() {
    static std::shared_ptr<WampAddressTranslator> theTranslator = std::make_shared<WampAddressTranslator>();
    return theTranslator;
}

WampAddressTranslator::WampAddressTranslator()
    : defaultDomain_("local") {
    init();

//TG    isDefault_ = ("wamp" == Runtime::get()->getDefaultBinding());
    isDefault_ = true;   //TG
}

void
WampAddressTranslator::init() {
    // Determine default configuration file
    const char *config = getenv("COMMONAPI_WAMP_CONFIG");
    if (config) {
        defaultConfig_ = config;
    } else {
        defaultConfig_ = COMMONAPI_WAMP_DEFAULT_CONFIG_FOLDER;
        defaultConfig_ += "/";
        defaultConfig_ += COMMONAPI_WAMP_DEFAULT_CONFIG_FILE;
    }

//TG    (void)readConfiguration();
}

bool
WampAddressTranslator::translate(const std::string &_key, WampAddress &_value) {
    return translate(CommonAPI::Address(_key), _value);
}

bool
WampAddressTranslator::translate(const CommonAPI::Address &_key, WampAddress &_value) {
    bool result(true);
    std::lock_guard<std::mutex> itsLock(mutex_);

    const auto it = forwards_.find(_key);
    if (it != forwards_.end()) {
        _value = it->second;
    } else if (isDefault_) {
    	_value.setRealm(_key.getAddress());
        forwards_.insert({ _key, _value });
        backwards_.insert({ _value, _key });
    } else {
        result = false;
    }

    return result;
}

bool
WampAddressTranslator::translate(const WampAddress &_key, std::string &_value) {
    CommonAPI::Address address;
    if (translate(_key, address)) {
        _value = address.getAddress();
        return true;
    }
    return false;
}

bool
WampAddressTranslator::translate(const WampAddress &_key, CommonAPI::Address &_value) {
    bool result(true);
    std::lock_guard<std::mutex> itsLock(mutex_);

    const auto it = backwards_.find(_key);
    if (it != backwards_.end()) {
        _value = it->second;
    } else if (isDefault_) {
    	_value.setAddress(_key.getRealm());
        forwards_.insert({_value, _key});
        backwards_.insert({_key, _value});
    } else {
        result = false;
    }

    return result;
}

/*TG
void
WampAddressTranslator::insert(
        const std::string &_address,
        const std::string &_service, const std::string &_path, const std::string &_interface, const bool _objPathStartWithDigits) {

    if (isValid(_service, '.',
            (_service.length() > 0 && _service[0] == ':'),
            (_service.length() > 0 && _service[0] == ':'),
            true)
      && isValid(_path, '/', true, _objPathStartWithDigits)
      && isValid(_interface, '.')) {
        CommonAPI::Address address(_address);
        DBusAddress dbusAddress(_service, _path, _interface);

        std::lock_guard<std::mutex> itsLock(mutex_);
        auto fw = forwards_.find(address);
        auto bw = backwards_.find(dbusAddress);
        if (fw == forwards_.end() && bw == backwards_.end()) {
            forwards_[address] = dbusAddress;
            backwards_[dbusAddress] = address;
            COMMONAPI_DEBUG(
                "Added address mapping: ", address, " <--> ", dbusAddress);
            if (!orgFreedesktopDBusPeerMapped_) {
                orgFreedesktopDBusPeerMapped_ = (_interface == "org.freedesktop.DBus.Peer");
                if (orgFreedesktopDBusPeerMapped_) {
                    COMMONAPI_DEBUG("org.freedesktop.DBus.Peer mapped");
                }
            }
        } else if(bw != backwards_.end() && bw->second != address) {
            COMMONAPI_ERROR("Trying to overwrite existing DBus address "
                    "which is already mapped to a CommonAPI address: ",
                    dbusAddress, " <--> ", _address);
        } else if(fw != forwards_.end() && fw->second != dbusAddress) {
            COMMONAPI_ERROR("Trying to overwrite existing CommonAPI address "
                    "which is already mapped to a DBus address: ",
                    _address, " <--> ", dbusAddress);
        }
    }
}

bool
WampAddressTranslator::readConfiguration() {
#define MAX_PATH_LEN 255
    std::string config;
    char currentDirectory[MAX_PATH_LEN];
#ifdef WIN32
    if (GetCurrentDirectory(MAX_PATH_LEN, currentDirectory)) {
#else
    if (getcwd(currentDirectory, MAX_PATH_LEN)) {
#endif
        config = currentDirectory;
        config += "/";
        config += COMMONAPI_DBUS_DEFAULT_CONFIG_FILE;

        struct stat s;
        if (stat(config.c_str(), &s) != 0) {
            config = defaultConfig_;
        }
    }

    IniFileReader reader;
    if (!reader.load(config))
        return false;

    for (auto itsMapping : reader.getSections()) {
        if(itsMapping.first == "segments") {
            std::map<std::string, std::string> mappings = itsMapping.second->getMappings();
            ConnectionId_t connectionId;
            std::string busType;
            for(auto const &it : mappings) {
                connectionId = it.first;
                busType = it.second;
                if(busType == "SESSION") {
                    dbusTypes_.insert({ connectionId, DBusType_t::SESSION });
                } else if (busType == "SYSTEM") {
                    dbusTypes_.insert({ connectionId, DBusType_t::SYSTEM });
                } else {
                    COMMONAPI_FATAL("Invalid bus type specified in .ini file, "
                            "choose one of {SYSTEM, SESSION}");
                    continue;
                }
                COMMONAPI_INFO("D-Bus bus type for connection: " + connectionId +
                        " is set to: " + busType + " via ini file");
            }
            continue;
        }

        CommonAPI::Address itsAddress(itsMapping.first);

        std::string service = itsMapping.second->getValue("service");
        std::string path = itsMapping.second->getValue("path");
        std::string interfaceName = itsMapping.second->getValue("interface");

        insert(itsMapping.first, service, path, interfaceName);
    }

    return true;
}

bool
WampAddressTranslator::isValid(
        const std::string &_name, const char _separator,
        bool _ignoreFirst, bool _isAllowedToStartWithDigit, bool _isBusName) const {
    (void)_isAllowedToStartWithDigit;
    // DBus addresses must contain at least one separator
    std::size_t separatorPos = _name.find(_separator);
    if (separatorPos == std::string::npos) {
        COMMONAPI_ERROR(
            "Invalid name \'", _name,
            "\'. Contains no \'", _separator, "\'");
        return false;
    }

    bool isInitial(true);
    std::size_t start(0);

    if (_ignoreFirst) {
        start = 1;
        if (separatorPos == 0) {
            // accept "root-only" i.e. '/' object path
            if (1 == _name.length()) {
                return true;
            }
            separatorPos = _name.find(_separator, separatorPos+1);
        }
    }

    while (start != std::string::npos) {
        // DBus names parts must not be empty
        std::string part;

        if (isInitial) {
            isInitial = false;
        } else {
            start++;
        }

        if (separatorPos == std::string::npos) {
            part = _name.substr(start);
        } else {
            part = _name.substr(start, separatorPos-start);
        }

        if ("" == part) {
            COMMONAPI_ERROR(
                "Invalid interface name \'", _name,
                "\'. Must not contain empty parts.");
            return false;
        }

        // DBus name parts consist of the ASCII characters [0-9][A-Z][a-z]_,
        for (auto c : part) {
            // bus names may additionally contain [-]
            if (_isBusName && c == '-')
                continue;

            if (c < '0' ||
                (c > '9' && c < 'A') ||
                (c > 'Z' && c < '_') ||
                (c > '_' && c < 'a') ||
                c > 'z') {
                COMMONAPI_ERROR(
                    "Invalid interface name \'", _name,
                    "\'. Contains illegal character \'", c,
                    "\'. Only \'[0-9][A-Z][a-z]_\' are allowed.");
                return false;
            }
        }

        start = separatorPos;
        separatorPos = _name.find(_separator, separatorPos+1);
    }

    // DBus names must not exceed the maximum length
    if (_name.length() > DBUS_MAXIMUM_NAME_LENGTH) {
        COMMONAPI_ERROR(
            "Invalid interface name \'", _name,
            "\'. Size exceeds maximum size.");
        return false;
    }

    return true;
}
*/

WampType_t
WampAddressTranslator::getWampBusType(const ConnectionId_t &_connectionId) const {
    auto itsWampTypesIterator = wampTypes_.find(_connectionId);
    if(itsWampTypesIterator != wampTypes_.end()) {
        return itsWampTypesIterator->second;
    } else {
        return WampType_t::SESSION;
    }
}

} // namespace Wamp
} // namespace CommonAPI
