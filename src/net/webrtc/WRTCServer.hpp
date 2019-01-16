#pragma once

//#define BOOST_NO_EXCEPTIONS
//#define BOOST_NO_RTTI

#include "algorithm/DispatchQueue.hpp"
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
namespace net {

class NetworkManager;
class DCO;
class PCO;
class SSDO;
class CSDO;

class WRTCServer {
public:
  WRTCServer(NetworkManager* nm);
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
  DispatchQueue* WRTCQueue; // uses parent thread (same thread)
  // rtc::ThreadChecker thread_checker_;
  // The data channel used to communicate.
  rtc::scoped_refptr<webrtc::DataChannelInterface>
      data_channel; // RTC_GUARDED_BY(thread_checker_);
  // The peer connection through which we engage in the Session Description
  // Protocol (SDP) handshake.
  rtc::CriticalSection pc_mutex_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface>
      peer_connection; // RTC_GUARDED_BY(pc_mutex_); // TODO: multiple clients?
  NetworkManager* nm_;
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
                             // rtc::scoped_refptr<PCO> peer_connection_observer
                             // = new
                             // rtc::RefCountedObject<PCO>(OnDataChannelCreated,
                             // OnIceCandidate);
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

} // namespace net
} // namespace utils
