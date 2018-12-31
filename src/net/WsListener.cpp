#include "net/WsListener.hpp"
#include "net/SessionManager.hpp"
#include "net/WsSession.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

namespace utils {
namespace net {

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
  std::cerr << "on_WsListener_fail: " << what << ": " << ec.message() << "\n";
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
  std::cout << "WS run\n";
  if (!isAccepting())
    return;
  do_accept();
}

void WsListener::do_accept() {
  std::cout << "WS do_accept\n";
  acceptor_.async_accept(socket_,
                         std::bind(&WsListener::on_accept, shared_from_this(),
                                   std::placeholders::_1));
}

/**
 * @brief handles new connections and starts sessions
 */
void WsListener::on_accept(beast::error_code ec) {
  std::cout << "WS on_accept\n";
  if (ec) {
    on_WsListener_fail(ec, "accept");
  } else {
    // Create the session and run it
    auto newWsSession = std::make_shared<utils::net::WsSession>(
        std::move(socket_), sm_, nextSessionId());
    sm_->registerSession(newWsSession);
    newWsSession->run();
    const std::string wsGuid =
        boost::lexical_cast<std::string>(newWsSession->getId());
    std::string welcomeMsg = "welcome, ";
    welcomeMsg += wsGuid;
    std::cout << "new ws session " << wsGuid << "\n";
    // newWsSession->send(std::make_shared<std::string>(welcomeMsg));
    ///////newWsSession->send(welcomeMsg);
  }

  // Accept another connection
  do_accept();
}

} // namespace net
} // namespace utils
