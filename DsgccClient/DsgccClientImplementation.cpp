#include "DsgccClientImplementation.h"
#include <iomanip>
#include <sstream>

#include <refsw/dsgcc_client_api.h>
#define CLIENTPORTKEEPALIVE 50
#define MAXLINE 4096

extern "C" {
#include "refsw/BcmSharedMemory.h"
#include "refsw/dsgccClientCallback_rpcServer.h"
#include "refsw/dsgccClientRegistration_rpcClient.h"

void dsgcc_ClientNotification(struct dsgccClientNotification* clientNotification);
extern char* TunnelStatusTypeName(unsigned int value);
}

namespace WPEFramework {
namespace Plugin {

    DsgccClientImplementation::DsgccClientImplementation()
        : _observers()
        , _worker(_config)
        , _dsgCallback()
    {
    }

    uint32_t DsgccClientImplementation::Configure(PluginHost::IShell* service)
    {
        uint32_t result = 0;
        ASSERT(service != nullptr);

        _config.FromString(service->ConfigLine());

        _worker.Run();
        //_dsgCallback.Run();

        return (result);
    }

    void DsgccClientImplementation::DsgccClientSet(const string& str)
    {
        this->str = str;
    }

    string DsgccClientImplementation::GetChannels() const
    {
        TRACE_L1("Entering %s", __FUNCTION__);
        return (_worker.getChannels());
    }

    uint32_t DsgccClientImplementation::Activity::Worker() {

        TRACE_L1("Entering %s state=%d", __PRETTY_FUNCTION__, Core::Thread::State());
        Setup(_config.DsgPort, _config.DsgType, _config.DsgId);

        Stop();                         // XXX:

        TRACE_L1("Exiting %s state=%d", __PRETTY_FUNCTION__, Core::Thread::State());
        return (Core::infinite);
    }

    void DsgccClientImplementation::Activity::Setup(unsigned int port, unsigned int dsgType, unsigned int dsgId)
    {
        static struct dsgClientRegInfo regInfoData;

        struct dsgClientRegInfo *regInfo = &regInfoData;
        int retVal;
        char msg[MAXLINE];
        int len;
        int sharedMemoryId;

        CLIENT* handle = NULL;

        bzero((char *) regInfo, sizeof(struct dsgClientRegInfo));
        regInfo->clientPort = port;
        regInfo->idType = dsgType;
        regInfo->clientId = dsgId;

        sharedMemoryId = BcmSharedMemoryCreate(regInfo->clientPort, 1);
        handle = clnt_create("127.0.0.1", DSGREGISTRAR, DSGREGISTRARVERS, "udp");
        if (handle == 0) {
            TRACE_L1("Could not contact DSG Registrar");
            return;
        }

        regInfo->handle = (int)handle;

        TRACE_L1("registerdsgclient port=%d type=%d id=%d", regInfo->clientPort, regInfo->idType, regInfo->clientId);
        regInfo->idType |= 0x00010000;    // strip IP/UDP headers
        retVal = registerdsgclient(regInfo);

        if (retVal == DSGCC_SUCCESS) {
            TRACE_L1("Tunnel request is registered successfully and tunnel is pending.");
        }

        retVal = dsgcc_KeepAliveClient(&regInfoData);
        if (retVal & DSGCC_CLIENT_REGISTERED) {
            // Get tunnel status = MSB 16 bits
            retVal >>= 16;
            TRACE_L1("tunnel status %s", TunnelStatusTypeName(retVal));
        }

        DsgParser _parser(_config.VctId);
        while ( _isRunning ) {
            len = BcmSharedMemoryRead(sharedMemoryId, msg, 0);
            if (len > 0) {
                if (len >= _config.DsgHeaderSize) {
                    char *pBuf = &msg[_config.DsgHeaderSize];
                    len -= _config.DsgHeaderSize;

                    _parser.parse((unsigned char *) pBuf, len);
                    if (_parser.isDone())
                        _isRunning = false;
                }
            }

            if (len <= 0) {
                if (CLIENTPORTKEEPALIVE) {
                    if (errno == EWOULDBLOCK) { // XXX:
                    retVal = dsgcc_KeepAliveClient(&regInfoData);
                        if(retVal & DSGCC_CLIENT_REGISTERED) {
                            retVal >>= 16;
                            TRACE_L1("tunnel status %s", TunnelStatusTypeName(retVal));
                        }
                    }
                }
            }
        } // while

        TRACE_L1("Unregistering DsgCC client.");
        retVal = dsgcc_UnregisterClient(regInfo);
        BcmSharedMemoryDelete(sharedMemoryId);

        _channels = _parser.getChannels();
        TRACE_L1("_parser.getChannels().length=%d", _channels.length());
    }

    uint32_t DsgccClientImplementation::ClientCallbackService::Worker() {
      
        TRACE_L1("Starting ClientCallbackService::%s", __FUNCTION__);
        dsgClientCallbackSvcRun();
        return (Core::infinite);
    }

    void dsgcc_ClientNotification(struct dsgccClientNotification* clientNotification)
    {
        TRACE_L1("%s: NOTIFICATION from DSG-CC -> %s", __FUNCTION__, TunnelStatusTypeName(clientNotification->eventType));
    }

} // namespace Plugin
} // namespace WPEFramework
