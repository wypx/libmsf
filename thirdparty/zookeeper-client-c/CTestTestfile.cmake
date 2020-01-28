# CMake generated Testfile for 
# Source directory: /media/psf/tomato/Ucloud/echoproject/zookeeper/zookeeper-client-c
# Build directory: /media/psf/tomato/Ucloud/echoproject/zookeeper/zookeeper-client-c
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(zktest_runner "/media/psf/tomato/Ucloud/echoproject/zookeeper/zookeeper-client-c/zktest")
set_tests_properties(zktest_runner PROPERTIES  ENVIRONMENT "ZKROOT=/media/psf/tomato/Ucloud/echoproject/zookeeper/zookeeper-client-c/../..;CLASSPATH=\$CLASSPATH:\$CLOVER_HOME/lib/clover*.jar" _BACKTRACE_TRIPLES "/media/psf/tomato/Ucloud/echoproject/zookeeper/zookeeper-client-c/CMakeLists.txt;245;add_test;/media/psf/tomato/Ucloud/echoproject/zookeeper/zookeeper-client-c/CMakeLists.txt;0;")
