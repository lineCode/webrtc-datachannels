/**
 * This a minimal fully functional example for setting up a client written in JavaScript that
 * communicates with a server via WebRTC data channels. This uses WebSockets to perform the WebRTC
 * handshake (offer/accept SDP) with the server. We only use WebSockets for the initial handshake
 * because TCP often presents too much latency in the context of real-time action games. WebRTC
 * data channels, on the other hand, allow for unreliable and unordered message sending via SCTP
 */

// URL to the server with the port we are using for WebSockets.
const webSocketUrl = 'ws://localhost:8080';
// The WebSocket object used to manage a connection.
let webSocketConnection = null;
// The RTCPeerConnection through which we engage in the SDP handshake.
let rtcPeerConnection = null;
// The data channel used to communicate.
let dataChannel = null;

const pingTimes = {};
const pingLatency = {};
let pingCount = 0;
const PINGS_PER_SECOND = 20;
const SECONDS_TO_PING = 20;
let pingInterval;
let startTime;

const PROTOCOL_VERSION = "0"; // TODO: use it

const WS_PING_OPCODE = "0";
const WS_CANDIDATE_OPCODE = "1";
const WS_OFFER_OPCODE = "2";
const WS_ANSWER_OPCODE = "3";

const WRTC_PING_OPCODE = "0";
const WRTC_SERVER_TIME_OPCODE = "1";

// Callback for when we receive a message on the data channel.
function onDataChannelMessage(event) {
  console.log("onDataChannelMessage")
  let messageObject = "";
  try {
      messageObject = JSON.parse(event.data);
  } catch(e) {
      messageObject = event.data;
  }
  console.log("onDataChannelMessage type =", messageObject.type, ";event.data= ", event.data)
  if (messageObject.type === WRTC_PING_OPCODE) {
    const key = messageObject.payload;
    pingLatency[key] = performance.now() - pingTimes[key];
  } else if (messageObject.type === WRTC_SERVER_TIME_OPCODE) {
    console.log("WRTC_SERVER_TIME_OPCODE type =", messageObject.type, ";event.data= ", event.data)
  } else {
    console.log('Unrecognized WEBRTC message type.', messageObject);
  }
}

// Callback for when the data channel was successfully opened.
function onDataChannelOpen() {
  console.log('Data channel opened!');
}

/* 
 * Callback for when the STUN server responds with the ICE candidates.
 * when we get information on how to connect to this browser (through STUN), we send it to the server
**/
function onIceCandidate(event) {
  if (event && event.candidate) {
    console.log("onIceCandidate ", event.candidate)
    webSocketConnection.send(JSON.stringify({type: WS_CANDIDATE_OPCODE, payload: event.candidate}));
  }
}

/*
 * Callback for when the SDP offer was successfully created.
 * Used to create an offer to connect and send it to the server
 * Here the local description is set
 * The server should reply with the answer message 
**/
function onOfferCreated(description) {
  console.log("onOfferCreated ", description)
  // description contains information about media capabilities
  // (for example, if it has a webcam or can play audio).
  rtcPeerConnection.setLocalDescription(description);
  webSocketConnection.send(JSON.stringify({type: WS_OFFER_OPCODE, payload: description}));
}

function sleep(delay) {
    var start = new Date().getTime();
    while (new Date().getTime() < start + delay);
}

/*
 * Callback for when the WebSocket is successfully opened.
 * Then we send a socket.io message to the server,
 * so that it knows to prepare a peerconnection for us to connect to.
**/
function onWebSocketOpen() {
  console.log("onWebSocketOpen ")
  connectWRTC()
}

function connectWRTC() {
  console.log("connectWRTC ")
  //sleep(1000);
  // @note: #Using five or more STUN/TURN servers causes problems
  // TODO >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  const config = {
    iceServers: [{
      url: 'stun:stun.l.google.com:19302'
    }, {
      url: 'stun:stun1.l.google.com:19302'
    }, {
      url: 'stun:stun2.l.google.com:19305'
    }, {
      url: 'stun:stun01.sipphone.com'
    }, {
      url: 'stun:stunserver.org'
    }]
  };
  console.log("opening rtcPeerConnection...")
  // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  //setTimeout(myFunction, 3000);
  rtcPeerConnection = new RTCPeerConnection(config);
  const dataChannelConfig = {
    // SCTP supports unordered data. Unordered data is unimportant for multiplayer games.
    ordered: false,
    // maxRetransmits is 0, because if a message didn’t arrive, we don’t care.
    maxRetransmits: 0
  };
  dataChannel = rtcPeerConnection.createDataChannel('dc', dataChannelConfig);
  dataChannel.onmessage = onDataChannelMessage;
  dataChannel.onopen = onDataChannelOpen;
  const sdpConstraints = {
    mandatory: {
      OfferToReceiveAudio: false,
      OfferToReceiveVideo: false,
    },
  };
  rtcPeerConnection.onicecandidate = onIceCandidate;
  // The actual offer is sent to the success callback.
  rtcPeerConnection.createOffer(
    onOfferCreated, /* callback for creation success */
    () => {}, /* TODO: callback for creation failure */
    sdpConstraints /* configuration object */);
}

// Callback for when we receive a message from the server via the WebSocket.
function onWebSocketMessage(event) {
  console.log("onWebSocketMessage")
  let messageObject = "";
  try {
      messageObject = JSON.parse(event.data);
  } catch(e) {
      messageObject = event.data;
  }
  console.log("onWebSocketMessage type =", messageObject.type, ";event.data= ", event.data)
  if (messageObject.type === WS_PING_OPCODE) {
    const key = messageObject.payload;
    pingLatency[key] = performance.now() - pingTimes[key];
  } else if (messageObject.type === WS_ANSWER_OPCODE) {
    // Client receives and verifies the answer from server.
    // It then starts the ICE protocol which in our example, contacts the STUN server to discover its public IP. 
    rtcPeerConnection.setRemoteDescription(new RTCSessionDescription(messageObject.payload));
  } else if (messageObject.type === WS_CANDIDATE_OPCODE) {
    rtcPeerConnection.addIceCandidate(new RTCIceCandidate(messageObject.payload));
  } else {
    console.log('Unrecognized WebSocket message type.', messageObject);
  }
}

function loopСonnect() {
  for (let i = 0; i < 1000; i++) {
    connect();
    // sleep(10);
  }
}

function loopСonnectWSOnly() {
  for (let i = 0; i < 1000; i++) {
    connectWSOnly();
    // sleep(10);
  }
}

function loopСonnectWRTC() {
  for (let i = 0; i < 1000; i++) {
    connectWRTC();
    // sleep(10);
  }
}

// Connects by creating a new WebSocket connection and associating some callbacks.
function connect() {
    webSocketConnection = new WebSocket(webSocketUrl);
    /*webSocketConnection = new WebSocketClient({
                                            httpServer: webSocketUrl//,
                                            //maxReceivedFrameSize: 131072,
                                            //maxReceivedMessageSize: 10 * 1024 * 1024,
                                            //autoAcceptConnections: false
                                        });*/
    webSocketConnection.onopen = onWebSocketOpen;
    webSocketConnection.onmessage = onWebSocketMessage;
}

// Connects by creating a new WebSocket connection and associating some callbacks.
function connectWSOnly() {
    webSocketConnection = new WebSocket(webSocketUrl);
    /*webSocketConnection = new WebSocketClient({
                                          httpServer: webSocketUrl//,
                                          //maxReceivedFrameSize: 131072,
                                          //maxReceivedMessageSize: 10 * 1024 * 1024,
                                          //autoAcceptConnections: false
                                      });*/
    webSocketConnection.onmessage = onWebSocketMessage;
}

function disconnectWRTCOnly() {
  //dataChannel.close();
  //rtcPeerConnection.onclose = function () {}; // disable onclose handler first
  rtcPeerConnection.close();
  //webSocketConnection = null;
  //webSocketConnection.onmessage = null;
}

function disconnectWSOnly() {
  webSocketConnection.onclose = function () {}; // disable onclose handler first
  webSocketConnection.close();
  //webSocketConnection = null;
  //webSocketConnection.onmessage = null;
}

function printLatency() {
  for (let i = 0; i < PINGS_PER_SECOND * SECONDS_TO_PING; i++) {
    console.log(i + ': ' + pingLatency[i + '']);
  }
}

function sendDataChannelPing() {
  const key = pingCount + '';
  pingTimes[key] = performance.now();
  dataChannel.send(JSON.stringify({type: WRTC_PING_OPCODE, payload: key}));
  pingCount++;
  if (pingCount === PINGS_PER_SECOND * SECONDS_TO_PING) {
    clearInterval(pingInterval);
    console.log('total time: ' + (performance.now() - startTime));
    setTimeout(printLatency, 10000);
    pingCount = 0;
  }
}

function sendWebSocketPing() {
  const key = pingCount + '';
  pingTimes[key] = performance.now();
  webSocketConnection.send(JSON.stringify({type: WS_PING_OPCODE, payload: key}));
  pingCount++;
  if (pingCount === PINGS_PER_SECOND * SECONDS_TO_PING) {
    clearInterval(pingInterval);
    console.log('total time: ' + (performance.now() - startTime));
    setTimeout(printLatency, 10000);
    pingCount = 0;
  }
}

// Pings the server via the DataChannel once the connection has been established.
function pingDataChannel() {
  startTime = performance.now();
  pingInterval = setInterval(sendDataChannelPing, 1000.0 / PINGS_PER_SECOND);
}

function serverTimeDataChannel() {
  dataChannel.send(JSON.stringify({type: WRTC_SERVER_TIME_OPCODE}));
}

// Pings the server via the DataChannel once the connection has been established.
function pingWebSocket() {
  startTime = performance.now();
  pingInterval = setInterval(sendWebSocketPing, 1000.0 / PINGS_PER_SECOND);
}
