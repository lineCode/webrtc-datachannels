#include "net/websockets/WsListener.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/websockets/WsSession.hpp"
#include "net/websockets/WsSessionManager.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace utils {
namespace net {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

static boost::uuids::uuid genGuid() {
  // return sm->maxSessionId() + 1; // TODO: collision
  boost::uuids::random_generator generator;
  return generator();
}

static std::string nextSessionId() {
  return boost::lexical_cast<std::string>(genGuid());
}

// Report a failure
void WsListener::on_WsListener_fail(beast::error_code ec, char const* what) {
  LOG(WARNING) << "on_WsListener_fail: " << what << ": " << ec.message();
}

void WsListener::configureAcceptor() {
  beast::error_code ec;

  // Open the acceptor
  acceptor_.open(endpoint_.protocol(), ec);
  if (ec) {
    on_WsListener_fail(ec, "open");
    return;
  }

  // Allow address reuse
  acceptor_.set_option(net::socket_base::reuse_address(true), ec);
  if (ec) {
    on_WsListener_fail(ec, "set_option");
    return;
  }

  // Bind to the server address
  acceptor_.bind(endpoint_, ec);
  if (ec) {
    on_WsListener_fail(ec, "bind");
    return;
  }

  // Start listening for connections
  acceptor_.listen(net::socket_base::max_listen_connections, ec);
  if (ec) {
    on_WsListener_fail(ec, "listen");
    return;
  }
}

// Start accepting incoming connections
void WsListener::run() {
  LOG(INFO) << "WS run";
  if (!isAccepting())
    return;
  do_accept();
}

void WsListener::do_accept() {
  LOG(INFO) << "WS do_accept";
  acceptor_.async_accept(socket_,
                         std::bind(&WsListener::on_accept, shared_from_this(),
                                   std::placeholders::_1));
}

/**
 * @brief handles new connections and starts sessions
 */
void WsListener::on_accept(beast::error_code ec) {
  LOG(INFO) << "WS on_accept";
  if (ec) {
    on_WsListener_fail(ec, "accept");
  } else {
    // Create the session and run it
    auto newWsSession = std::make_shared<utils::net::WsSession>(
        std::move(socket_), nm_, nextSessionId());
    nm_->getWsSessionManager()->registerSession(newWsSession);
    newWsSession->run();
    const std::string wsGuid =
        boost::lexical_cast<std::string>(newWsSession->getId());
    std::string welcomeMsg = "welcome, ";
    welcomeMsg += wsGuid;
    LOG(INFO) << "new ws session " << wsGuid;
    // newWsSession->send(std::make_shared<std::string>(welcomeMsg));
    ///////newWsSession->send(welcomeMsg);
  }

  // Accept another connection
  do_accept();
}

} // namespace net
} // namespace utils
