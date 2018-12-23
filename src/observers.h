// No-op implementations of most webrtc::*Observer methods. For the ones we do care about in the
// example, we supply a callback in the constructor.

#ifndef WEBRTC_EXAMPLE_SERVER_OBSERVERS_H
#define WEBRTC_EXAMPLE_SERVER_OBSERVERS_H

#include <webrtc/api/peerconnectioninterface.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

//#define BOOST_NO_EXCEPTIONS
//#define BOOST_NO_RTTI

/*#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/foreach.hpp>
//#include <boost/lockfree/spsc_queue.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>*/
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/rtc_base/physicalsocketserver.h>
#include <webrtc/rtc_base/ssladapter.h>
#include <webrtc/rtc_base/thread.h>
#include <webrtc/media/engine/webrtcmediaengine.h>
#include "webrtc/rtc_base/third_party/sigslot/sigslot.h"
#include "webrtc/rtc_base/strings/json.h"
#include "webrtc/p2p/base/basicpacketsocketfactory.h"
#include "webrtc/p2p/client/basicportallocator.h" // https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp
#include "webrtc/pc/peerconnectionfactory.h"
#include "webrtc/pc/peerconnection.h"
//#include "webrtc/rtc_base/gunit.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <thread>

// WebSocket++ types are gnarly.
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

/*namespace po = boost::program_options;
namespace pt = boost::property_tree;
namespace ws = boost::beast::websocket;*/

typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;
typedef WebSocketServer::message_ptr message_ptr;

// TODO: global?
// Constants for Session Description Protocol (SDP)
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// see https://github.com/webrtc-uwp/webrtc/blob/master/examples/peerconnection/client/linux/main.cc#L93
/*class CustomSocketServer : public rtc::PhysicalSocketServer {
 public:
  explicit CustomSocketServer()
      : conductor_(NULL), client_(NULL) {}
  virtual ~CustomSocketServer() {}

  void SetMessageQueue(rtc::MessageQueue* queue) override {
    message_queue_ = queue;
  }

  void set_client(PeerConnectionClient* client) { client_ = client; }
  void set_conductor(Conductor* conductor) { conductor_ = conductor; }

  // Override so that we can also pump the GTK message loop.
  bool Wait(int cms, bool process_io) override {
    // Pump GTK events.
    // TODO(henrike): We really should move either the socket server or UI to a
    // different thread.  Alternatively we could look at merging the two loops
    // by implementing a dispatcher for the socket server and/or use
    // g_main_context_set_poll_func.
    return rtc::PhysicalSocketServer::Wait(0,
                                           process_io);
  }

 protected:
  rtc::MessageQueue* message_queue_;
  Conductor* conductor_;
  PeerConnectionClient* client_;
};*/

class WebRtcObserver;
class WRTCServer;
class NetworkManager;

// PeerConnection events.
// see https://github.com/sourcey/libsourcey/blob/ce311ff22ca02c8a83df7162a70f6aa4f760a761/doc/api-webrtc.md
class PCO : public webrtc::PeerConnectionObserver {
  public:
    // Constructor taking a few callbacks.
    PCO(WebRtcObserver& observer) :
        m_observer(&observer) {}

    // Triggered when a remote peer opens a data channel.
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;

    // Override ICE candidate.
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;

    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;

    // Triggered when media is received on a new stream from remote peer.
    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface>/* stream*/) override;

    // Triggered when a remote peer close a stream.
    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface>/* stream*/) override;

    // Triggered when renegotiation is needed. For example, an ICE restart
    // has begun.
    void OnRenegotiationNeeded() override;

    // Called any time the IceConnectionState changes.
    //
    // Note that our ICE states lag behind the standard slightly. The most
    // notable differences include the fact that "failed" occurs after 15
    // seconds, not 30, and this actually represents a combination ICE + DTLS
    // state, so it may be "failed" if DTLS fails while ICE succeeds.
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

    // Called any time the IceGatheringState changes.
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

    // TODO OnInterestingUsage

  private:
    WebRtcObserver* m_observer;
};

// DataChannel events.
class DCO : public webrtc::DataChannelObserver {
  public:
    // Constructor taking a callback.
    DCO(WebRtcObserver& observer) :
        m_observer(&observer) {}

    // Buffered amount change.
    void OnBufferedAmountChange(uint64_t /* previous_amount */) override;

    void OnStateChange() override;

    // Message received.
    void OnMessage(const webrtc::DataBuffer& buffer) override;

  private:
   WebRtcObserver* m_observer;
};

// Create SessionDescription events.
class CSDO : public webrtc::CreateSessionDescriptionObserver {
  public:
    // Constructor taking a callback.
    CSDO(WebRtcObserver& observer) :
        m_observer(&observer) {}

    /*void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;

    void OnFailure(const std::string& error) override;*/

    // Successfully created a session description.
    /*void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
      OnAnswerCreated(desc);
    }*/

    // Failure to create a session description.
    //void OnFailure(const std::string& /* error */) {}

    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;

    void OnFailure(const std::string& error) override;

    // SEE https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h#L102
    // Unimplemented virtual function.
    void AddRef() const override { return; }

    // Unimplemented virtual function.
    rtc::RefCountReleaseStatus Release() const override { return rtc::RefCountReleaseStatus::kDroppedLastRef; }

  private:
    WebRtcObserver* m_observer;
};

// Set SessionDescription events.
class SSDO : public webrtc::SetSessionDescriptionObserver {
  public:
    // Default constructor.
    SSDO() {}

    // Successfully set a session description.
    void OnSuccess() override {}

    // Failure to set a sesion description.
    void OnFailure(const std::string& /* error */) override {}

    // SEE https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h#L102
    // Unimplemented virtual function.
    void AddRef() const override { return; }

    // Unimplemented virtual function.
    rtc::RefCountReleaseStatus Release() const override { return rtc::RefCountReleaseStatus::kDroppedLastRef; }
};


class WSServer {
  public:
    WSServer(WRTCServer& webRTCServer)
      : m_WRTC(&webRTCServer) {}
    void Quit();
    void InitAndRun();
    static void OnWebSocketMessage(WRTCServer* m_WRTC, WSServer* m_WS, WebSocketServer* /* s */, websocketpp::connection_hdl hdl, message_ptr msg);
    void handleWebsocketsPing(websocketpp::connection_hdl hdl, message_ptr msg);
    void send(const std::string payload);
  public:
    // thread for WebSocket listening loop.
    std::thread websockets_thread;
    // The WebSocket connection handler that uniquely identifies one of the connections that the
    // WebSocket has open. If you want to have multiple connections, you will need to store more than
    // one of these.
    websocketpp::connection_hdl websocket_connection_handler;
    websocketpp::server<websocketpp::config::asio> ws_server;
    WRTCServer* m_WRTC;
};

class WRTCServer {
  public:
    WRTCServer(WSServer& webSocketServer, WebRtcObserver& webRtcObserver)
      : webSocketServer(&webSocketServer), dataChannelstate(),
        observer(&webRtcObserver) {}
    bool sendDataViaDataChannel(const std::string& data);
    bool sendDataViaDataChannel(const webrtc::DataBuffer& buffer);
    webrtc::DataChannelInterface::DataState updateDataChannelState();
    bool isDataChannelOpen();
    void Quit();
    void resetWebRtcConfig(const std::vector<webrtc::PeerConnectionInterface::IceServer>& iceServers);
    void InitAndRun();
  public:
    // The data channel used to communicate.
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;
    // The peer connection through which we engage in the Session Description Protocol (SDP) handshake.
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection; // TODO: multiple clients?
    WSServer* webSocketServer;
    // TODO: global config var
    webrtc::DataChannelInit data_channel_config;
    // thread for WebRTC listening loop.
    std::thread webrtc_thread;
    webrtc::PeerConnectionInterface::RTCConfiguration webrtcConfiguration;
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtc_gamedata_options;
    // TODO: free memory
    //rtc::Thread* signaling_thread;
    /*
    * The signaling thread handles the bulk of WebRTC computation;
    * it creates all of the basic components and fires events we can consume by calling the observer methods
    */
    std::unique_ptr<rtc::Thread> signaling_thread;
    std::unique_ptr<rtc::Thread> network_thread;
    /*
    * worker thread, on the other hand, is delegated resource-intensive tasks
    * such as media streaming to ensure that the signaling thread doesn’t get blocked
    */
    std::unique_ptr<rtc::Thread> worker_thread;
    //rtc::Thread* network_thread_;
    // The peer conncetion factory that sets up signaling and worker threads. It is also used to create
    // the PeerConnection.
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
    // The socket that the signaling thread and worker thread communicate on.
    //CustomSocketServer socket_server;
    //rtc::PhysicalSocketServer socket_server;
    // last updated DataChannel state
    webrtc::DataChannelInterface::DataState dataChannelstate;
  //private:
    WebRtcObserver* observer;
};

class WebRtcObserver {
  public:
    WebRtcObserver(WRTCServer& WRTCServer)
      : m_WRTC(&WRTCServer), data_channel_observer(DCO(*this)),
        create_session_description_observer(CSDO(*this)),
        peer_connection_observer(PCO(*this)), data_channel_count(0) {}
    void OnDataChannelCreated(rtc::scoped_refptr<webrtc::DataChannelInterface> channel);
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);
    void OnDataChannelMessage(const webrtc::DataBuffer& buffer);
    void OnAnswerCreated(webrtc::SessionDescriptionInterface* desc);
    void onDataChannelOpen();
    void onDataChannelClose();
  public:
    uint32_t data_channel_count; // TODO
  //private:
    WRTCServer* m_WRTC;
    // The observer that responds to session description set events. We don't really use this one here.
    // webrtc::SetSessionDescriptionObserver for acknowledging and storing an offer or answer.
    SSDO local_description_observer;
    SSDO remote_description_observer;
    // The observer that responds to data channel events.
    // webrtc::DataChannelObserver for data channel events like receiving SCTP messages.
    DCO data_channel_observer;//(webRtcObserver);
    //rtc::scoped_refptr<PCO> peer_connection_observer = new rtc::RefCountedObject<PCO>(OnDataChannelCreated, OnIceCandidate);
    // The observer that responds to session description creation events.
    // webrtc::CreateSessionDescriptionObserver for creating an offer or answer.
    CSDO create_session_description_observer;
    // The observer that responds to peer connection events.
    // webrtc::PeerConnectionObserver for peer connection events such as receiving ICE candidates.
    PCO peer_connection_observer;
};

class NetworkManager {
  public:
    NetworkManager()
      : wsServer(WSServer(wrtcServer)),
        wrtcServer(WRTCServer(wsServer, wrtcObserver)),
        wrtcObserver(WebRtcObserver(wrtcServer)) {
    }
    // The WebSocket server being used to handshake with the clients.
    WSServer wsServer;
    // WebRTC server
    WRTCServer wrtcServer;
    WebRtcObserver wrtcObserver;
};

#endif  // WEBRTC_EXAMPLE_SERVER_OBSERVERS_H