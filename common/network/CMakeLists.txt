add_library(network STATIC
  Socket.cxx
  TcpSocket.cxx)

target_sources(network PRIVATE WebSocketSocket.cxx)

if(NOT WIN32)
  target_sources(network PRIVATE UnixSocket.cxx)
endif()

target_include_directories(network PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(network core rdr)
if(OPENSSL_FOUND)
  target_include_directories(network SYSTEM PUBLIC ${OPENSSL_INCLUDE_DIR})
  target_link_libraries(network ${OPENSSL_CRYPTO_LIBRARY})
endif()

if(WIN32)
	target_link_libraries(network ws2_32)
endif()

if(UNIX)
  libtool_create_control_file(network)
endif()
