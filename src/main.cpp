// This a minimal fully functional example for setting up a server written in C++ that communicates
// with clients via WebRTC data channels. This uses WebSockets to perform the WebRTC handshake
// (offer/accept SDP) with the client. We only use WebSockets for the initial handshake because TCP
// often presents too much latency in the context of real-time action games. WebRTC data channels,
// on the other hand, allow for unreliable and unordered message sending via SCTP.

#include "observers.h"
// TODO: refactor global variables

NetworkManager NWM;

// The thread entry point for the WebSockets thread.
void WebSocketThreadEntry() {
  // Run the WebSocket server as a separate thread so your main
  // process can handle the game loop.
  NWM.wsServer->InitAndRun();
}

// The thread entry point for the WebRTC thread. This sets the WebRTC thread as the signaling thread
// and creates a worker thread in the background.
void WebRtcSignalThreadEntry() {
  // ICE is the protocol chosen for NAT traversal in WebRTC.
  webrtc::PeerConnectionInterface::IceServer ice_servers[5];
  // TODO >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  ice_servers[0].uri = "stun:stun.l.google.com:19302";
  ice_servers[1].uri = "stun:stun1.l.google.com:19302";
  ice_servers[2].uri = "stun:stun2.l.google.com:19305";
  ice_servers[3].uri = "stun:stun01.sipphone.com";
  ice_servers[4].uri = "stun:stunserver.org";
  // TODO ice_server.username = "xxx";
  // TODO ice_server.password = kTurnPassword;
  // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  NWM.wrtcServer->resetWebRtcConfig({
    ice_servers[0],
    ice_servers[1],
    ice_servers[2],
    ice_servers[3],
    ice_servers[4]
  });
  NWM.wrtcServer->InitAndRun();
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
    // Runs WebSocket listening loop
    //NWM.wsServer->websockets_thread = std::thread(WebSocketThreadEntry);
    NWM.wsServer->websockets_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&WebSocketServer::run, &NWM.wsServer->ws_server);
    NWM.wsServer->websockets_thread->join();
    //WebSocketThreadEntry(); // TODO: thread
    // Runs WebRTC listening loop
    //NWM.wrtcServer->webrtc_thread = std::thread(WebRtcSignalThreadEntry);
    WebRtcSignalThreadEntry(); // uses RTC_RUN_ON "thread_checker_" thread
  }

  bool shouldQuit = false;
  std::cout << "!shouldQuit" << std::endl;
  while(!shouldQuit){
   // TODO
  }

  NWM.wsServer->Quit();
  NWM.wrtcServer->Quit();
}