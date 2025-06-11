#ifndef __NETWORK_WEBSOCKET_SOCKET_H__
#define __NETWORK_WEBSOCKET_SOCKET_H__

#include <network/TcpSocket.h>
#include <rdr/BufferedInStream.h>
#include <rdr/BufferedOutStream.h>
#include <string>

namespace network {

class WebSocketSocket : public TcpSocket {
public:
  WebSocketSocket(const char *host, int port);
  ~WebSocketSocket();

  rdr::InStream& wsInStream() { return *wsis; }
  rdr::OutStream& wsOutStream() { return *wsos; }

private:
  class WSInStream : public rdr::BufferedInStream {
  public:
    WSInStream(rdr::InStream *in);
  private:
    bool fillBuffer() override;
    rdr::InStream *in;
  };

  class WSOutStream : public rdr::BufferedOutStream {
  public:
    WSOutStream(rdr::OutStream *out);
    void flush() override;
    void cork(bool enable) override;
  private:
    bool flushBuffer() override;
    void writeFrame(const uint8_t *data, size_t length);
    rdr::OutStream *out;
  };

  void handshake(const char *host, int port);

  WSInStream *wsis;
  WSOutStream *wsos;
};

}

#endif
