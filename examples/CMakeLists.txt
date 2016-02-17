cmake_minimum_required(VERSION 2.8)
project (zmq_examples)

include(${ARDUINO_SETTINGS_FILE})
 
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
 
include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_INCLUDE_DIR}")

### add example executable directories
add_avr_executable(zmq-monitoringServer-test zmq-monitoringServer-test/zmq-monitoringServer-test.cpp)
add_avr_executable(uiptest uiptest/uiptest.cpp)

### add the dependant libraries
### zmq monitoring server
avr_target_link_libraries(zmq-monitoringServer-test ARDUINO)
#avr_target_link_libraries(zmq-monitoringServer-test AMC7812)
avr_target_link_libraries(zmq-monitoringServer-test UIPETHERNET)
avr_target_link_libraries(zmq-monitoringServer-test ZMQ)
# uipTest
avr_target_link_libraries(uiptest AMC7812)
avr_target_link_libraries(uiptest ARDUINO)
avr_target_link_libraries(uiptest UIPETHERNET)
