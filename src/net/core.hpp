#pragma once

#include <boost/asio.hpp>  // IWYU pragma: export
#include <boost/beast.hpp> // IWYU pragma: export

namespace beast = boost::beast;               // from <boost/beast.hpp>
namespace http = beast::http;                 // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;       // from <boost/beast/websocket.hpp>
namespace net = boost::asio;                  // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;             // from <boost/asio/ip/tcp.hpp>
using error_code = boost::system::error_code; // from <boost/system/error_code.hpp>
