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

// Callback for when we receive a message on the data channel.
function onDataChannelMessage(event) {
  const key = event.data;
  pingLatency[key] = performance.now() - pingTimes[key];
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
    webSocketConnection.send(JSON.stringify({type: 'candidate', payload: event.candidate}));
  }
}

/*
 * Callback for when the SDP offer was successfully created.
 * Used to create an offer to connect and send it to the server
 * Here the local description is set
 * The server should reply with the answer message 
**/
function onOfferCreated(description) {
  // description contains information about media capabilities
  // (for example, if it has a webcam or can play audio).
  rtcPeerConnection.setLocalDescription(description);
  webSocketConnection.send(JSON.stringify({type: 'offer', payload: description}));
}

/*
 * Callback for when the WebSocket is successfully opened.
 * Then we send a socket.io message to the server,
 * so that it knows to prepare a peerconnection for us to connect to.
**/
function onWebSocketOpen() {
  const config = { iceServers: [{
    // TODO >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    url: 'stun:stun.l.google.com:19302'
    // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  }] };
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
  const messageObject = JSON.parse(event.data);
  if (messageObject.type === 'ping') {
    const key = messageObject.payload;
    pingLatency[key] = performance.now() - pingTimes[key];
  } else if (messageObject.type === 'answer') {
    // Client receives and verifies the answer from server.
    // It then starts the ICE protocol which in our example, contacts the STUN server to discover its public IP. 
    rtcPeerConnection.setRemoteDescription(new RTCSessionDescription(messageObject.payload));
  } else if (messageObject.type === 'candidate') {
    rtcPeerConnection.addIceCandidate(new RTCIceCandidate(messageObject.payload));
  } else {
    console.log('Unrecognized WebSocket message type.');
  }
}

// Connects by creating a new WebSocket connection and associating some callbacks.
function connect() {
  webSocketConnection = new WebSocket(webSocketUrl);
  webSocketConnection.onopen = onWebSocketOpen;
  webSocketConnection.onmessage = onWebSocketMessage;
}

function printLatency() {
  for (let i = 0; i < PINGS_PER_SECOND * SECONDS_TO_PING; i++) {
    console.log(i + ': ' + pingLatency[i + '']);
  }
}

function sendDataChannelPing() {
  const key = pingCount + '';
  pingTimes[key] = performance.now();
  dataChannel.send(key);
  pingCount++;
  if (pingCount === PINGS_PER_SECOND * SECONDS_TO_PING) {
    clearInterval(pingInterval);
    console.log('total time: ' + (performance.now() - startTime));
    setTimeout(printLatency, 10000);
  }
}

function sendWebSocketPing() {
  const key = pingCount + '';
  pingTimes[key] = performance.now();
  webSocketConnection.send(JSON.stringify({type: 'ping', payload: key}));
  pingCount++;
  if (pingCount === PINGS_PER_SECOND * SECONDS_TO_PING) {
    clearInterval(pingInterval);
    console.log('total time: ' + (performance.now() - startTime));
    setTimeout(printLatency, 10000);
  }
}

// Pings the server via the DataChannel once the connection has been established.
function ping() {
  startTime = performance.now();
  pingInterval = setInterval(sendDataChannelPing, 1000.0 / PINGS_PER_SECOND);
  //pingInterval = setInterval(sendWebSocketPing, 1000.0 / PINGS_PER_SECOND);
}