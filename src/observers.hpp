// No-op implementations of most webrtc::*Observer methods. For the ones we do
// care about in the example, we supply a callback in the constructor.

#ifndef WEBRTC_EXAMPLE_SERVER_OBSERVERS_H
#define WEBRTC_EXAMPLE_SERVER_OBSERVERS_H
/*
#include <websocketpp/client.hpp>
#include <websocketpp/common/memory.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>
#include <websocketpp/server.hpp>
*/
#include <webrtc/api/peerconnectioninterface.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include "dispatch_queue.hpp"

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
#include <webrtc/base/macros.h>
#include <webrtc/media/engine/webrtcmediaengine.h>
#include <webrtc/rtc_base/physicalsocketserver.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>
#include <webrtc/rtc_base/ssladapter.h>
#include <webrtc/rtc_base/thread.h>
//#include "webrtc/rtc_base/third_party/sigslot/sigslot.h"
//#include "webrtc/rtc_base/strings/json.h"
#include "webrtc/p2p/base/basicpacketsocketfactory.h"
#include "webrtc/p2p/client/basicportallocator.h" // https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp
#include "webrtc/pc/peerconnection.h"
#include "webrtc/pc/peerconnectionfactory.h"
//#include "webrtc/rtc_base/gunit.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <memory>
#include <thread>

// WebSocket++ types are gnarly.
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

/*namespace po = boost::program_options;
namespace pt = boost::property_tree;
namespace ws = boost::beast::websocket;*/

/*struct testee_config : public websocketpp::config::asio {
    // pull default settings from our core config
    typedef websocketpp::config::asio core;

    typedef core::concurrency_type concurrency_type;
    typedef core::request_type request_type;
    typedef core::response_type response_type;
    typedef core::message_type message_type;
    typedef core::con_msg_manager_type con_msg_manager_type;
    typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;

    typedef core::alog_type alog_type;
    typedef core::elog_type elog_type;
    typedef core::rng_type rng_type;
    typedef core::endpoint_base endpoint_base;

    static bool const enable_multithreading = true;

    struct transport_config : public core::transport_config {
        typedef core::concurrency_type concurrency_type;
        typedef core::elog_type elog_type;
        typedef core::alog_type alog_type;
        typedef core::request_type request_type;
        typedef core::response_type response_type;

        static bool const enable_multithreading = true;
    };

    typedef websocketpp::transport::asio::endpoint<transport_config>
        transport_type;

    static const websocketpp::log::level elog_level =
        websocketpp::log::elevel::none;
    static const websocketpp::log::level alog_level =
        websocketpp::log::alevel::none;

    /// permessage_compress extension
    struct permessage_deflate_config {};

    typedef websocketpp::extensions::permessage_deflate::enabled
        <permessage_deflate_config> permessage_deflate_type;
};

//
https://github.com/zaphoyd/websocketpp/blob/378437aecdcb1dfe62096ffd5d944bf1f640ccc3/examples/testee_server/testee_server.cpp
typedef websocketpp::server<testee_config> WsServer;*/

typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;
typedef WebSocketServer::message_ptr message_ptr;

// TODO: global?
// Constants for Session Description Protocol (SDP)
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// see
// https://github.com/webrtc-uwp/webrtc/blob/master/examples/peerconnection/client/linux/main.cc#L93
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

class WRTCServer;
class NetworkManager;

// PeerConnection events.
// see
// https://github.com/sourcey/libsourcey/blob/ce311ff22ca02c8a83df7162a70f6aa4f760a761/doc/api-webrtc.md
class PCO : public webrtc::PeerConnectionObserver {
public:
  // Constructor taking a few callbacks.
  PCO(WRTCServer& observer) : m_observer(&observer) {}

  // Triggered when a remote peer opens a data channel.
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;

  // Override ICE candidate.
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;

  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override;

  // Triggered when media is received on a new stream from remote peer.
  void OnAddStream(
      rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream*/) override;

  // Triggered when a remote peer close a stream.
  void OnRemoveStream(
      rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream*/) override;

  // Triggered when renegotiation is needed. For example, an ICE restart
  // has begun.
  void OnRenegotiationNeeded() override;

  // Called any time the IceConnectionState changes.
  //
  // Note that our ICE states lag behind the standard slightly. The most
  // notable differences include the fact that "failed" occurs after 15
  // seconds, not 30, and this actually represents a combination ICE + DTLS
  // state, so it may be "failed" if DTLS fails while ICE succeeds.
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

  // Called any time the IceGatheringState changes.
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

  // TODO OnInterestingUsage

private:
  WRTCServer* m_observer;

  // see
  // https://cs.chromium.org/chromium/src/remoting/protocol/webrtc_transport.cc?q=SetSessionDescriptionObserver&dr=CSs&l=148
  DISALLOW_COPY_AND_ASSIGN(PCO);
};

// DataChannel events.
class DCO : public webrtc::DataChannelObserver {
public:
  // Constructor taking a callback.
  DCO(WRTCServer& observer) : m_observer(&observer) {}

  // Buffered amount change.
  void OnBufferedAmountChange(uint64_t /* previous_amount */) override;

  void OnStateChange() override;

  // Message received.
  void OnMessage(const webrtc::DataBuffer& buffer) override;

private:
  WRTCServer* m_observer;

  // see
  // https://cs.chromium.org/chromium/src/remoting/protocol/webrtc_transport.cc?q=SetSessionDescriptionObserver&dr=CSs&l=148
  DISALLOW_COPY_AND_ASSIGN(DCO);
};

// Create SessionDescription events.
class CSDO : public webrtc::CreateSessionDescriptionObserver {
public:
  // Constructor taking a callback.
  CSDO(WRTCServer& observer) : m_observer(&observer) {}

  /*void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;

  void OnFailure(const std::string& error) override;*/

  // Successfully created a session description.
  /*void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
    OnAnswerCreated(desc);
  }*/

  // Failure to create a session description.
  // void OnFailure(const std::string& /* error */) {}

  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;

  void OnFailure(const std::string& error) override;

  // SEE
  // https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h#L102
  // Unimplemented virtual function.
  void AddRef() const override { return; }

  // Unimplemented virtual function.
  rtc::RefCountReleaseStatus Release() const override {
    return rtc::RefCountReleaseStatus::kDroppedLastRef;
  }

private:
  WRTCServer* m_observer;

  // see
  // https://cs.chromium.org/chromium/src/remoting/protocol/webrtc_transport.cc?q=SetSessionDescriptionObserver&dr=CSs&l=148
  DISALLOW_COPY_AND_ASSIGN(CSDO);
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

  // SEE
  // https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h#L102
  // Unimplemented virtual function.
  void AddRef() const override { return; }

  // Unimplemented virtual function.
  rtc::RefCountReleaseStatus Release() const override {
    return rtc::RefCountReleaseStatus::kDroppedLastRef;
  }

  // see
  // https://cs.chromium.org/chromium/src/remoting/protocol/webrtc_transport.cc?q=SetSessionDescriptionObserver&dr=CSs&l=148
  DISALLOW_COPY_AND_ASSIGN(SSDO);
};

class WSServer {
public:
  WSServer(WRTCServer* webRTCServer, NetworkManager* networkManager)
      : m_WRTC(webRTCServer), m_networkManager(networkManager),
        websocket_connection_handler(websocketpp::connection_hdl()),
        ws_server(websocketpp::server<websocketpp::config::asio>()) {}
  void Quit();
  void InitAndRun();
  static void OnWebSocketMessage(WRTCServer* m_WRTC, WSServer* m_WS,
                                 WebSocketServer* /* s */,
                                 websocketpp::connection_hdl hdl,
                                 message_ptr msg);
  void handleWebsocketsPing(websocketpp::connection_hdl hdl, message_ptr msg);
  void send(const std::string& payload);

public:
  // thread for WebSocket listening loop.
  // std::thread websockets_thread;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> websockets_thread;
  // The WebSocket connection handler that uniquely identifies one of the
  // connections that the WebSocket has open. If you want to have multiple
  // connections, you will need to store more than one of these.
  websocketpp::connection_hdl websocket_connection_handler;
  websocketpp::server<websocketpp::config::asio> ws_server;
  WRTCServer* m_WRTC;
  NetworkManager* m_networkManager;
  // TODO dispatch_queue WSQueue{"WebSockets Server Dispatch Queue"}; // uses
  // parent thread (same thread)
};

class WRTCServer {
public:
  WRTCServer(WSServer* webSocketServer, NetworkManager* networkManager)
      : webSocketServer(webSocketServer), m_networkManager(networkManager),
        dataChannelstate(webrtc::DataChannelInterface::kClosed),
        data_channel_count(0),
        webrtcConfiguration(
            webrtc::PeerConnectionInterface::RTCConfiguration()),
        webrtc_gamedata_options(
            webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()) {
    WRTCQueue =
        new dispatch_queue(std::string{"WebRTC Server Dispatch Queue"}, 0);
    data_channel_observer = new DCO(*this);
    create_session_description_observer = new CSDO(*this);
    peer_connection_observer = new PCO(*this);
    local_description_observer = new SSDO();
    remote_description_observer = new SSDO();
    // thread_checker_.DetachFromThread();
  }
  ~WRTCServer() { // TODO: virtual
    // auto call Quit()?
    delete WRTCQueue;
  }
  bool sendDataViaDataChannel(
      const std::string& data); // RTC_RUN_ON(thread_checker_);
  bool sendDataViaDataChannel(
      const webrtc::DataBuffer& buffer); // RTC_RUN_ON(thread_checker_);
  webrtc::DataChannelInterface::DataState
  updateDataChannelState(); // RTC_RUN_ON(thread_checker_);
  bool isDataChannelOpen(); // RTC_RUN_ON(thread_checker_);
  void Quit();              // RTC_RUN_ON(thread_checker_);
  void resetWebRtcConfig(
      const std::vector<webrtc::PeerConnectionInterface::IceServer>&
          iceServers);
  void InitAndRun(); // RTC_RUN_ON(thread_checker_);
  void SetRemoteDescriptionAndCreateAnswer(
      const rapidjson::Document&
          message_object); // RTC_RUN_ON(thread_checker_);
  void
  createAndAddIceCandidate(const rapidjson::Document&
                               message_object); // RTC_RUN_ON(thread_checker_);
  void setLocalDescription(
      webrtc::SessionDescriptionInterface* sdi); // RTC_RUN_ON(thread_checker_);
public:
  NetworkManager* m_networkManager;
  dispatch_queue* WRTCQueue; // uses parent thread (same thread)
  // rtc::ThreadChecker thread_checker_;
  // The data channel used to communicate.
  rtc::scoped_refptr<webrtc::DataChannelInterface>
      data_channel; // RTC_GUARDED_BY(thread_checker_);
  // The peer connection through which we engage in the Session Description
  // Protocol (SDP) handshake.
  rtc::CriticalSection pc_mutex_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface>
      peer_connection; // RTC_GUARDED_BY(pc_mutex_); // TODO: multiple clients?
  WSServer* webSocketServer;
  // TODO: global config var
  webrtc::DataChannelInit
      data_channel_config; // RTC_GUARDED_BY(thread_checker_);
  // thread for WebRTC listening loop.
  std::thread webrtc_thread;
  webrtc::PeerConnectionInterface::RTCConfiguration
      webrtcConfiguration; // RTC_GUARDED_BY(thread_checker_);
  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions
      webrtc_gamedata_options; // RTC_GUARDED_BY(thread_checker_);
  // TODO: free memory
  // rtc::Thread* signaling_thread;
  /*
   * The signaling thread handles the bulk of WebRTC computation;
   * it creates all of the basic components and fires events we can consume by
   * calling the observer methods
   */
  std::unique_ptr<rtc::Thread>
      signaling_thread; // RTC_GUARDED_BY(thread_checker_);
  std::unique_ptr<rtc::Thread>
      network_thread; // RTC_GUARDED_BY(thread_checker_);
  /*
   * worker thread, on the other hand, is delegated resource-intensive tasks
   * such as media streaming to ensure that the signaling thread doesn’t get
   * blocked
   */
  std::unique_ptr<rtc::Thread>
      worker_thread; // RTC_GUARDED_BY(thread_checker_);
  // rtc::Thread* network_thread_;
  // The peer conncetion factory that sets up signaling and worker threads. It
  // is also used to create the PeerConnection.
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory; // RTC_GUARDED_BY(thread_checker_);
  // The socket that the signaling thread and worker thread communicate on.
  // CustomSocketServer socket_server;
  // rtc::PhysicalSocketServer socket_server;
  // last updated DataChannel state
  webrtc::DataChannelInterface::DataState
      dataChannelstate; // RTC_GUARDED_BY(thread_checker_);
                        // private:
  // The observer that responds to session description set events. We don't
  // really use this one here. webrtc::SetSessionDescriptionObserver for
  // acknowledging and storing an offer or answer.
  SSDO* local_description_observer;  // RTC_GUARDED_BY(thread_checker_);
  SSDO* remote_description_observer; // RTC_GUARDED_BY(thread_checker_);
  // The observer that responds to data channel events.
  // webrtc::DataChannelObserver for data channel events like receiving SCTP
  // messages.
  DCO*
      data_channel_observer; // RTC_GUARDED_BY(thread_checker_);//(webRtcObserver);
  // rtc::scoped_refptr<PCO> peer_connection_observer = new
  // rtc::RefCountedObject<PCO>(OnDataChannelCreated, OnIceCandidate);
  // The observer that responds to session description creation events.
  // webrtc::CreateSessionDescriptionObserver for creating an offer or answer.
  CSDO* create_session_description_observer; // RTC_GUARDED_BY(thread_checker_);
  // The observer that responds to peer connection events.
  // webrtc::PeerConnectionObserver for peer connection events such as receiving
  // ICE candidates.
  PCO* peer_connection_observer; // RTC_GUARDED_BY(thread_checker_);
  void OnDataChannelCreated(rtc::scoped_refptr<webrtc::DataChannelInterface>
                                channel); // RTC_RUN_ON(thread_checker_);
  void OnIceCandidate(const webrtc::IceCandidateInterface*
                          candidate); // RTC_RUN_ON(thread_checker_);
  void OnDataChannelMessage(
      const webrtc::DataBuffer& buffer); // RTC_RUN_ON(thread_checker_);
  void OnAnswerCreated(webrtc::SessionDescriptionInterface*
                           desc); // RTC_RUN_ON(thread_checker_);
  void onDataChannelOpen();       // RTC_RUN_ON(thread_checker_);
  void onDataChannelClose();      // RTC_RUN_ON(thread_checker_);
public:
  uint32_t data_channel_count; // TODO
};

class NetworkManager {
public:
  NetworkManager() {
    wrtcServer = new WRTCServer(wsServer, this);
    wsServer = new WSServer(wrtcServer, this);
    std::cout << std::this_thread::get_id() << ":"
              << "created NetworkManager" << std::endl;
    // dispatchQueue("Main Server Dispatch Queue", 1);
  }
  ~NetworkManager() {
    delete wsServer;
    delete wrtcServer;
  }
  // The WebSocket server being used to handshake with the clients.
  WSServer* wsServer;
  // WebRTC server
  WRTCServer* wrtcServer;
  dispatch_queue gameLogicQueue{std::string{"Main Server Dispatch Queue"},
                                1}; // creates separate thread
};

#endif // WEBRTC_EXAMPLE_SERVER_OBSERVERS_H
