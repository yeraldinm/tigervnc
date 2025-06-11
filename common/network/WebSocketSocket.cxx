#include "WebSocketSocket.h"
#include <openssl/sha.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <vector>
#include <sstream>

using namespace network;

static const char b64_table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Encode(const uint8_t *data, size_t len)
{
  std::string out;
  for (size_t i = 0; i < len; i += 3) {
    uint32_t v = data[i] << 16;
    if (i + 1 < len) v |= data[i + 1] << 8;
    if (i + 2 < len) v |= data[i + 2];
    out.push_back(b64_table[(v >> 18) & 0x3f]);
    out.push_back(b64_table[(v >> 12) & 0x3f]);
    if (i + 1 < len) out.push_back(b64_table[(v >> 6) & 0x3f]);
    else out.push_back('=');
    if (i + 2 < len) out.push_back(b64_table[v & 0x3f]);
    else out.push_back('=');
  }
  return out;
}

static std::string wsAccept(const std::string &key)
{
  static const char guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  SHA_CTX ctx;
  uint8_t hash[SHA_DIGEST_LENGTH];
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, key.data(), key.size());
  SHA1_Update(&ctx, guid, strlen(guid));
  SHA1_Final(hash, &ctx);
  return base64Encode(hash, SHA_DIGEST_LENGTH);
}

WebSocketSocket::WSInStream::WSInStream(rdr::InStream *i)
  : in(i)
{
}

bool WebSocketSocket::WSInStream::fillBuffer()
{
  if (!in->hasData(2))
    return false;
  const uint8_t *p = in->getptr(2);
  uint8_t b1 = p[0];
  uint8_t b2 = p[1];
  size_t pos = 2;
  uint64_t len = b2 & 0x7f;
  if (len == 126) {
    if (!in->hasData(pos + 2))
      return false;
    p = in->getptr(pos + 2);
    len = ((uint64_t)p[pos] << 8) | p[pos + 1];
    pos += 2;
  } else if (len == 127) {
    if (!in->hasData(pos + 8))
      return false;
    p = in->getptr(pos + 8);
    len = 0;
    for (int i = 0; i < 8; i++) {
      len = (len << 8) | p[pos + i];
    }
    pos += 8;
  }
  uint8_t mask[4] = {0,0,0,0};
  bool masked = b2 & 0x80;
  if (masked) {
    if (!in->hasData(pos + 4))
      return false;
    p = in->getptr(pos + 4);
    memcpy(mask, p + pos, 4);
    pos += 4;
  }
  if (!in->hasData(pos + len))
    return false;
  ensureSpace(len);
  p = in->getptr(pos + len);
  const uint8_t *data = p + pos;
  for (size_t i = 0; i < len; i++)
    end[i] = data[i] ^ (masked ? mask[i % 4] : 0);
  in->setptr(pos + len);
  end += len;
  (void)b1; // ignore opcode
  return true;
}

WebSocketSocket::WSOutStream::WSOutStream(rdr::OutStream *o)
  : BufferedOutStream(false), out(o)
{
}

void WebSocketSocket::WSOutStream::flush()
{
  BufferedOutStream::flush();
  out->flush();
}

void WebSocketSocket::WSOutStream::cork(bool enable)
{
  BufferedOutStream::cork(enable);
  out->cork(enable);
}

bool WebSocketSocket::WSOutStream::flushBuffer()
{
  while (sentUpTo < ptr) {
    size_t n = ptr - sentUpTo;
    writeFrame(sentUpTo, n);
    sentUpTo += n;
  }
  return true;
}

void WebSocketSocket::WSOutStream::writeFrame(const uint8_t *data, size_t len)
{
  uint8_t header[14];
  size_t hlen = 0;
  header[0] = 0x82; // FIN + binary
  if (len <= 125) {
    header[1] = 0x80 | (uint8_t)len;
    hlen = 2;
  } else if (len <= 65535) {
    header[1] = 0x80 | 126;
    header[2] = (len >> 8) & 0xff;
    header[3] = len & 0xff;
    hlen = 4;
  } else {
    header[1] = 0x80 | 127;
    for (int i = 0; i < 8; i++)
      header[2 + i] = (len >> (56 - 8 * i)) & 0xff;
    hlen = 10;
  }
  uint8_t mask[4];
  for (int i = 0; i < 4; i++)
    mask[i] = rand() & 0xff;
  memcpy(header + hlen, mask, 4);
  hlen += 4;
  out->writeBytes(header, hlen);
  std::vector<uint8_t> buf(len);
  for (size_t i = 0; i < len; i++)
    buf[i] = data[i] ^ mask[i % 4];
  out->writeBytes(buf.data(), len);
}

WebSocketSocket::WebSocketSocket(const char *host, int port)
  : TcpSocket(host, port), wsis(nullptr), wsos(nullptr)
{
  handshake(host, port);
  wsis = new WSInStream(&TcpSocket::inStream());
  wsos = new WSOutStream(&TcpSocket::outStream());
}

WebSocketSocket::~WebSocketSocket()
{
  delete wsis;
  delete wsos;
}

void WebSocketSocket::handshake(const char *host, int port)
{
  uint8_t rnd[16];
  FILE *f = fopen("/dev/urandom", "rb");
  if (f) {
    fread(rnd, 1, 16, f);
    fclose(f);
  } else {
    for (int i = 0; i < 16; i++)
      rnd[i] = rand() & 0xff;
  }
  std::string key = base64Encode(rnd, 16);
  std::string expect = wsAccept(key);
  std::ostringstream req;
  req << "GET / HTTP/1.1\r\n";
  req << "Host: " << host << ":" << port << "\r\n";
  req << "Upgrade: websocket\r\n";
  req << "Connection: Upgrade\r\n";
  req << "Sec-WebSocket-Key: " << key << "\r\n";
  req << "Sec-WebSocket-Version: 13\r\n";
  req << "\r\n";
  std::string r = req.str();
  TcpSocket::outStream().writeBytes((const uint8_t*)r.c_str(), r.size());
  TcpSocket::outStream().flush();
  std::string resp;
  while (true) {
    if (!TcpSocket::inStream().hasData(1))
      continue;
    char c = TcpSocket::inStream().readS8();
    resp.push_back(c);
    if (resp.find("\r\n\r\n") != std::string::npos)
      break;
  }
  if (resp.find("101") == std::string::npos)
    throw std::runtime_error("WebSocket upgrade failed");
  size_t p = resp.find("Sec-WebSocket-Accept:");
  if (p == std::string::npos)
    throw std::runtime_error("WebSocket handshake missing accept");
  p += strlen("Sec-WebSocket-Accept:");
  while (p < resp.size() && (resp[p] == ' ' || resp[p] == '\t')) p++;
  size_t e = resp.find('\r', p);
  std::string accept = resp.substr(p, e - p);
  if (accept != expect)
    throw std::runtime_error("WebSocket handshake validation failed");
}
