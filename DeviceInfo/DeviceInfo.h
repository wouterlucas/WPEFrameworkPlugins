#ifndef DEVICEINFO_DEVICEINFO_H
#define DEVICEINFO_DEVICEINFO_H

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class DeviceInfo : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        class Data : public Core::JSON::Container {
        public:
            class SocketPortInfo : public Core::JSON::Container {
            private:
                SocketPortInfo(const SocketPortInfo&) = delete;
                SocketPortInfo& operator=(const SocketPortInfo&) = delete;

            public:
                SocketPortInfo()
                    : Core::JSON::Container()
                    , Total(0)
                    , Open(0)
                    , Link(0)
                    , Exception(0)
                    , Shutdown(0)
                    , Runs(0)
                {
                    Add(_T("total"), &Total);
                    Add(_T("link"), &Link);
                    Add(_T("open"), &Open);
                    Add(_T("exception"), &Exception);
                    Add(_T("shutdown"), &Shutdown);
                    Add(_T("runs"), &Runs);
                }
                ~SocketPortInfo()
                {
                }

            public:
                Core::JSON::DecUInt32 Total;
                Core::JSON::DecUInt32 Open;
                Core::JSON::DecUInt32 Link;
                Core::JSON::DecUInt32 Exception;
                Core::JSON::DecUInt32 Shutdown;
                Core::JSON::DecUInt32 Runs;
            };
            class SysInfo : public Core::JSON::Container {
            private:
                SysInfo(const SysInfo&) = delete;
                SysInfo& operator=(const SysInfo&) = delete;

            public:
                SysInfo()
                    : Time("No information")
                    , Version("0.0")
                    , Uptime(0)
                    , TotalRam(0)
                    , FreeRam(0)
                    , Hostname("No information")
                    , CpuLoad("No information")
                    , TotalGpuRam(0)
                    , FreeGpuRam(0)
                    , SerialNumber("No information")
                    , DeviceId("No information")
                {
                    Add(_T("version"), &Version);
                    Add(_T("uptime"), &Uptime);
                    Add(_T("totalram"), &TotalRam);
                    Add(_T("freeram"), &FreeRam);
                    Add(_T("devicename"), &Hostname);
                    Add(_T("cpuload"), &CpuLoad);
                    Add(_T("totalgpuram"), &TotalGpuRam);
                    Add(_T("freegpuram"), &FreeGpuRam);
                    Add(_T("serialnumber"), &SerialNumber);
                    Add(_T("deviceid"), &DeviceId);
                    Add(_T("time"), &Time);
                }

                ~SysInfo()
                {
                }

                void Update(void);

            public:
                Core::JSON::String Time;
                Core::JSON::String Version;
                Core::JSON::DecUInt32 Uptime;
                Core::JSON::DecUInt64 TotalRam;
                Core::JSON::DecUInt64 FreeRam;
                Core::JSON::String Hostname;
                Core::JSON::String CpuLoad;
                Core::JSON::DecUInt64 TotalGpuRam;
                Core::JSON::DecUInt64 FreeGpuRam;
                Core::JSON::String SerialNumber;
                Core::JSON::String DeviceId;
            };
            class AddressInfo : public Core::JSON::Container {
            public:
                AddressInfo()
                    : Core::JSON::Container()
                    , Name()
                    , MACAddress()
                    , IPAddress()
                {
                    Add(_T("name"), &Name);
                    Add(_T("mac"), &MACAddress);
                    Add(_T("ip"), &IPAddress);
                }
                AddressInfo(const string& name, const string& mac)
                    : Core::JSON::Container()
                    , Name(name)
                    , MACAddress(mac)
                    , IPAddress()
                {
                    Add(_T("name"), &Name);
                    Add(_T("mac"), &MACAddress);
                    Add(_T("ip"), &IPAddress);
                }
                AddressInfo(const AddressInfo& copy)
                    : Core::JSON::Container()
                    , Name(copy.Name)
                    , MACAddress(copy.MACAddress)
                    , IPAddress()
                {

                    Add(_T("name"), &Name);
                    Add(_T("mac"), &MACAddress);
                    Add(_T("ip"), &IPAddress);

                    auto index(copy.IPAddress.Elements());

                    while (index.Next() == true) {
                        IPAddress.Add(index.Current());
                    }
                }
                AddressInfo& operator=(const AddressInfo& RHS)
                {
                    auto index(RHS.IPAddress.Elements());

                    Name = RHS.Name;
                    MACAddress = RHS.MACAddress;

                    while (index.Next() == true) {
                        IPAddress.Add(index.Current());
                    }

                    return *this;
                }

                virtual ~AddressInfo()
                {
                }

            public:
                Core::JSON::String Name;
                Core::JSON::String MACAddress;
                Core::JSON::ArrayType<Core::JSON::String> IPAddress;
            };

        private:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

        public:
            Data()
                : Core::JSON::Container()
                , Addresses()
                , SystemInfo()
            {
                Add(_T("addresses"), &Addresses);
                Add(_T("systeminfo"), &SystemInfo);
                Add(_T("sockets"), &Sockets);
            }

            virtual ~Data()
            {
            }

        public:
            Core::JSON::ArrayType<AddressInfo> Addresses;
            SysInfo SystemInfo;
            SocketPortInfo Sockets;
        };

    private:
        DeviceInfo(const DeviceInfo&) = delete;
        DeviceInfo& operator=(const DeviceInfo&) = delete;

        uint32_t addresses(const Core::JSON::String& parameters, Core::JSON::ArrayType<Data::AddressInfo>& response)
        {
            AddressInfo(response);
            return (Core::ERROR_NONE);
        }
        uint32_t system(const Core::JSON::String& parameters, Data::SysInfo& response)
        {
            SysInfo(response);
            return (Core::ERROR_NONE);
        }
        uint32_t sockets(const Core::JSON::String& parameters, Data::SocketPortInfo& response)
        {
            SocketPortInfo(response);
            return (Core::ERROR_NONE);
        }

    public:
        DeviceInfo()
            : _skipURL(0)
            , _service(nullptr)
            , _subSystem(nullptr)
            , _idProvider(nullptr)
            , _systemId()
            , _deviceId()
        {
            Register<Core::JSON::String, Core::JSON::ArrayType<Data::AddressInfo>>(_T("addresses"), &DeviceInfo::addresses, this);
            Register<Core::JSON::String, Data::SysInfo>(_T("system"), &DeviceInfo::system, this);
            Register<Core::JSON::String, Data::SocketPortInfo>(_T("sockets"), &DeviceInfo::sockets, this);
        }

        virtual ~DeviceInfo()
        {
        }

        BEGIN_INTERFACE_MAP(DeviceInfo)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        void SysInfo(Data::SysInfo& systemInfo);
        void AddressInfo(Core::JSON::ArrayType<Data::AddressInfo>& addressInfo);
        void SocketPortInfo(Data::SocketPortInfo& socketPortInfo);
        string GetDeviceId() const;

        class IdentityProvider : public PluginHost::ISubSystem::IIdentifier {
        public:
            IdentityProvider();
            virtual ~IdentityProvider(){
                if (_identifier != nullptr) {
                    delete (_identifier);
                }
            };

            IdentityProvider(const IdentityProvider&) = delete;
            IdentityProvider& operator=(const IdentityProvider&) = delete;

            BEGIN_INTERFACE_MAP(IdentityProvider)
                INTERFACE_ENTRY(PluginHost::ISubSystem::IIdentifier)
            END_INTERFACE_MAP

            virtual uint8_t Identifier(const uint8_t length, uint8_t buffer[]) const override{
                uint8_t result = 0;

                if (_identifier != nullptr) {
                    result = _identifier[0];
                    ::memcpy(buffer, &(_identifier[1]), (result > length ? length : result));
                }

                return (result);
            }         
        private:
            uint8_t* _identifier;
        };

    private:
        uint8_t _skipURL;
        PluginHost::IShell* _service;
        PluginHost::ISubSystem* _subSystem;
        IdentityProvider* _idProvider;
        string _systemId;
        string _deviceId;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // DEVICEINFO_DEVICEINFO_H
