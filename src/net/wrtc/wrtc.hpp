#pragma once

namespace gloer {
namespace net {
namespace wrtc {

/// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";
static const char kAnswerSdpName[] = "answer";

/// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

/// Labels for ICE streams.
const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamLabel[] = "stream_label";

/// Example Server URIs for ICE candidates.
/* Example:
 * "stun:stun.l.google.com:19302";
 * "stun:stun1.l.google.com:19302";
 * "stun:stun2.l.google.com:19305";
 * "stun:stun01.sipphone.com";
 * "stun:stunserver.org";
 */
const char kGoogleStunServerUri[] = "stun:stun.l.google.com:19302";

} // namespace wrtc
} // namespace net
} // namespace gloer
