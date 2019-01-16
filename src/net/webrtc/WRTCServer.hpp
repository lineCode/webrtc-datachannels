#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/base/macros.h>
#include <webrtc/media/engine/webrtcmediaengine.h>
#include <webrtc/p2p/base/basicpacketsocketfactory.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/pc/peerconnection.h>
#include <webrtc/pc/peerconnectionfactory.h>
#include <webrtc/rtc_base/physicalsocketserver.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>
#include <webrtc/rtc_base/ssladapter.h>
#include <webrtc/rtc_base/thread.h>

namespace utils {
namespace algo {
class DispatchQueue;
} // namespace algo
} // namespace utils

namespace utils {
namespace net {

class NetworkManager;
class DCO;
class PCO;
class SSDO;
class CSDO;

class WRTCServer {
public:
  WRTCServer(NetworkManager* nm);
  ~WRTCServer();
  bool sendDataViaDataChannel(const std::string& data);

  bool sendDataViaDataChannel(const webrtc::DataBuffer& buffer);

  webrtc::DataChannelInterface::DataState updateDataChannelState();

  bool isDataChannelOpen();

  void Quit();

  void resetWebRtcConfig(
      const std::vector<webrtc::PeerConnectionInterface::IceServer>&
          iceServers);
  void InitAndRun();

  void SetRemoteDescriptionAndCreateAnswer(
      const rapidjson::Document& message_object);

  void createAndAddIceCandidate(const rapidjson::Document& message_object);

  void setLocalDescription(webrtc::SessionDescriptionInterface* sdi);

  void OnDataChannelCreated(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel);

  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);

  void OnDataChannelMessage(const webrtc::DataBuffer& buffer);

  void OnAnswerCreated(webrtc::SessionDescriptionInterface* desc);

  void onDataChannelOpen();

  void onDataChannelClose();

  algo::DispatchQueue* getWRTCQueue() const { return WRTCQueue_; };

private:
  algo::DispatchQueue* WRTCQueue_; // uses parent thread (same thread)

  // The data channel used to communicate.
  rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannelI_;

  // The peer connection through which we engage in the Session Description
  // Protocol (SDP) handshake.
  rtc::CriticalSection pcMutex_;

  rtc::scoped_refptr<webrtc::PeerConnectionInterface>
      peerConnection_; // RTC_GUARDED_BY(pc_mutex_); // TODO: multiple clients?

  NetworkManager* nm_;

  // TODO: global config var
  webrtc::DataChannelInit dataChannelConf_;

  // thread for WebRTC listening loop.
  // TODO
  // std::thread webrtc_thread;

  webrtc::PeerConnectionInterface::RTCConfiguration webrtcConf_;

  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtcGamedataOpts_;
  // TODO: free memory
  // rtc::Thread* signaling_thread;

  /*
   * The signaling thread handles the bulk of WebRTC computation;
   * it creates all of the basic components and fires events we can consume by
   * calling the observer methods
   */
  std::unique_ptr<rtc::Thread> signalingThread_;

  std::unique_ptr<rtc::Thread> networkThread_;
  /*
   * worker thread, on the other hand, is delegated resource-intensive tasks
   * such as media streaming to ensure that the signaling thread doesn’t get
   * blocked
   */
  std::unique_ptr<rtc::Thread> workerThread_;

  // rtc::Thread* network_thread_;
  // The peer conncetion factory that sets up signaling and worker threads. It
  // is also used to create the PeerConnection.
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peerConnectionFactory_;

  // The socket that the signaling thread and worker thread communicate on.
  // CustomSocketServer socket_server;
  // rtc::PhysicalSocketServer socket_server;
  // last updated DataChannel state
  webrtc::DataChannelInterface::DataState dataChannelstate_;

  // The observer that responds to session description set events. We don't
  // really use this one here. webrtc::SetSessionDescriptionObserver for
  // acknowledging and storing an offer or answer.
  SSDO* localDescriptionObserver_;

  SSDO* remoteDescriptionObserver_;

  // The observer that responds to data channel events.
  // webrtc::DataChannelObserver for data channel events like receiving SCTP
  // messages.
  DCO* dataChannelObserver_; //(webRtcObserver);
                             // rtc::scoped_refptr<PCO> peer_connection_observer
                             // = new
                             // rtc::RefCountedObject<PCO>(OnDataChannelCreated,
                             // OnIceCandidate);

  // The observer that responds to session description creation events.
  // webrtc::CreateSessionDescriptionObserver for creating an offer or answer.
  CSDO* createSDO_;

  // The observer that responds to peer connection events.
  // webrtc::PeerConnectionObserver for peer connection events such as receiving
  // ICE candidates.
  PCO* peerConnectionObserver_;

  uint32_t dataChannelCount_; // TODO
  // TODO: close data_channel on timer?
  // uint32_t getMaxSessionId() const { return maxSessionId_; }
  // TODO: limit max num of open sessions
  // uint32_t maxSessionId_ = 0;
  // TODO: limit max num of open connections per IP
  // uint32_t maxConnectionsPerIP_ = 0;
};

} // namespace net
} // namespace utils
