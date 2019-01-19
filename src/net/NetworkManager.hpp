#pragma once

#include "net/NetworkManager.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <vector>

namespace utils {
namespace config {
class ServerConfig;
} // namespace config
} // namespace utils

namespace utils {
namespace net {

class WsSession;
class WsSessionManager;
class WRTCServer;

struct NetworkOperation {
  NetworkOperation(uint32_t operationCode, const std::string& operationName)
      : operationCode_(operationCode), operationCodeStr_(std::to_string(operationCode)),
        operationName_(operationName) {}

  NetworkOperation(uint32_t operationCode)
      : operationCode_(operationCode), operationCodeStr_(std::to_string(operationCode)),
        operationName_("") {}

  const uint32_t operationCode_;
  const std::string operationCodeStr_;
  /**
   * operationName usefull for logging
   * NOTE: operationName may be empty
   **/
  const std::string operationName_;

  bool operator<(const NetworkOperation& rhs) const { return operationCode_ < rhs.operationCode_; }
};

// TODO: move to header
const NetworkOperation PING_OPERATION = NetworkOperation(0, "PING");
const NetworkOperation CANDIDATE_OPERATION = NetworkOperation(1, "CANDIDATE");
const NetworkOperation OFFER_OPERATION = NetworkOperation(2, "OFFER");
const NetworkOperation ANSWER_OPERATION = NetworkOperation(3, "ANSWER");

typedef std::function<void(utils::net::WsSession* clientSession,
                           std::shared_ptr<boost::beast::multi_buffer> messageBuffer)>
    WsNetworkOperationCallback;

/*typedef std::function<void(
    utils::net::WRTCSession* clientSession,
    std::string_view messageBuffer)>
    WrtcNetworkOperationCallback;*/

template <typename opType, typename cbType> class CallbackManager {
public:
  CallbackManager() {}

  virtual ~CallbackManager(){};

  virtual std::map<opType, cbType> getCallbacks() const = 0;

  virtual void addCallback(const opType& op, const cbType& cb) = 0;

protected:
  std::map<opType, cbType> operationCallbacks_;
};

class WSInputCallbacks : public CallbackManager<NetworkOperation, WsNetworkOperationCallback> {
public:
  WSInputCallbacks();

  ~WSInputCallbacks();

  std::map<NetworkOperation, WsNetworkOperationCallback> getCallbacks() const override;

  void addCallback(const NetworkOperation& op, const WsNetworkOperationCallback& cb) override;
};
/*
class PlayerSession {
  PlayerSession() {}

  std::shared_ptr<utils::net::WsSession> wsSess_;

  std::shared_ptr<utils::net::WrtcSession> wrtcSess_;
};

class Player {
  Player() {}
  PlayerSession session;
};*/

class NetworkManager {
public:
  NetworkManager(const utils::config::ServerConfig& serverConfig);

  std::shared_ptr<utils::net::WsSessionManager> getWsSessionManager() const;

  WSInputCallbacks getWsOperationCallbacks() const;

  void handleAllPlayerMessages();

  void runWsThreads(const utils::config::ServerConfig& serverConfig);

  void runWrtcThreads(const utils::config::ServerConfig& serverConfig);

  void finishWsThreads();

  void runIocWsListener(const utils::config::ServerConfig& serverConfig);

  void webRtcSignalThreadEntry(
      /*const utils::config::ServerConfig& serverConfig*/); // TODO

  std::shared_ptr<utils::net::WRTCServer> getWRTC() const { return wrtcServer_; }

private:
  WSInputCallbacks wsOperationCallbacks_;

  std::shared_ptr<utils::net::WsSessionManager>
      wsSM_; // TODO: rename to wsServer_ && WsSessionManager -> WSServer

  std::shared_ptr<utils::net::WRTCServer> wrtcServer_;

  // TODO
  std::thread webrtcThread_;

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> wsThreads_;

  // TODO
  //  std::vector<Player> players_;

  // The io_context is required for all I/O
  boost::asio::io_context ioc_;
};

} // namespace net
} // namespace utils
