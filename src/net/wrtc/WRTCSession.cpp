#include "net/wrtc/WRTCSession.hpp" // IWYU pragma: associated
#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "algo/StringUtils.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/wrtc/Observers.hpp"
#include "net/wrtc/PeerConnectivityChecker.hpp"
#include "net/wrtc/WRTCServer.hpp"
#include "net/wrtc/wrtc.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
#include <api/call/callfactoryinterface.h>
#include <api/jsep.h>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <logging/rtc_event_log/rtc_event_log_factory_interface.h>
#include <p2p/base/portallocator.h>
#include <rapidjson/encodings.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <thread>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/media/base/mediaengine.h>
#include <webrtc/rtc_base/bind.h>
#include <webrtc/rtc_base/checks.h>
#include <webrtc/rtc_base/copyonwritebuffer.h>
#include <webrtc/rtc_base/rtccertificategenerator.h>
#include <webrtc/rtc_base/scoped_ref_ptr.h>
#include <webrtc/rtc_base/ssladapter.h>
#include <webrtc/rtc_base/thread.h>

namespace {

using namespace ::gloer::net::wrtc;

std::unique_ptr<webrtc::IceCandidateInterface>
createIceCandidateFromJson(const rapidjson::Document& message_object) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "createIceCandidateFromJson";
  std::string candidate = message_object["payload"][kCandidateSdpName].GetString();
  LOG(INFO) << "got candidate from client =" << candidate;
  int sdp_mline_index = message_object["payload"][kCandidateSdpMlineIndexName].GetInt();
  std::string sdp_mid = message_object["payload"][kCandidateSdpMidName].GetString();
  webrtc::SdpParseError error;
  // TODO: free memory?
  // TODO: handle error?
  std::unique_ptr<webrtc::IceCandidateInterface> iceCanidate(
      webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error));
  if (!iceCanidate.get()) {
    LOG(WARNING) << "createIceCandidateFromJson:: iceCanidate IS NULL" << error.description.c_str();
  }
  return iceCanidate;
}

static bool IsValidPortRange(uint16_t min_port, uint16_t max_port) {
  RTC_DCHECK(min_port <= max_port);
  return min_port != 0 && max_port != 0;
}

} // namespace

namespace gloer {
namespace net {
namespace wrtc {

const boost::posix_time::time_duration WRTCSession::timerDeadlinePeriod =
    boost::posix_time::seconds(10);

WRTCSession::WRTCSession(NetworkManager* nm, const std::string& webrtcId, const std::string& wsId)
    : SessionBase(webrtcId), lastDataChannelstate_(webrtc::DataChannelInterface::kClosed), nm_(nm),
      wsId_(wsId), isClosing_(false) {

  RTC_DCHECK(nm_ != nullptr);

  RTC_DCHECK_GT(webrtcId.length(), 0);
  RTC_DCHECK_GT(wsId.length(), 0);

  RTC_DCHECK_LT(webrtcId.length(), MAX_ID_LEN);
  RTC_DCHECK_LT(wsId.length(), MAX_ID_LEN);

  // wrtc session requires ws session (only at creation time)
  auto wsSess = nm_->getWS()->getSessById(wsId_);
  RTC_DCHECK(wsSess.get() != nullptr);
  if (!wsSess || !wsSess.get()) {
    LOG(WARNING) << "onAnswerCreated: Invalid getSessById for " << wsId_;
    close_s(false, false); // NOTE: both ws and wrtc must exist at the same time
    return;
  }

  // @see
  // webrtc.googlesource.com/src/+/master/examples/objcnativeapi/objc/objc_call_client.mm#63
  // Changes the thread that is checked for in CalledOnValidThread. This may
  // be useful when an object may be created on one thread and then used
  // exclusively on another thread.
  // RTC_DCHECK_RUN_ON(&thread_checker_);
  thread_checker_.DetachFromThread();
}

WRTCSession::~WRTCSession() {
  // RTC_DCHECK_RUN_ON(&thread_checker_);
  // LOG(INFO) << "~WRTCSession";

  {
    auto closeHook = [this] {
      RTC_DCHECK_RUN_ON(signalingThread());

      // free resources in order
      close_s(true, true);

      // ensure resources are freed
      while (!sendQueue_.isEmpty()) {
        sendQueue_.popFront();
      }
      dataChannelObserver_ = nullptr;
      peerConnectionObserver_ = nullptr;    // used in pci_ = CreatePeerConnection
      localDescriptionObserver_ = nullptr;  // used in pci_->SetLocalDescription
      remoteDescriptionObserver_ = nullptr; // used in pci_->SetRemoteDescription
      createSDO_ = nullptr;                 // used in  pci_->CreateAnswer*/
      pci_ = nullptr;
      connectionChecker_ = nullptr;
    };

    if (!nm_->getWRTC()->signalingThread()->IsCurrent()) {
      nm_->getWRTC()->signalingThread()->Invoke<void>(RTC_FROM_HERE, closeHook);
    } else {
      closeHook();
    }
  }
}

void WRTCSession::close_s(bool closePci, bool resetChannelObserver) {
  LOG(WARNING) << "WRTCSession::close_s";
  const std::string wrtcConnId = getId(); // remember id before session deletion

  // @see
  // github.com/WebKit/webkit/blob/master/Source/ThirdParty/libwebrtc/Source/webrtc/pc/peerconnection.cc#L849
  RTC_DCHECK_RUN_ON(signalingThread());

  /*{
    if (!nm_->getWRTC()->signaling_thread()->IsCurrent()) {
      nm_->getWRTC()->signaling_thread()->Invoke<void>(RTC_FROM_HERE,
                                                       [this] { return setClosing(true); });
    } else {
      setClosing(true);
    }
  }*/

  if (!onCloseCallback_) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Not set onMessageCallback_!";
    return;
  }

  while (!sendQueue_.isEmpty()) {
    sendQueue_.popFront();
  }

  onCloseCallback_(wrtcConnId);

  if (connectionChecker_.get()) {
    connectionChecker_->close();
    connectionChecker_.reset();
  }

  // Need to stop transceivers before destroying the stats collector because
  // AudioRtpSender has a reference to the StatsCollector it will update when
  // stopping.
  /*for (const auto& transceiver : transceivers_) {
    transceiver->Stop();
  }

  stats_.reset(nullptr);
  if (stats_collector_) {
    stats_collector_->WaitForPendingRequest();
    stats_collector_ = nullptr;
  }*/

  while (isSendBusy_) {
    LOG(WARNING) << "isSendBusy_ on WRTC close";
    rtc::Thread::Current()->SleepMs(1);
    // ProcessMessages will process I/O and dispatch messages until:
    //  1) cms milliseconds have elapsed (returns true)
    //  2) Stop() is called (returns false)
    rtc::Thread::Current()->ProcessMessages(/* cms milliseconds */ 1000);
  }

  CloseDataChannel(resetChannelObserver);

  // port_allocator_ lives on the network thread and should be destroyed there.
  // see
  // github.com/WebKit/webkit/blob/master/Source/ThirdParty/libwebrtc/Source/webrtc/pc/peerconnection.cc#L877
  if (!nm_->getWRTC()->networkThread()->IsCurrent()) {
    networkThread()->Invoke<void>(RTC_FROM_HERE, [this] { portAllocator_.reset(); });
  } else {
    portAllocator_ = nullptr;
  }

  if (closePci) {
    rtc::CritScope lock(&peerConIMutex_);
    if (pci_ && pci_.get()) {
      if (pci_->signaling_state() != webrtc::PeerConnectionInterface::kClosed) {
        // Terminates all media, closes the transports, and in general releases any
        // resources used by the PeerConnection. This is an irreversible operation.
        //
        // Note that after this method completes, the PeerConnection will no longer
        // use the PeerConnectionObserver interface passed in on construction, and
        // thus the observer object can be safely destroyed.
        pci_->Close();
      }
      {
        // clear observers after closing of PeerConnection
        // NOTE: don`t reset observer if called from observer
        peerConnectionObserver_ = nullptr;    // used in pci_ = CreatePeerConnection
        localDescriptionObserver_ = nullptr;  // used in pci_->SetLocalDescription
        remoteDescriptionObserver_ = nullptr; // used in pci_->SetRemoteDescription
        createSDO_ = nullptr;                 // used in  pci_->CreateAnswer*/
      }
      pci_ = nullptr;
    }
  }

  LOG(WARNING) << "close_s";
  setFullyCreated(true); // allows auto-deletion
}

void WRTCSession::setClosing(bool closing) {
  LOG(INFO) << rtc::Thread::Current()->name() << ":"
            << "WRTCSession::setClosing " << closing;

  if (!signalingThread()->IsCurrent()) {
    return signalingThread()->Invoke<void>(RTC_FROM_HERE,
                                           [this, closing] { return setClosing(closing); });
  }

  RTC_DCHECK_RUN_ON(signalingThread());

  isClosing_ = closing;
}

bool WRTCSession::isClosing() const {
  if (!signalingThread()->IsCurrent()) {
    return signalingThread()->Invoke<bool>(RTC_FROM_HERE, [this] { return isClosing(); });
  }

  RTC_DCHECK_RUN_ON(signalingThread());

  /*LOG(INFO) << rtc::Thread::Current()->name() << ":"
            << "WRTCSession::isClosing " << isClosing_;*/
  /*{
    if (!nm_->getWRTC()->network_thread()->IsCurrent()) {
      return nm_->getWRTC()->network_thread()->Invoke<bool>(RTC_FROM_HERE,
                                                            [this] { return isClosing(); });
    }
  }*/
  return isClosing_;
}

/**
 * 16 Kbyte for the highest throughput, while also being the most portable one
 * @see viblast.com/blog/2015/2/5/webrtc-data-channel-message-size/
 **/
size_t WRTCSession::MAX_IN_MSG_SIZE_BYTE = 16 * 1024;
size_t WRTCSession::MAX_OUT_MSG_SIZE_BYTE = 16 * 1024;

void WRTCSession::createPeerConnectionObserver() {
  RTC_DCHECK_RUN_ON(signalingThread());

  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::finishThreads: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(
          RTC_FROM_HERE, [this] { return createPeerConnectionObserver(); });
    }
  }*/

  // nm_->getWRTC()->workerThread_->Post(RTC_FROM_HERE, this,
  // static_cast<uint32_t>(Message::RestartRelays)); nm_->getWRTC()->workerThread_->PostAt()

  /**
   * @brief WebRTC peer connection observer
   * Used to create WebRTC session paired with WebSocket session
   */
  std::shared_ptr<PCO> peerConnectionObserver =
      std::make_shared<PCO>(nm_, getId(), wsId_); // TODO: to private

  RTC_DCHECK(peerConnectionObserver.get() != nullptr);
  if (!peerConnectionObserver || !peerConnectionObserver.get()) {
    LOG(WARNING) << "empty peer_connection_observer";
    close_s(false, false);
    return;
  }

  // save at WRTCSession to prevent garbage collection
  peerConnectionObserver_ = peerConnectionObserver;
}

void WRTCSession::CloseDataChannel(bool resetObserver) {
  RTC_DCHECK_RUN_ON(signalingThread());
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::finishThreads: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(
          RTC_FROM_HERE, [this] { return CloseDataChannel(nm, in_data_channel, pci); });
    }
  }*/

  /*if (nm->getWRTC()->workerThread_.get() && !nm->getWRTC()->workerThread_->IsCurrent()) {
    auto handle = OnceFunctor([this, nm, &in_data_channel, pci]() {
      WRTCSession::CloseDataChannel(nm, in_data_channel, pci);
    });
    nm->getWRTC()->workerThread_->Post(RTC_FROM_HERE, handle);
    return;
  }*/

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "CloseDataChannel";

  {

    if (dataChannelI_ && dataChannelI_.get()) {
      // Used to receive events from the data channel. Only one observer can be
      // registered at a time. UnregisterObserver should be called before the
      // observer object is destroyed.
      // if (dataChannelI_->hasObserver())
      dataChannelI_->UnregisterObserver();

      if (resetObserver) {
        // NOTE: don`t reset observer if called from same observer
        dataChannelObserver_ = nullptr; // used in dataChannelI_->RegisterObserver
      }

      if (dataChannelI_->state() != webrtc::DataChannelInterface::kClosed) {
        dataChannelI_->Close();
      }

      // resetObserver == called from destructor
      if (resetObserver) {
        // call manually cause observer deleted
        if (!signalingThread()->IsCurrent()) {
          signalingThread()->Invoke<void>(RTC_FROM_HERE,
                                          [this] { return onDataChannelDeallocated(); });
        } else {
          onDataChannelDeallocated();
        }

        /*dataChannelI_ = nullptr;*/
      }
    }

    {
      rtc::CritScope lock(&lastStateMutex_);
      lastDataChannelstate_ = webrtc::DataChannelInterface::kClosed;
    }

    // in_data_channel = nullptr;

    /*if (sess && sess.get()) {
      LOG(WARNING) << "WRTCServer::unregisterSession: clean observers...";
      sess->updateDataChannelState();
      if (sess->localDescriptionObserver_) {
        // sess->localDescriptionObserver_->Release();
        sess->localDescriptionObserver_.release();
      }
      if (sess->remoteDescriptionObserver_) {
        // sess->remoteDescriptionObserver_->Release();
        sess->remoteDescriptionObserver_.release();
      }
      if (sess->createSDO_) {
        // sess->createSDO_->Release();
        sess->createSDO_.release();
      }
      if (sess->dataChannelObserver_) {
        // sess->dataChannelObserver_->Release();
        // sess->dataChannelObserver_.release();
        sess->dataChannelObserver_.reset();
      }
    }*/
  }
}

// see
// github.com/WebKit/webkit/blob/master/Source/ThirdParty/libwebrtc/Source/webrtc/pc/peerconnection.cc#L944
bool WRTCSession::InitializePortAllocator_n() {
  RTC_DCHECK_RUN_ON(networkThread());

  if (!portAllocator_) {
    portAllocator_ = std::make_unique<cricket::BasicPortAllocator>(
        nm_->getWRTC()->wrtcNetworkManager_.get(), nm_->getWRTC()->socketFactory_.get());
  } else {
    LOG(WARNING) << "Recreating portAllocator_";
  }

  RTC_DCHECK(portAllocator_.get() != nullptr);
  if (!portAllocator_ || !portAllocator_.get()) {
    close_s(false, false);
    LOG(WARNING) << "WRTCServer::createPeerConnection: invalid portAllocator_";
    return false;
  }

  // NOTE: We need port per session
  // This doesn't make everything go on one port, but should limit the number.
  // If you don't expect many concurrent connections you can open only a small range of ports.
  if (IsValidPortRange(minPort_, maxPort_)) {
    portAllocator_->SetPortRange(minPort_, maxPort_);
  }

  // TODO: more setting for portAllocator_
  // https://github.com/mobhuyan/webrtc/blob/98a867ccd2af391267d0568f279dd3274e623f81/pc/peerconnection.cc#L4232

  if (!enableEnumeratingAllNetworkInterfaces_) {
    portAllocator_->set_flags(portAllocator_->flags() |
                              cricket::PORTALLOCATOR_DISABLE_ADAPTER_ENUMERATION);
  }

  return true;
}

void WRTCSession::createPeerConnection() {
  RTC_DCHECK_RUN_ON(signalingThread());
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::finishThreads: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(RTC_FROM_HERE,
                                                         [this] { return createPeerConnection(); });
    }
  }*/

  LOG(WARNING) << "creating portAllocator_...";

  // see github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h
  // see github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peer.cpp

  // The port allocator lives on the network thread and should be initialized
  // there.
  // InitializePortAllocator();
  if (!nm_->getWRTC()->networkThread()->Invoke<bool>(
          RTC_FROM_HERE, ::rtc::Bind(&WRTCSession::InitializePortAllocator_n, this))) {
    LOG(WARNING) << "WRTCServer::createPeerConnection: invalid portAllocator_";
    return;
  }

  LOG(WARNING) << "creating PeerConnection...";

  {
    // TODO rtc::CritScope lock(nm_->getWRTC()->pcfMutex_, &peerConIMutex_);

    // prevents pci_ garbage collection by 'operator='
    rtc::CritScope lock(&peerConIMutex_);

    RTC_DCHECK(nm_->getWRTC()->peerConnectionFactory_.get() != nullptr);
    if (!nm_->getWRTC()->peerConnectionFactory_.get()) {
      close_s(false, false);
      LOG(WARNING) << "Error: Invalid CreatePeerConnectionFactory.";
      return;
    }

    RTC_DCHECK(peerConnectionObserver_.get() != nullptr);
    if (!peerConnectionObserver_.get()) {
      close_s(false, false);
      LOG(WARNING) << "Error: Invalid peerConnectionObserver_.";
      return;
    }

    pci_ = nm_->getWRTC()->peerConnectionFactory_->CreatePeerConnection(
        nm_->getWRTC()->getWRTCConf(), /*std::move(portAllocator_)*/ nullptr,
        /* cert_generator */ nullptr, peerConnectionObserver_.get());

    RTC_DCHECK(pci_.get() != nullptr);
    if (!pci_ || !pci_.get()) {
      close_s(false, false);
      LOG(WARNING) << "WRTCServer::createPeerConnection: empty PCI";
      return;
    }
  }
}

void WRTCSession::setObservers(bool isServer) {
  RTC_DCHECK_RUN_ON(&thread_checker_);
  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::createPeerConnection: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(RTC_FROM_HERE,
                                                         [this] { return setObservers(); });
    }
  }*/

  LOG(INFO) << "creating CSDO ";
  createSDO_ = new rtc::RefCountedObject<CSDO>(isServer, nm_, shared_from_this());

  RTC_DCHECK(createSDO_.get() != nullptr);
  if (!createSDO_ || !createSDO_.get()) {
    LOG(WARNING) << "empty CSDO";
    close_s(false, false);
    return;
  }

  LOG(INFO) << "creating localDescriptionObserver_ ";
  localDescriptionObserver_ = new rtc::RefCountedObject<SSDO>(nm_, shared_from_this());

  RTC_DCHECK(localDescriptionObserver_.get() != nullptr);
  if (!localDescriptionObserver_ || !localDescriptionObserver_.get()) {
    LOG(WARNING) << "empty localDescriptionObserver_";
    close_s(false, false);
    return;
  }
}

bool WRTCSession::isExpired() const {

  if (!signalingThread()->IsCurrent()) {
    return signalingThread()->Invoke<bool>(RTC_FROM_HERE, [this] { return isExpired(); });
  }

  RTC_DCHECK_RUN_ON(signalingThread());

  const bool isTimerExpired = boost::posix_time::second_clock::local_time() > timerDeadline;
  if (isTimerExpired) {
    return true;
  }
  return false;
}

/*
rtc::scoped_refptr<webrtc::PeerConnectionInterface> WRTCSession::getPCI() const {
  return pci_;
}*/

/*rtc::scoped_refptr<webrtc::DataChannelInterface> WRTCSession::getDataChannelI() const {
  return dataChannelI_;
}*/

void WRTCSession::send(const std::string& data) {
  // RTC_DCHECK_RUN_ON(&thread_checker_);

  /*if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
    auto nm = nm_;
    auto handle = OnceFunctor([this, data]() { send(data); });
    nm_->getWRTC()->workerThread_->Post(RTC_FROM_HERE, handle);
    return;
  }*/

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::sendDataViaDataChannel const std::string&";*/
  WRTCSession::send(nm_, shared_from_this(), data);
}

bool WRTCSession::send(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess,
                       const std::string& data) {
  // RTC_DCHECK_RUN_ON(&wrtcSess->thread_checker_);
  // LOG(WARNING) << "WRTCSession::send 1";
  const bool isClosing_n = nm->getWRTC()->signalingThread()->Invoke<bool>(
      RTC_FROM_HERE, [wrtcSess] { return wrtcSess->isClosing(); });
  if (isClosing_n) {
    // session is closing...
    return false;
  }

  /*if (!nm->getWRTC()->workerThread_->IsCurrent()) {
    auto handle = OnceFunctor(
        [nm, wrtcSess, data]() { WRTCSession::sendDataViaDataChannel(nm, wrtcSess, data); });
    nm->getWRTC()->workerThread_->Post(RTC_FROM_HERE, handle);
    return false;
  }*/

  /*{
    if (!nm->getWRTC()->workerThread_ || !nm->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return false;
    }

    if (!nm->getWRTC()->workerThread_->IsCurrent()) {
      return nm->getWRTC()->workerThread_->Invoke<bool>(
          RTC_FROM_HERE, [this] { return sendDataViaDataChannel(nm, wrtcSess, data); });
    }
  }*/

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::sendDataViaDataChannel std::shared_ptr<WRTCSession> wrtcSess, const "
               "std::string& data";*/

  if (!wrtcSess || !wrtcSess.get()) {
    LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: wrtc session is not established";
    return false;
  }

  if (!wrtcSess->isDataChannelOpen()) {
    LOG(WARNING) << "sendDataViaDataChannel: dataChannel not open!";
    return false;
  }

  // Construct a buffer and copy the specified number of bytes into it. The
  // source array may be (const) uint8_t*, int8_t*, or char*.
  // webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(data.c_str(), data.size()), /* binary */
  // false);
  // send(nm, wrtcSess, std::move(buffer)); // use queue instead

  // check buffer size
  {
    if (!data.size()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: Invalid messageBuffer";
      return false;
    }

    if (data.size() > MAX_OUT_MSG_SIZE_BYTE) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: Too big messageBuffer of size "
                   << data.size();
      return false;
    }
  }

  // check pointer
  {
    if (!wrtcSess || !wrtcSess.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: wrtc session is not established";
      return false;
    }
  }

  // write to send queue
  {
    if (!wrtcSess->sendQueue_.isFull()) {
      wrtcSess->sendQueue_.write(std::make_shared<std::string>(data));
    } else {
      // Too many messages in queue
      LOG(WARNING) << "WRTC send_queue_ isFull!";
      return false;
    }
  }

  // LOG(WARNING) << "WRTCSession::send 2";
  sendQueued(nm, wrtcSess);

  return true;
}

bool WRTCSession::sendQueued(NetworkManager* nm, std::shared_ptr<WRTCSession> wrtcSess) {
  // check pointer
  {
    if (!wrtcSess || !wrtcSess.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: wrtc session is not established";
      return false;
    }
  }

  if (!wrtcSess->signalingThread()->IsCurrent()) {
    return wrtcSess->signalingThread()->Invoke<bool>(
        RTC_FROM_HERE, [nm, wrtcSess] { return wrtcSess->sendQueued(nm, wrtcSess); });
  }

  RTC_DCHECK_RUN_ON(wrtcSess->signalingThread());

  bool isClosing_n = false;
  if (!wrtcSess->signalingThread()->IsCurrent()) {
    isClosing_n = wrtcSess->signalingThread()->Invoke<bool>(
        RTC_FROM_HERE, [wrtcSess] { return wrtcSess->isClosing(); });
  } else {
    isClosing_n = wrtcSess->isClosing();
  }
  if (isClosing_n) {
    // session is closing...
    nm->getWRTC()->unregisterSession(wrtcSess->getId());
    return false;
  }

  // LOG(WARNING) << "WRTCSession::send 3";

  /*if (!nm->getWRTC()->networkThread_->IsCurrent()) {
    auto handle = OnceFunctor([nm, wrtcSess]() { WRTCSession::sendQueued(nm, wrtcSess); });
    nm->getWRTC()->networkThread_->Post(RTC_FROM_HERE, handle);
    return false;
  }*/

  /*{
    if (!nm->getWRTC()->workerThread_ || !nm->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return false;
    }

    if (!nm->getWRTC()->workerThread_->IsCurrent()) {
      // Uses Send() internally, which blocks the current thread until execution
      // is complete.
      // NOTE: This function can only be called when synchronous calls are allowed.
      return nm->getWRTC()->workerThread_->Invoke<bool>(
          RTC_FROM_HERE, [this] { return sendDataViaDataChannel(nm, wrtcSess, buffer); });
    }
  }*/

  /*rtc::MessageHandler* mh = new MessageHandler();
  nm->getWRTC()->workerThread_->Post(RTC_FROM_HERE, mh);*/

  /*LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::sendDataViaDataChannel const webrtc::DataBuffer& buffer";*/

  if (!wrtcSess->isSendBusy_ && !wrtcSess->sendQueue_.isEmpty()) {
    wrtcSess->isSendBusy_ = true;

    if (!wrtcSess->sendQueue_.frontPtr() || !wrtcSess->sendQueue_.frontPtr()->get()) {
      LOG(WARNING) << "WRTC: invalid sendQueue_.frontPtr()";
      wrtcSess->isSendBusy_ = false;
      return false;
    }

    // std::shared_ptr<const std::string> dp = *(sendQueue_.frontPtr());
    std::shared_ptr<const std::string> dp;
    {
      wrtcSess->sendQueue_.read(dp);

      if (!wrtcSess->sendQueue_.isEmpty()) {
        // Remove the already written string from the queue
        // sendQueue_.erase(sendQueue_.begin());
        wrtcSess->sendQueue_.popFront();
      }
    }

    // check buffer size
    {
      if (!dp->size()) {
        LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: Invalid messageBuffer";
        wrtcSess->isSendBusy_ = false;
        return false;
      }

      if (dp->size() > MAX_OUT_MSG_SIZE_BYTE) {
        LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: Too big messageBuffer of size "
                     << dp->size();
        wrtcSess->isSendBusy_ = false;
        return false;
      }
    }

    webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(dp->c_str(), dp->size()), /* binary */ false);

    if (!dp || !dp.get()) {
      LOG(WARNING) << "WRTC invalid sendQueue_.front()) ";
      wrtcSess->isSendBusy_ = false;
      return false;
    }

    // We are not currently writing, so send this immediately
    {
      if (!wrtcSess->isDataChannelOpen()) {
        LOG(WARNING) << "sendDataViaDataChannel: dataChannel not open!";
        wrtcSess->isSendBusy_ = false;
        return false;
      }

      if (!wrtcSess->dataChannelI_ || !wrtcSess->dataChannelI_.get()) {
        LOG(WARNING) << "sendDataViaDataChannel: empty peer_connection!";
        wrtcSess->isSendBusy_ = false;
        return false;
      }

      /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "wrtcSess->dataChannelI_->Send " << buffer.size();*/

      // Returns the number of bytes of application data (UTF-8 text and binary
      // data) that have been queued using Send but have not yet been processed at
      // the SCTP level. See comment above Send below.
      const uint64_t buffered_bytes = wrtcSess->dataChannelI_->buffered_amount();
      // LOG(WARNING) << "buffered_bytes = " << buffered_bytes;
      if (buffered_bytes > wrtcSess->MAX_TO_BUFFER_BYTES) {
        LOG(WARNING) << "REACHED DATACHANNEL MAX_TO_BUFFER_BYTES!";
        // wrtcSess->isSendBusy_ = false;
        // return false;
      }

      // Sends |data| to the remote peer. If the data can't be sent at the SCTP
      // level (due to congestion control), it's buffered at the data channel level,
      // up to a maximum of 16MB. If Send is called while this buffer is full, the
      // data channel will be closed abruptly.
      //
      // So, it's important to use buffered_amount() and OnBufferedAmountChange to
      // ensure the data channel is used efficiently but without filling this
      // buffer.
      // LOG(WARNING) << "WRTCSession::send 4 " << dp->c_str() << " size " << buffer.size();
      // LOG(WARNING) << "wrtcSess->dataChannelI_->state() " << wrtcSess->dataChannelI_->state();
      // LOG(WARNING) << "IsStable() " << wrtcSess->IsStable();
      if (!wrtcSess->dataChannelI_->Send(std::move(buffer))) {
        LOG(WARNING) << "Can`t send via dataChannelI_";
        switch (wrtcSess->dataChannelI_->state()) {
        case webrtc::DataChannelInterface::kConnecting: {
          LOG(WARNING)
              << "WRTCSession::sendDataViaDataChannel: Unable to send arraybuffer. DataChannel "
                 "is connecting";
          break;
        }
        case webrtc::DataChannelInterface::kOpen: {
          LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: Unable to send arraybuffer.";
          break;
        }
        case webrtc::DataChannelInterface::kClosing: {
          LOG(WARNING)
              << "WRTCSession::sendDataViaDataChannel: Unable to send arraybuffer. DataChannel "
                 "is closing";
          break;
        }
        case webrtc::DataChannelInterface::kClosed: {
          LOG(WARNING)
              << "WRTCSession::sendDataViaDataChannel: Unable to send arraybuffer. DataChannel "
                 "is closed";
          break;
        }
        default: {
          LOG(WARNING)
              << "WRTCSession::sendDataViaDataChannel: Unable to send arraybuffer. DataChannel "
                 "unknown state";
          break;
        }
        }
      }

      // send queued messages too
      if (!wrtcSess->sendQueue_.isEmpty()) {
        sendQueued(nm, wrtcSess);
      }

      // AsyncInvoke TODO: use
      // github.com/WebKit/webkit/blob/master/Source/ThirdParty/libwebrtc/Source/webrtc/pc/peerconnection.cc#L6133
      wrtcSess->isSendBusy_ = false;
    }
  }

  return true;
}

void WRTCSession::setFullyCreated(bool isFullyCreated) {
  // RTC_DCHECK_RUN_ON(signaling_thread());

  // rtc::CritScope lock(&FullyCreatedMutex_);

  isFullyCreated_ = isFullyCreated;
}

/*std::unique_ptr<webrtc::SessionDescriptionInterface>
WRTCSession::createSessionDescription(const std::string& type, const std::string& sdp) {
  {
    if (!signalingThread()->IsCurrent()) {
      return signalingThread()->Invoke<std::unique_ptr<webrtc::SessionDescriptionInterface>>(
          RTC_FROM_HERE, [this, &type, &sdp] { return createSessionDescription(type, sdp); });
    }
  }

  RTC_DCHECK_RUN_ON(signalingThread());

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "createSessionDescriptionFromJson";

  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::SessionDescriptionInterface> sdi = webrtc::CreateSessionDescription(
      (type == "offer" ? webrtc::SdpType::kOffer : webrtc::SdpType::kAnswer), sdp, &error);
  if (sdi == nullptr) {
    LOG(WARNING) << "createSessionDescriptionFromJson:: SDI IS NULL" << error.description.c_str();
    LOG(WARNING) << error.description;
  }
  return sdi;
}*/

webrtc::SessionDescriptionInterface* WRTCSession::createSessionDescription(const std::string& type,
                                                                           const std::string& sdp) {
  {
    if (!signalingThread()->IsCurrent()) {
      return signalingThread()->Invoke<webrtc::SessionDescriptionInterface*>(
          RTC_FROM_HERE, [this, &type, &sdp] { return createSessionDescription(type, sdp); });
    }
  }

  RTC_DCHECK_RUN_ON(signalingThread());

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "createSessionDescriptionFromJson";

  webrtc::SdpParseError error;
  webrtc::SessionDescriptionInterface* sdi = webrtc::CreateSessionDescription(type, sdp, &error);
  if (sdi == nullptr) {
    LOG(WARNING) << "createSessionDescriptionFromJson:: SDI IS NULL" << error.description.c_str();
    LOG(WARNING) << error.description;
  }
  return sdi;
}

webrtc::DataChannelInterface::DataState WRTCSession::updateDataChannelState() {
  {
    if (!signalingThread()->IsCurrent()) {
      return signalingThread()->Invoke<webrtc::DataChannelInterface::DataState>(
          RTC_FROM_HERE, [this] { return updateDataChannelState(); });
    }
  }

  RTC_DCHECK_RUN_ON(signalingThread());

  rtc::CritScope lock(&lastStateMutex_);
  lastDataChannelstate_ = webrtc::DataChannelInterface::kClosed;

  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return webrtc::DataChannelInterface::kClosed;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<webrtc::DataChannelInterface::DataState>(
          RTC_FROM_HERE, [this] { return updateDataChannelState(); });
    }
  }*/

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::updateDataChannelState";

  if (!dataChannelI_ || !dataChannelI_.get()) {
    LOG(WARNING) << "WRTCSession::updateDataChannelState: Data channel is not established";
    return webrtc::DataChannelInterface::kClosed;
  }

  lastDataChannelstate_ = dataChannelI_->state();

  if (lastDataChannelstate_ == webrtc::DataChannelInterface::kClosed) {
    close_s(false, false);
  }

  return lastDataChannelstate_;
}

void WRTCSession::setLocalDescription(webrtc::SessionDescriptionInterface* sdi) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::setLocalDescription";

  RTC_DCHECK_RUN_ON(signalingThread());

  {
    /*LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::setLocalDescription peerConIMutex_";*/
    rtc::CritScope lock(&peerConIMutex_);

    if (isClosing()) {
      LOG(WARNING) << "setLocalDescription: isClosing";
      return;
    }

    RTC_DCHECK(localDescriptionObserver_.get() != nullptr);
    RTC_DCHECK(localDescriptionObserver_->wrtcSess_.lock().get() != nullptr);
    RTC_DCHECK(localDescriptionObserver_->wrtcSess_.lock()->getId() == getId());
    RTC_DCHECK(nm_->getWRTC()->getSessById(getId()).get() != nullptr);
    RTC_DCHECK(nm_->getWRTC()->getSessById(getId())->getId() == getId());
    RTC_DCHECK(nm_->getWRTC()->getSessById(getId())->wsId_ == wsId_);
    RTC_DCHECK(nm_->getWS()->getSessById(wsId_).get() != nullptr);
    RTC_DCHECK(nm_->getWS()->getSessById(wsId_)->getWRTCSession().lock().get() != nullptr);
    RTC_DCHECK(nm_->getWS()->getSessById(wsId_)->getWRTCSession().lock()->getId() == getId());
    if (!localDescriptionObserver_.get()) {
      LOG(WARNING) << "empty local_description_observer";
      // close_s(false, false);
      return;
    }

    RTC_DCHECK(sdi != nullptr);
    if (!sdi) {
      LOG(WARNING) << "empty sdi";
      // close_s(false, false);
      return;
    }

    RTC_DCHECK(pci_.get() != nullptr);
    if (!pci_ || !pci_.get()) {
      LOG(WARNING) << "setLocalDescription: empty peer_connection!";
      close_s(false, false);
      return;
    }

    // store the server’s own answer
    pci_->SetLocalDescription(localDescriptionObserver_.get(), sdi);
  }
  // setLocalDescription(&local_description_observer, sdi);
}

void WRTCSession::createAndAddIceCandidateFromJson(const rapidjson::Document& message_object) {
  {
    if (!signalingThread()->IsCurrent()) {
      return signalingThread()->Invoke<void>(RTC_FROM_HERE, [this, &message_object] {
        return createAndAddIceCandidateFromJson(message_object);
      });
    }
  }

  RTC_DCHECK_RUN_ON(signalingThread());

  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(
          RTC_FROM_HERE, [this] { return createAndAddIceCandidate(message_object); });
    }
  }*/

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::createAndAddIceCandidate";
  auto candidate_object = createIceCandidateFromJson(message_object);
  {
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::createAndAddIceCandidate peerConIMutex_";
    rtc::CritScope lock(&peerConIMutex_);

    if (isClosing()) {
      LOG(WARNING) << "createAndAddIceCandidate: isClosing";
      return;
    }

    RTC_DCHECK(pci_.get() != nullptr);
    if (!pci_ || !pci_.get()) {
      LOG(WARNING) << "createAndAddIceCandidate: empty peer_connection!";
      close_s(false, false);
      return;
    }

    // * A "pending" description is one that's part of an incomplete offer/answer
    // exchange (thus, either an offer or a pranswer). Once the offer/answer
    // exchange is finished, the "pending" description will become "current".
    // * A "current" description the one currently negotiated from a complete
    // offer/answer exchange.
    RTC_DCHECK(pci_->pending_remote_description() || pci_->current_remote_description());
    // @see github.com/vmolsa/libcrtc/blob/master/src/rtcpeerconnection.cc#L134
    if (!pci_->pending_remote_description() && !pci_->current_remote_description()) {
      LOG(WARNING) << "ICE candidates can't be added without any remote session description.";
      return;
    }

    // sends IceCandidate to Client via websockets, see OnIceCandidate
    if (!pci_->AddIceCandidate(candidate_object.get())) {
      LOG(WARNING) << "createAndAddIceCandidate: Failed to apply the received candidate!";
      return;
    }
  }
}

bool WRTCSession::isDataChannelOpen() {

  rtc::CritScope lock(&lastStateMutex_);

  // RTC_DCHECK_RUN_ON(&thread_checker_);

  /*LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::isDataChannelOpen";*/
  if (!dataChannelI_ || !dataChannelI_.get()) {
    LOG(WARNING) << "WRTCSession::isDataChannelOpen: Data channel is not established";

    return false;
  }

  return lastDataChannelstate_ == webrtc::DataChannelInterface::kOpen &&
         dataChannelI_->state() == webrtc::DataChannelInterface::kOpen;
}

void WRTCSession::createDCI() {
  RTC_DCHECK_RUN_ON(signalingThread());

  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(RTC_FROM_HERE,
                                                         [this] { return createDCI(); });
    }
  }*/

  LOG(INFO) << "creating DataChannel...";
  const std::string data_channel_lable = "dc_" + getId();

  {
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::createDCI peerConIMutex_";
    rtc::CritScope lock(&peerConIMutex_);

    if (isClosing()) {
      LOG(WARNING) << "createDCI: isClosing";
      return;
    }

    RTC_DCHECK(dataChannelI_.get() == nullptr);
    if (dataChannelI_.get() != nullptr) {
      LOG(WARNING) << "dataChannelI_ already created";
      return;
    }

    // NOTE: that an offer/answer negotiation is still necessary
    // before the data channel can be used.
    //
    // Also, calling CreateDataChannel is the only way to get a data "m=" section
    // in SDP, so it should be done before CreateOffer is called, if the
    // application plans to use data channels.
    dataChannelI_ = pci_->CreateDataChannel(data_channel_lable, &nm_->getWRTC()->dataChannelConf_);
    LOG(INFO) << "created DataChannel";

    RTC_DCHECK(dataChannelI_.get() != nullptr);
    if (!dataChannelI_ || !dataChannelI_.get()) {
      LOG(WARNING) << "empty dataChannelI_";
      close_s(false, false);
      return;
    }

    // NOTE: DCO created after PCO::OnDataChannel event
    // NOTE: calls RegisterObserver from constructor
    dataChannelObserver_ = std::make_unique<DCO>(nm_, dataChannelI_, shared_from_this());

    RTC_DCHECK(dataChannelObserver_.get() != nullptr);
    if (!dataChannelObserver_ || !dataChannelObserver_.get()) {
      LOG(WARNING) << "empty dataChannelObserver_";
      close_s(false, false);
      return;
    }

    // Used to receive events from the data channel. Only one observer can be
    // registered at a time. UnregisterObserver should be called before the
    // observer object is destroyed.
    // dataChannelI_->RegisterObserver(dataChannelObserver_.get());

    onDataChannelAllocated();

    // NOTE: DCO observer will be created after PCO::OnDataChannel event

    RTC_DCHECK(isDataChannelOpen() == false);
  }
}

bool WRTCSession::fullyCreated() const {
  // RTC_DCHECK_RUN_ON(signaling_thread());

  // rtc::CritScope lock(&FullyCreatedMutex_);

  return isFullyCreated_;
}

void WRTCSession::setRemoteDescription(
    webrtc::SessionDescriptionInterface* clientSessionDescription) {
  LOG(INFO) << "SetRemoteDescription...";
  {
    if (!signalingThread()->IsCurrent()) {
      return signalingThread()->Invoke<void>(RTC_FROM_HERE, [this, &clientSessionDescription] {
        return setRemoteDescription(clientSessionDescription);
      });
    }
  }

  RTC_DCHECK_RUN_ON(signalingThread());

  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(
          RTC_FROM_HERE, [this] { return SetRemoteDescription(clientSessionDescription); });
    }
  }*/

  if (!clientSessionDescription) {
    LOG(WARNING) << "empty clientSessionDescription";
    // close_s(false, false);
    return;
  }

  {
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::SetRemoteDescription peerConIMutex_";
    {
      rtc::CritScope lock(&peerConIMutex_);

      if (isClosing()) {
        LOG(WARNING) << "SetRemoteDescription: isClosing";
        return;
      }

      // LOG(INFO) << "peerConIMutex_...";

      RTC_DCHECK(pci_.get() != nullptr);
      if (!pci_) {
        LOG(WARNING) << "empty peer_connection!";
        close_s(false, false);
        return;
      }
      // TODO: std::move(std::unique_ptr<webrtc::SessionDescriptionInterface>)
      // @see
      // https://github.com/enjoyvc/android_example/blob/55c3c26b551ef39106408e6a82d3737519913e04/androidnativeapi/jni/androidcallclient.cc#L256

      LOG(INFO) << "creating remoteDescriptionObserver_ ";
      remoteDescriptionObserver_ = new rtc::RefCountedObject<SSDO>(nm_, shared_from_this());

      RTC_DCHECK(remoteDescriptionObserver_.get() != nullptr);
      if (!remoteDescriptionObserver_ || !remoteDescriptionObserver_.get()) {
        LOG(WARNING) << "empty remote_description_observer";
        close_s(false, false);
        return;
      }

      pci_->SetRemoteDescription(remoteDescriptionObserver_.get(), clientSessionDescription);
    }
  }
}

void WRTCSession::CreateAnswer() {
  {
    if (!signalingThread()->IsCurrent()) {
      return signalingThread()->Invoke<void>(RTC_FROM_HERE, [this] { return CreateAnswer(); });
    }
  }

  RTC_DCHECK_RUN_ON(signalingThread());

  /*{
    if (!nm_->getWRTC()->workerThread_ || !nm_->getWRTC()->workerThread_.get()) {
      LOG(WARNING) << "WRTCSession::sendDataViaDataChannel: invalid workerThread_";
      return;
    }

    if (!nm_->getWRTC()->workerThread_->IsCurrent()) {
      return nm_->getWRTC()->workerThread_->Invoke<void>(RTC_FROM_HERE,
                                                         [this] { return CreateAnswer(); });
    }
  }*/

  // peer_connection->CreateAnswer(&m_WRTC->observer->create_session_description_observer,
  // nullptr);

  // SEE
  // chromium.googlesource.com/external/webrtc/+/HEAD/pc/peerconnection_signaling_unittest.cc
  // SEE
  // books.google.ru/books?id=jfNfAwAAQBAJ&pg=PT208&lpg=PT208&dq=%22RTCOfferAnswerOptions%22+%22CreateAnswer%22&source=bl&ots=U1c5gQMSlU&sig=UHb_PlSNaDpfim6dlx0__a8BZ8Y&hl=ru&sa=X&ved=2ahUKEwi2rsbqq7PfAhU5AxAIHWZCA204ChDoATAGegQIBxAB#v=onepage&q=%22RTCOfferAnswerOptions%22%20%22CreateAnswer%22&f=false
  // READ w3.org/TR/webrtc/#dom-rtcofferansweroptions
  // SEE gist.github.com/MatrixMuto/e37f50567e4b9b982dd8673a1e49dcbe

  // Answer to client offer with server media capabilities
  // Answer sents by websocker in OnAnswerCreated
  // CreateAnswer will trigger the OnSuccess event of
  // CreateSessionDescriptionObserver. This will in turn invoke our
  // OnAnswerCreated callback for sending the answer to the client.
  LOG(INFO) << "peer_connection->CreateAnswer...";
  {
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::CreateAnswer peerConIMutex_";
    rtc::CritScope lock(&peerConIMutex_);

    if (isClosing()) {
      LOG(WARNING) << "CreateAnswer: isClosing";
      return;
    }

    RTC_DCHECK(createSDO_.get() != nullptr);
    if (!createSDO_ || !createSDO_.get() || !nm_ || !nm_->getWRTC() || !nm_->getWRTC().get()) {
      close_s(false, false);
      LOG(WARNING) << "empty create_session_description_observer";
      return;
    }
    // The CreateSessionDescriptionObserver callback will be called when done.
    pci_->CreateAnswer(createSDO_.get(), nm_->getWRTC()->webrtcGamedataOpts_);
    updateDataChannelState();
  }
  // LOG(INFO) << "peer_connection created answer";
}

void WRTCSession::CreateOffer() {
  LOG(INFO) << "peer_connection->CreateOffer...";
  RTC_DCHECK_RUN_ON(signalingThread());

  {
    LOG(INFO) << std::this_thread::get_id() << ":"
              << "WRTCSession::CreateOffer peerConIMutex_";
    rtc::CritScope lock(&peerConIMutex_);

    if (isClosing()) {
      LOG(WARNING) << "CreateOffer: isClosing";
      return;
    }

    RTC_DCHECK(createSDO_.get() != nullptr);
    if (!createSDO_.get() || !nm_ || !nm_->getWRTC().get()) {
      close_s(false, false);
      LOG(WARNING) << "empty create_session_description_observer";
      return;
    }

    // TODO: Do this asynchronously via e.g. PostTaskAndReply.
    // @see
    // https://chromium.googlesource.com/experimental/chromium/src/+/refs/wip/bajones/webvr_1/content/renderer/media/rtc_peer_connection_handler.cc#1104

    // The CreateSessionDescriptionObserver callback will be called when done.
    pci_->CreateOffer(createSDO_.get(), nm_->getWRTC()->webrtcGamedataOpts_);
    updateDataChannelState();
  }
  // LOG(INFO) << "peer_connection created answer";
}

/*
 * TODO: Use Facebook’s lock-free queue && process the network messages in batch
 * server also contains a glaring inefficiency in its
 * immediate handling of data channel messages in the OnDataChannelMessage
 * callback. The cost of doing so in our example is negligible, but in an actual
 * game server, the message handler will be a more costly function that must
 * interact with state. The message handling function will then block the
 * signaling thread from processing any other messages on the wire during its
 * execution. To avoid this, I recommend pushing all messages onto a thread-safe
 * message queue, and during the next game tick, the main game loop running in a
 * different thread can process the network messages in batch.
 **/
// Callback for when the server receives a message on the data channel.
void WRTCSession::onDataChannelMessage(const webrtc::DataBuffer& buffer) {

  LOG(WARNING) << rtc::Thread::Current()->name() << ":"
               << "WRTCSession::OnDataChannelMessage";

  RTC_DCHECK_RUN_ON(signalingThread());

  LOG(WARNING) << "WRTCSession::onDataChannelMessage";
  setFullyCreated(true); // TODO

  if (isClosing()) {
    return;
  }

  lastRecievedMsgTime = boost::posix_time::second_clock::local_time();
  timerDeadline = lastRecievedMsgTime + timerDeadlinePeriod;

  if (!buffer.size()) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Invalid messageBuffer";
    return;
  }

  if (buffer.size() > MAX_IN_MSG_SIZE_BYTE) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Too big messageBuffer of size "
                 << buffer.size();
    return;
  }

  const std::string data = std::string(buffer.data.data<char>(), buffer.size());

  RTC_DCHECK(dataChannelI_.get() != nullptr);
  if (!dataChannelI_ || !dataChannelI_.get()) {
    LOG(WARNING) << "onDataChannelMessage: Invalid dataChannelI_";
    close_s(false, false);
    return;
  }

  webrtc::DataChannelInterface::DataState state = dataChannelI_->state();
  if (state != webrtc::DataChannelInterface::kOpen) {
    LOG(WARNING) << "OnDataChannelMessage: data channel not open!";
    // TODO return false;
  }
  // std::string str = "pong";
  // webrtc::DataBuffer resp(rtc::CopyOnWriteBuffer(str.c_str(), str.length()),
  // false /* binary */);

  // handleIncomingJSON(data);

  if (connectionChecker_.get() && connectionChecker_->onRemoteActivity(data)) {
    // got ping-pong messages to check connection status
    // return;
  }

  if (!onMessageCallback_) {
    LOG(WARNING) << "WRTCSession::onDataChannelMessage: Not set onMessageCallback_!";
    // close_s(false, false);
    return;
  }

  onMessageCallback_(getId(), data);

  // send back?
  // WRTCSession::sendDataViaDataChannel(nm_, shared_from_this(), buffer);
}

// Callback for when the data channel is successfully created. We need to
// re-register the updated data channel here.
// called from PCO::OnDataChannel
void WRTCSession::onDataChannelCreated(NetworkManager* nm,
                                       rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::OnDataChannelCreated";

  RTC_DCHECK_RUN_ON(signalingThread());

  RTC_DCHECK(nm != nullptr);
  if (!nm) {
    LOG(WARNING) << "onDataChannelCreated: Invalid NetworkManager";
    return;
  }

  RTC_DCHECK(channel != nullptr);
  if (!channel) {
    LOG(WARNING) << "onDataChannelCreated: Invalid DataChannelInterface";
    // close_s(false, false);
    return;
  }

  /*    kConnecting,
    kOpen,  // The DataChannel is ready to send data.
    kClosing,
    kClosed*/
  LOG(WARNING) << "onDataChannelCreated: channel->state()" << channel->state();
  RTC_DCHECK(channel->state() != webrtc::DataChannelInterface::kClosed);

  {
    // NOTE: call RegisterObserver only after dataChannelI_ assigned!
    // NOTE: dataChannelI_ exists, but reassigned
    dataChannelI_ = channel;

    LOG(INFO) << "creating dataChannelObserver_ ";
    RTC_DCHECK(dataChannelI_.get() != nullptr);
    if (!dataChannelI_.get()) {
      LOG(WARNING) << "empty dataChannelI_";
      close_s(false, false);
      return;
    }

    // TODO: RegisterObserver required by client. Tmp fix by double call RegisterObserver

    // NOTE: DCO created after PCO::OnDataChannel event
    // NOTE: calls RegisterObserver from constructor
    // dataChannelObserver_ = std::make_unique<DCO>(nm_, dataChannelI_, shared_from_this());

    RTC_DCHECK(dataChannelObserver_.get() != nullptr);
    if (!dataChannelObserver_ || !dataChannelObserver_.get()) {
      LOG(WARNING) << "empty dataChannelObserver_";
      close_s(false, false);
      return;
    }

    LOG(INFO) << "registering observer...";
    RTC_DCHECK(dataChannelObserver_.get() != nullptr);
    if (!dataChannelObserver_ || !dataChannelObserver_.get()) {
      LOG(WARNING) << "empty data_channel_observer";
      close_s(false, false);
      return;
    }

    // TODO: RegisterObserver required by client. Tmp fix by double call RegisterObserver
    // Used to receive events from the data channel. Only one observer can be
    // registered at a time. UnregisterObserver should be called before the
    // observer object is destroyed.
    dataChannelI_->RegisterObserver(dataChannelObserver_.get());

    LOG(INFO) << "registered observer";
  }

  updateDataChannelState();

  // TODO
#ifdef NOPE
  {
    /**
     * offerers periodically check the connection and may reinitialise it.
     * The answerer side will recreate the peerconnection on receiving the offer
     * delay the reinit to not call the PeerConnectivityChecker destructor in its own callback
     **/
    const auto sid = wrtcSess->getId();
    const auto nm = wrtcSess->nm_;
    wrtcSess->connectionChecker_ = std::make_unique<PeerConnectivityChecker>(
        nm, wrtcSess, wrtcSess->dataChannelI_,
        /* ConnectivityLostCallback */ [nm, /* need weak ownership */ sid]() {
          // bool needStop = true;
          LOG(WARNING) << "WrtcServer::handleAllPlayerMessages: session timer expired";
          if (nm && /*wrtcSess && wrtcSess.get() &&*/ nm->getWRTC() && nm->getWRTC().get()) {
            // needStop = true;
            nm->getWRTC()->unregisterSession(sid);
            // close called from unregisterSession
            // nm->getWRTC()->CloseDataChannel(nm, wrtcSess->dataChannelI_, pci_);
          }
          return true; // stop periodic checks
        });
  }
#endif
}

// TODO: on closed

// Callback for when the STUN server responds with the ICE candidates.
// Sends by websocket JSON containing { candidate, sdpMid, sdpMLineIndex }
// TODO: WORKS WITHOUT OnIceCandidate???
void WRTCSession::onIceCandidate(NetworkManager* nm, const std::string& wsConnId,
                                 const webrtc::IceCandidateInterface* candidate) {
  if (!nm) {
    LOG(WARNING) << "onIceCandidate: Invalid NetworkManager";
    return;
  }

  if (!candidate) {
    LOG(WARNING) << "onIceCandidate: Invalid IceCandidateInterface";
    // close_s(false, false);
    return;
  }

  const std::string sdp_mid_copy = candidate->sdp_mid();
  auto wsSess = nm->getWS()->getSessById(wsConnId);
  if (!wsSess || !wsSess.get()) {
    LOG(WARNING) << "onIceCandidate: Invalid getSessById for " << wsConnId;
    return;
  }

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::OnIceCandidate";
  std::string candidate_str;
  if (!candidate->ToString(&candidate_str)) {
    LOG(WARNING) << "Failed to serialize candidate";
    // LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }

  // LOG(FATAL) << "onIceCandidate" << sdp_mid_copy << " " << candidate_str;

  rapidjson::Document message_object;
  message_object.SetObject();
  message_object.AddMember(
      "type", rapidjson::StringRef(algo::Opcodes::opcodeToStr(algo::WS_OPCODE::CANDIDATE)),
      message_object.GetAllocator());
  rapidjson::Value candidate_value;
  candidate_value.SetString(rapidjson::StringRef(candidate_str.c_str()));
  rapidjson::Value sdp_mid_value;
  sdp_mid_value.SetString(rapidjson::StringRef(sdp_mid_copy.c_str()));
  rapidjson::Value message_payload;
  message_payload.SetObject();
  // candidate: Host candidate for RTP on UDP - in this ICE line our browser is
  // giving its host candidates- the IP example "candidate":
  // "candidate:1791031112 1 udp 2122260223 10.10.15.25 57339 typ host
  // generation 0 ufrag say/ network-id 1 network-cost 10",
  message_payload.AddMember(kCandidateSdpName, candidate_value, message_object.GetAllocator());
  // sdpMid: audio or video or ...
  message_payload.AddMember(kCandidateSdpMidName, sdp_mid_value, message_object.GetAllocator());
  message_payload.AddMember(kCandidateSdpMlineIndexName, candidate->sdp_mline_index(),
                            message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  bool done = message_object.Accept(writer);
  if (!done) {
    LOG(WARNING) << "OnIceCandidate: INVALID JSON!";
    return;
  }
  std::string payload = strbuf.GetString();

  RTC_DCHECK(wsSess.get() != nullptr);
  RTC_DCHECK(wsSess->isOpen() == true);
  if (wsSess && wsSess.get() && wsSess->isOpen())
    wsSess->send(payload); // TODO: use Task queue
  /*else {
    close_s(false, false);
  }*/
}

bool WRTCSession::IsStable() {

  if (pci_.get()) {
    webrtc::PeerConnectionInterface::SignalingState state(pci_->signaling_state());

    if (state == webrtc::PeerConnectionInterface::kStable) {
      return true;
    }
  }

  return false;
}

// Callback for when the answer is created. This sends the answer back to the
// client.
void WRTCSession::onAnswerCreated(webrtc::SessionDescriptionInterface* sdi) {
  RTC_DCHECK_RUN_ON(signalingThread());

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::OnAnswerCreated";
  if (sdi == nullptr) {
    LOG(WARNING) << "WRTCSession::OnAnswerCreated INVALID SDI";
    return;
  }
  // TODO: replace rapidjson to ...?
  std::string offer_string;
  if (!sdi->ToString(&offer_string)) {
    LOG(WARNING) << "Failed to serialize sdi";
    // LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }

  auto wsSess = nm_->getWS()->getSessById(wsId_);

  RTC_DCHECK(wsSess.get() != nullptr); // TODO: REMOVE <<<<<<<<
  if (!wsSess || !wsSess.get()) {
    LOG(WARNING) << "onAnswerCreated: Invalid getSessById for " << wsId_;
    close_s(false, false); // NOTE: both ws and wrtc must exist at the same time
    return;
  }

  LOG(INFO) << "OnAnswerCreated";
  // store the server’s own answer
  setLocalDescription(sdi);
  // TODO: replace rapidjson to ...?
  rapidjson::Document message_object;
  message_object.SetObject();
  rapidjson::Value type;
  message_object.AddMember(
      "type", rapidjson::StringRef(algo::Opcodes::opcodeToStr(algo::WS_OPCODE::ANSWER)),
      message_object.GetAllocator());
  rapidjson::Value sdp_value;
  sdp_value.SetString(rapidjson::StringRef(offer_string.c_str()));

  rapidjson::Value message_payload;
  message_payload.SetObject();
  message_payload.AddMember(
      "type",
      kAnswerSdpName, // rapidjson::StringRef(ANSWER_OPERATION.operationCodeStr_.c_str()),
      message_object.GetAllocator());
  message_payload.AddMember("sdp", sdp_value, message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());

  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  bool done = message_object.Accept(writer);
  if (!done) {
    LOG(WARNING) << "OnAnswerCreated: INVALID JSON!";
    return;
  }
  std::string payload = strbuf.GetString();

  RTC_DCHECK(wsSess.get() != nullptr);
  RTC_DCHECK(wsSess->isOpen() == true);
  if (wsSess && wsSess.get() && wsSess->isOpen())
    wsSess->send(payload); // TODO: use Task queue
}

// Callback for when the offer is created. This sends the answer back to the
// client.
void WRTCSession::onOfferCreated(webrtc::SessionDescriptionInterface* sdi) {
  RTC_DCHECK_RUN_ON(signalingThread());

  LOG(INFO) << std::this_thread::get_id() << ":"
            << "WRTCSession::onOfferCreated";
  if (sdi == nullptr) {
    LOG(WARNING) << "WRTCSession::onOfferCreated INVALID SDI";
    return;
  }
  std::string offer_string;
  sdi->ToString(&offer_string);

  LOG(WARNING) << std::this_thread::get_id() << ":"
               << "WRTCSession::onOfferCreated " << offer_string;

  auto wsSess = nm_->getWS()->getSessById(wsId_);

  RTC_DCHECK(wsSess.get() != nullptr); ///// <<< REMOVE
  if (!wsSess || !wsSess.get()) {
    LOG(WARNING) << "onOfferCreated: Invalid getSessById for " << wsId_;
    close_s(false, false); // NOTE: both ws and wrtc must exist at the same time
    return;
  }

  updateDataChannelState();

  // RTC_DCHECK(createdWRTCSession->isDataChannelOpen() == true);

  // TODO:
  // github.com/YOU-i-Labs/webkit/blob/master/Source/WebCore/Modules/mediastream/libwebrtc/LibWebRTCMediaEndpoint.cpp#L182
  /*webrtc::SessionDescriptionInterface* clientSessionDescription =
      createdWRTCSession->createSessionDescription(kOffer, sdp);
  if (!clientSessionDescription) {
    LOG(WARNING) << "empty clientSessionDescription!";
    return;
  }

  // TODO: CreateSessionDescriptionMsg : public rtc::MessageData
  // https://github.com/modulesio/webrtc/blob/master/pc/webrtcsessiondescriptionfactory.cc#L68

  createdWRTCSession->SetRemoteDescription(clientSessionDescription);*/

  LOG(INFO) << "onOfferCreated";
  // store the server’s own offer
  setLocalDescription(sdi);

  // wait p.iceGatheringState != 'complete'
  // TODO
  // std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // TODO

  // setRemoteDescription(sdi); // <<<<<<<<<<<<<<<<<<<<
  // TODO: replace rapidjson to ...?
  rapidjson::Document message_object;
  message_object.SetObject();
  rapidjson::Value type;
  message_object.AddMember("type",
                           rapidjson::StringRef(algo::Opcodes::opcodeToStr(algo::WS_OPCODE::OFFER)),
                           message_object.GetAllocator());
  rapidjson::Value sdp_value;
  sdp_value.SetString(rapidjson::StringRef(offer_string.c_str()));

  // RTC_DCHECK(algo::Opcodes::opcodeToStr(algo::WS_OPCODE::OFFER) == "2");

  rapidjson::Value message_payload;
  message_payload.SetObject();
  message_payload.AddMember(
      "type",
      kOfferSdpName, // rapidjson::StringRef(ANSWER_OPERATION.operationCodeStr_.c_str()),
      message_object.GetAllocator());
  message_payload.AddMember("sdp", sdp_value, message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());

  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  bool done = message_object.Accept(writer);
  if (!done) {
    LOG(WARNING) << "OnOfferCreated: INVALID JSON!";
    return;
  }
  std::string payload = strbuf.GetString();

  // description contains information about media capabilities
  // (for example, if it has a webcam or can play audio).
  // rtcPeerConnection.setLocalDescription(description);
  // webSocketConnection.send(JSON.stringify({type: WS_OFFER_OPCODE, payload: description}));

  RTC_DCHECK(wsSess.get() != nullptr && wsSess->isOpen() == true);
  if (wsSess && wsSess.get() && wsSess->isOpen())
    wsSess->send(payload); // TODO: use Task queue
}

void WRTCSession::onDataChannelAllocated() {
  RTC_DCHECK_RUN_ON(signalingThread());

  LOG(INFO) << "WRTCSession::onDataChannelConnecting";
  nm_->getWRTC()->addGlobalDataChannelCount_s(1);
  addDataChannelCount_s(1);
}

void WRTCSession::onDataChannelDeallocated() {
  RTC_DCHECK_RUN_ON(signalingThread());

  LOG(INFO) << "WRTCSession::onDataChannelClose";
  nm_->getWRTC()->subGlobalDataChannelCount_s(1);
  subDataChannelCount_s(1);
}

rtc::Thread* WRTCSession::networkThread() const { return nm_->getWRTC()->networkThread(); }
rtc::Thread* WRTCSession::workerThread() const { return nm_->getWRTC()->workerThread(); }
rtc::Thread* WRTCSession::signalingThread() const { return nm_->getWRTC()->signalingThread(); }

// TODO
// github.com/shenghan97/vegee/blob/master/Server/webrtc-streamer/src/PeerConnectionManager.cpp#L531
bool WRTCSession::streamStillUsed(const std::string& streamLabel) {
  RTC_DCHECK_RUN_ON(signalingThread());

  bool stillUsed = false;

  // Accessor method to active local streams.
  // This method is not supported with kUnifiedPlan semantics. Please use
  // GetSenders() instead.
  rtc::scoped_refptr<webrtc::StreamCollectionInterface> localstreams;
  {
    rtc::CritScope lock(&peerConIMutex_);

    if (isClosing()) {
      LOG(WARNING) << "streamStillUsed: isClosing";
      return false;
    }

    localstreams = pci_->local_streams();
  }
  if (!localstreams.get()) {
    LOG(WARNING) << "empty localstreams";
    return false;
  }
  for (unsigned int i = 0; i < localstreams->count(); i++) {
    if (localstreams->at(i)->id() == streamLabel) {
      stillUsed = true;
      break;
    }
  }

  return stillUsed;
}

void WRTCSession::addDataChannelCount_s(uint32_t count) {
  RTC_DCHECK_RUN_ON(signalingThread());

  // LOG(INFO) << "WRTCServer::addDataChannelCount_s";
  dataChannelCount_ += count;
  // TODO: check overflow
  LOG(INFO) << "WRTCServer::addDataChannelCount_s: data channel count: " << dataChannelCount_;

  // expected: 1 session == 1 dataChannel
  // NOTE: reconnect == new session
  RTC_DCHECK_LE(dataChannelCount_, std::numeric_limits<uint32_t>::max());
  RTC_DCHECK_LE(dataChannelCount_, 1);
}

void WRTCSession::subDataChannelCount_s(uint32_t count) {
  RTC_DCHECK_RUN_ON(signalingThread());

  // LOG(INFO) << "WRTCServer::subDataChannelCount_s";
  RTC_DCHECK_GT(dataChannelCount_, 0);
  dataChannelCount_ -= count;
  // TODO: check overflow
  LOG(INFO) << "WRTCServer::subDataChannelCount_s: data channel count: " << dataChannelCount_;

  // expected: 1 session == 1 dataChannel
  // NOTE: reconnect == new session
  RTC_DCHECK_LE(dataChannelCount_, 1);
}

} // namespace wrtc
} // namespace net
} // namespace gloer
