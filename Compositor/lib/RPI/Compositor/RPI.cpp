#include <interfaces/IComposition.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {

class CompositorImplementation :
        public Exchange::IComposition,
        public Exchange::IComposition::INotification {
private:
    CompositorImplementation(const CompositorImplementation&) = delete;
    CompositorImplementation& operator=(const CompositorImplementation&) = delete;

    class ExternalAccess : public RPC::Communicator {
    private:
        ExternalAccess() = delete;
        ExternalAccess(const ExternalAccess &) = delete;
        ExternalAccess & operator=(const ExternalAccess &) = delete;

    public:
        ExternalAccess(
                const Core::NodeId & source,
                IComposition::INotification* parentInterface,
                const string& proxyStub)
        : RPC::Communicator(source, Core::ProxyType< RPC::InvokeServerType<8, 1> >::Create(), proxyStub)
        , _parentInterface (parentInterface) {
        }

        ~ExternalAccess() {
            Close(Core::infinite);
        }

    private:
        void* Instance(const string& className, const uint32_t interfaceId, const uint32_t versionId) override {
            void* result = nullptr;
            // Currently we only support version 1 of the IRPCLink :-)
            if (((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) &&
                 (interfaceId == IComposition::INotification::ID)) {
                // Reference count our parent
                _parentInterface->AddRef();
                result = _parentInterface;
            }
            return (result);
        }
        
    private:
        IComposition::INotification* _parentInterface;
    };

public:
    CompositorImplementation()
    : _adminLock()
    , _service(nullptr)
    , _externalAccess()
    , _observers()
    , _clients() {
    }

    ~CompositorImplementation() {
        if (_service != nullptr) {
            _service->Release();
        }
    }

    BEGIN_INTERFACE_MAP(CompositorImplementation)
    INTERFACE_ENTRY(Exchange::IComposition)
    INTERFACE_ENTRY(Exchange::IComposition::INotification)
    END_INTERFACE_MAP

private:
    class Config : public Core::JSON::Container {
    private:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        Config()
        : Core::JSON::Container()
        , Connector(_T("/tmp/compositor")) {
            Add(_T("connector"), &Connector);
        }

        ~Config() {
        }

    public:
        Core::JSON::String Connector;
    };

public:
    uint32_t Configure(PluginHost::IShell* service) {
        _service = service;
        _service->AddRef();

        string configuration(service->ConfigLine());
        Config config;
        config.FromString(service->ConfigLine());

        _externalAccess.reset(
                new ExternalAccess(
                        Core::NodeId(config.Connector.Value().c_str()), this, service->ProxyStubPath()));
        uint32_t result = _externalAccess->Open(Core::infinite);
        if (result == Core::ERROR_NONE) {
            // Announce the port on which we are listening
            Core::SystemInfo::SetEnvironment(
                    _T("COMPOSITOR"), config.Connector.Value(), true);
            PlatformReady();
        }
        else {
            _externalAccess.reset();
            TRACE(Trace::Error,
                    (_T("Could not open RPI Compositor RPCLink server. Error: %s"),
                            Core::NumberType<uint32_t>(result).Text()));
        }
        return  result;
    }

    void Register(Exchange::IComposition::INotification* notification) {
        _adminLock.Lock();
        ASSERT(std::find(_observers.begin(),
                _observers.end(), notification) == _observers.end());
        notification->AddRef();
        _observers.push_back(notification);
        std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());
        while (index != _clients.end()) {
            notification->Attached(*index);
            index++;
        }
        _adminLock.Unlock();
    }

    void Unregister(Exchange::IComposition::INotification* notification) {
        _adminLock.Lock();
        std::list<Exchange::IComposition::INotification*>::iterator index(
                std::find(_observers.begin(), _observers.end(), notification));
        ASSERT(index != _observers.end());
        if (index != _observers.end()) {
            _observers.erase(index);
            notification->Release();
        }
        _adminLock.Unlock();
    }

    Exchange::IComposition::IClient* Client(const uint8_t id) {
        Exchange::IComposition::IClient* result = nullptr;
        uint8_t count = id;
        _adminLock.Lock();
        std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());
        while ((count != 0) && (index != _clients.end())) {
            count--;
            index++;
        }
        if (index != _clients.end()) {
            result = (*index);
            result->AddRef();
        }
        _adminLock.Unlock();
        return (result);
    }

    Exchange::IComposition::IClient* Client(const string& name) {
        Exchange::IComposition::IClient* result = nullptr;
        _adminLock.Lock();
        std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());
        while ((index != _clients.end()) && ((*index)->Name() != name)) {
            index++;
        }
        if (index != _clients.end()) {
            result = (*index);
            result->AddRef();
        }
        _adminLock.Unlock();
        return (result);
    }

    void Resolution(const Exchange::IComposition::ScreenResolution format) {
        fprintf(stderr, "CompositorImplementation::Resolution %d\n", format);
    }

    Exchange::IComposition::ScreenResolution Resolution() const {
        return Exchange::IComposition::ScreenResolution::ScreenResolution_720p;
    }

private:
    void Add(Exchange::IComposition::IClient* client) {

        ASSERT (client != nullptr);
        if (client != nullptr) {

            const std::string name(client->Name());
            if (name.empty() == true) {
                TRACE(Trace::Information,
                        (_T("Registration of a nameless client.")));
            }
            else {
                std::list<Exchange::IComposition
                ::IClient*>::iterator index(_clients.begin());
                while ((index != _clients.end()) && (name != (*index)->Name())) {
                    index++;
                }
                if (index != _clients.end()) {
                    TRACE(Trace::Information,
                            (_T("Client already registered %s."), name.c_str()));
                }
                else {
                    client->AddRef();
                    _clients.push_back(client);
                    TRACE(Trace::Information, (_T("Added client %s."), name.c_str()));

                    if (_observers.size() > 0) {
                        std::list<Exchange::IComposition
                        ::INotification*>::iterator index(_observers.begin());
                        while (index != _observers.end()) {
                            (*index)->Attached(client);
                            index++;
                        }
                    }
                }
            }
        }
    }

    void Remove(const char clientName[]) {
        const std::string name(clientName);
        if (name.empty() == true) {
            TRACE(Trace::Information, (_T("Unregistering a nameless client.")));
        }
        else {
            std::list<Exchange::IComposition
            ::IClient*>::iterator index(_clients.begin());
            while ((index != _clients.end()) && (name != (*index)->Name())) {
                index++;
            }
            if (index == _clients.end()) {
                TRACE(Trace::Information,
                        (_T("Client already unregistered %s."), name.c_str()));
            }
            else {
                Exchange::IComposition::IClient* entry(*index);
                _clients.erase(index);
                TRACE(Trace::Information, (_T("Removed client %s."), name.c_str()));

                if (_observers.size() > 0) {
                    std::list<Exchange::IComposition
                    ::INotification*>::iterator index(_observers.begin());
                    while (index != _observers.end()) {
                        (*index)->Detached(entry);
                        index++;
                    }
                }
                entry->Release();
            }
        }
    }

    void PlatformReady() {
        PluginHost::ISubSystem* subSystems(_service->SubSystems());
        ASSERT(subSystems != nullptr);
        if (subSystems != nullptr) {
            subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
            subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
            subSystems->Release();
        }
    }

    void Attached(IClient* client) override {
        Add(client);
    }

    void Detached(IClient* client) override {
        Remove(client->Name().c_str());
    }

    Core::CriticalSection _adminLock;
    PluginHost::IShell* _service;
    std::unique_ptr<ExternalAccess> _externalAccess;
    std::list<Exchange::IComposition::INotification*> _observers;
    std::list<Exchange::IComposition::IClient*> _clients;
};

SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
