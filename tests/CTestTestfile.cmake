# CMake generated Testfile for 
# Source directory: /home/denis/workspace/webrtc-test/tests
# Build directory: /home/denis/workspace/webrtc-test/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(server "/home/denis/workspace/webrtc-test/bin/Release/server" "-r" "console" "--use-colour" "yes" "--order" "rand" "--durations" "yes" "--warn" "NoTests" "--invisibles")
add_test(utils "/home/denis/workspace/webrtc-test/bin/Release/utils" "-r" "console" "--use-colour" "yes" "--order" "rand" "--durations" "yes" "--warn" "NoTests" "--invisibles")
