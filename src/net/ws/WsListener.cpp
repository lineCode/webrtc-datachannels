#include "net/ws/WsListener.hpp" // IWYU pragma: associated
#include "algo/StringUtils.hpp"
#include "log/Logger.hpp"
#include "net/NetworkManager.hpp"
#include "net/ws/WsServer.hpp"
#include "net/ws/WsSession.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace gloer {
namespace net {
namespace ws {

namespace {

// TODO: prevent collision? respond ERROR to client if collided?
static std::string nextWsSessionId() { return gloer::algo::genGuid(); }

} // namespace

WsListener::WsListener(boost::asio::io_context& ioc, const boost::asio::ip::tcp::endpoint& endpoint,
                       std::shared_ptr<std::string const> doc_root, NetworkManager* nm)
    : acceptor_(ioc), socket_(ioc), doc_root_(doc_root), nm_(nm), endpoint_(endpoint) {
  configureAcceptor();
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
  acceptor_.set_option(::net::socket_base::reuse_address(true), ec);
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
  acceptor_.listen(::net::socket_base::max_listen_connections, ec);
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
  acceptor_.async_accept(
      socket_, std::bind(&WsListener::on_accept, shared_from_this(), std::placeholders::_1));
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
    const auto newSessId = nextWsSessionId();
    auto newWsSession = std::make_shared<WsSession>(std::move(socket_), nm_, newSessId);
    nm_->getWS()->addSession(newSessId, newWsSession);
    newWsSession->run();
    std::string welcomeMsg = "welcome, ";
    welcomeMsg += newSessId;
    LOG(INFO) << "new ws session " << newSessId;
    // newWsSession->send(std::make_shared<std::string>(welcomeMsg));
    ///////newWsSession->send(welcomeMsg);
  }

  // Accept another connection
  do_accept();
}

} // namespace ws
} // namespace net
} // namespace gloer
