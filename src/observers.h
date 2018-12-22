//#ifdef NOPE

// No-op implementations of most webrtc::*Observer methods. For the ones we do care about in the
// example, we supply a callback in the constructor.
//
// Author: brian@brkho.com

#ifndef WEBRTC_EXAMPLE_SERVER_OBSERVERS_H
#define WEBRTC_EXAMPLE_SERVER_OBSERVERS_H

#include <webrtc/api/peerconnectioninterface.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class WebRtcObserver {
  // TODO
};

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

// PeerConnection events.
// see https://github.com/sourcey/libsourcey/blob/ce311ff22ca02c8a83df7162a70f6aa4f760a761/doc/api-webrtc.md
class PCO : public webrtc::PeerConnectionObserver {
  public:
    // Constructor taking a few callbacks.
    PCO(std::function<void(webrtc::DataChannelInterface*)> on_data_channel,
        std::function<void(const webrtc::IceCandidateInterface*)> on_ice_candidate) :
        on_data_channel{on_data_channel}, on_ice_candidate{on_ice_candidate} {}

    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::SignalingChange(" << new_state << ")" << std::endl;
    }

    // Triggered when media is received on a new stream from remote peer.
    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface>/* stream*/) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::AddStream" << std::endl;
    }

    // Triggered when a remote peer close a stream.
    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface>/* stream*/) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::RemoveStream" << std::endl;
    }

    /*void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::DataChannel(" << data_channel
                << ", " << parent.data_channel.get() << ")" << std::endl;
      // Answer送信側は、onDataChannelでDataChannelの接続を受け付ける
      parent.data_channel = data_channel;
      parent.data_channel->RegisterObserver(&parent.dco);
    };*/

    // Triggered when renegotiation is needed. For example, an ICE restart
    // has begun.
    void OnRenegotiationNeeded() override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::RenegotiationNeeded" << std::endl;
    }

    // Called any time the IceConnectionState changes.
    //
    // Note that our ICE states lag behind the standard slightly. The most
    // notable differences include the fact that "failed" occurs after 15
    // seconds, not 30, and this actually represents a combination ICE + DTLS
    // state, so it may be "failed" if DTLS fails while ICE succeeds.
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::IceConnectionChange(" << static_cast<int>(new_state) << ")" << std::endl;
      switch (new_state)
      {
        case webrtc::PeerConnectionInterface::kIceConnectionNew:
          break;
        case webrtc::PeerConnectionInterface::kIceConnectionChecking:
          break;
        case webrtc::PeerConnectionInterface::kIceConnectionConnected:
          break;
        case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
          break;
        case webrtc::PeerConnectionInterface::kIceConnectionFailed:
          break;
        case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
          break;
        case webrtc::PeerConnectionInterface::kIceConnectionClosed:
          break;
        case webrtc::PeerConnectionInterface::kIceConnectionMax:
          // TODO
          /* not in https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/iceConnectionState */
          break;
      }
    }

    // Called any time the IceGatheringState changes.
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::IceGatheringChange(" << new_state << ")" << std::endl;
    }

    /*void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override {
      std::cout << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::IceCandidate" << std::endl;
      parent.onIceCandidate(candidate);
    };*/

    // Triggered when a remote peer opens a data channel.
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
      on_data_channel(channel);
    }

    // Override ICE candidate.
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
      on_ice_candidate(candidate);
    }

    // TODO OnInterestingUsage

  private:
    std::function<void(rtc::scoped_refptr<webrtc::DataChannelInterface>)> on_data_channel;
    std::function<void(const webrtc::IceCandidateInterface*)> on_ice_candidate;
};

// DataChannel events.
class DCO : public webrtc::DataChannelObserver {
  public:
    // Constructor taking a callback.
    DCO(std::function<void(const webrtc::DataBuffer&)> on_message) :
        on_message{on_message} {}

    // Change in state of the Data Channel.
    void OnStateChange() {
      /*
      // TODO!!!
      if (data_channel_) {
        webrtc::DataChannelInterface::DataState state = data_channel_->state();
        if (state != webrtc::DataChannelInterface::kOpen) {
          std::cout << "OnStateChange: data channel not open!" << std::endl;
          // TODO return false;  
        } else {
          on_channel_open();
        }
      }*/
    }
    
    // Message received.
    void OnMessage(const webrtc::DataBuffer& buffer) {
      on_message(buffer);
    }

    // Buffered amount change.
    void OnBufferedAmountChange(uint64_t /* previous_amount */) {}

  private:
    std::function<void(const webrtc::DataBuffer&)> on_message;
};

// Create SessionDescription events.
class CSDO : public webrtc::CreateSessionDescriptionObserver {
  public:
    // Constructor taking a callback.
    CSDO(std::function<void(webrtc::SessionDescriptionInterface*)>
        OnAnswerCreated) : OnAnswerCreated{OnAnswerCreated} {}
  
    // Successfully created a session description.
    /*void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
      OnAnswerCreated(desc);
    }*/

    // Failure to create a session description.
    //void OnFailure(const std::string& /* error */) {}

    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
      /*std::cout << std::this_thread::get_id() << ":"
                << "CreateSessionDescriptionObserver::OnSuccess" << std::endl;
      parent.onSuccessCSD(desc);*/
      OnAnswerCreated(desc);
    }

    void OnFailure(const std::string& error) override {
      std::cout << std::this_thread::get_id() << ":"
                << "CreateSessionDescriptionObserver::OnFailure" << std::endl << error << std::endl;
    }

    // SEE https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h#L102
    // Unimplemented virtual function.
    void AddRef() const override { return; }

    // Unimplemented virtual function.
    rtc::RefCountReleaseStatus Release() const override { return rtc::RefCountReleaseStatus::kDroppedLastRef; }

  private:
    std::function<void(webrtc::SessionDescriptionInterface*)> OnAnswerCreated;
};

// Set SessionDescription events.
class SSDO : public webrtc::SetSessionDescriptionObserver {
  public:
    // Default constructor.
    SSDO() {}

    // Successfully set a session description.
    void OnSuccess() {}

    // Failure to set a sesion description.
    void OnFailure(const std::string& /* error */) {}

    // SEE https://github.com/sourcey/libsourcey/blob/master/src/webrtc/include/scy/webrtc/peer.h#L102
    // Unimplemented virtual function.
    void AddRef() const override { return; }

    // Unimplemented virtual function.
    rtc::RefCountReleaseStatus Release() const override { return rtc::RefCountReleaseStatus::kDroppedLastRef; }
};

#endif  // WEBRTC_EXAMPLE_SERVER_OBSERVERS_H

//#endif 