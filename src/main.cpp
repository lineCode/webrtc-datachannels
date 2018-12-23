// This a minimal fully functional example for setting up a server written in C++ that communicates
// with clients via WebRTC data channels. This uses WebSockets to perform the WebRTC handshake
// (offer/accept SDP) with the client. We only use WebSockets for the initial handshake because TCP
// often presents too much latency in the context of real-time action games. WebRTC data channels,
// on the other hand, allow for unreliable and unordered message sending via SCTP.

#include "observers.h"

// TODO: refactor global variables

static NetworkManager NWM;

// The thread entry point for the WebSockets thread.
void WebSocketThreadEntry() {
  // Run the WebSocket server as a separate thread so your main
  // process can handle the game loop.
  NWM.wsServer.InitAndRun();
}

// The thread entry point for the WebRTC thread. This sets the WebRTC thread as the signaling thread
// and creates a worker thread in the background.
void WebRtcSignalThreadEntry() {
  // ICE is the protocol chosen for NAT traversal in WebRTC.
  webrtc::PeerConnectionInterface::IceServer ice_server;
  // TODO >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  ice_server.uri = "stun:stun.l.google.com:19302";
  // TODO ice_server.username = "xxx";
  // TODO ice_server.password = kTurnPassword;
  // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  NWM.wrtcServer.resetWebRtcConfig({
    ice_server
  });
  NWM.wrtcServer.InitAndRun();
}

// Main entry point of the code.
int main() {
  std::cout << std::this_thread::get_id() << ":"
            << "Main thread" << std::endl;
  std::cout << "Initialized SSL" << std::endl;

  //webrtc_thread = std::thread(create_factory_then_run);

  /*
   * Nothing is stopping a server from being one of the WebRTC peers.
   * WebSockets makes things easier, because the server is always reachable,
   * which means STUN and TURN are not really necessary.
  **/
  // TODO: mutexes? Message Queue?
  {
    // Runs WebRTC listening loop
    NWM.wrtcServer.webrtc_thread = std::thread(WebRtcSignalThreadEntry);
    // Runs WebSocket listening loop
    NWM.wsServer.websockets_thread = std::thread(WebSocketThreadEntry);
  }

  while(true){
   // TODO
  }

  NWM.wsServer.Quit();
  NWM.wrtcServer.Quit();
}
