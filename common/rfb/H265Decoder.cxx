/* Copyright (C) 2021 Vladimir Sukhonosov <xornet@xornet.org>
 * Copyright (C) 2021 Martins Mozeiko <martins.mozeiko@gmail.com>
 * All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MAX_H265_INSTANCES 64

#include <deque>

#include <rdr/MemInStream.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/H265Decoder.h>
#include <rfb/H265DecoderContext.h>

using namespace rfb;

enum rectFlags {
  resetContext       = 0x1,
  resetAllContexts   = 0x2,
};

H265Decoder::H265Decoder() : Decoder(DecoderOrdered)
{
}

H265Decoder::~H265Decoder()
{
  resetContexts();
}

void H265Decoder::resetContexts()
{
  for (H265DecoderContext* context : contexts)
    delete context;
  contexts.clear();
}

H265DecoderContext* H265Decoder::findContext(const core::Rect& r)
{
  for (H265DecoderContext* context : contexts)
    if (context->isEqualRect(r))
      return context;
  return nullptr;
}

bool H265Decoder::readRect(const core::Rect& /*r*/,
                           rdr::InStream* is,
                           const ServerParams& /*server*/,
                           rdr::OutStream* os)
{
  uint32_t len;

  if (!is->hasData(8))
    return false;

  is->setRestorePoint();

  len = is->readU32();
  os->writeU32(len);
  uint32_t reset = is->readU32();

  os->writeU32(reset);

  if (!is->hasDataOrRestore(len))
    return false;

  is->clearRestorePoint();

  os->copyBytes(is, len);

  return true;
}

void H265Decoder::decodeRect(const core::Rect& r, const uint8_t* buffer,
                             size_t buflen,
                             const ServerParams& /*server*/,
                             ModifiablePixelBuffer* pb)
{
  rdr::MemInStream is(buffer, buflen);
  uint32_t len = is.readU32();
  uint32_t reset = is.readU32();

  H265DecoderContext* ctx = nullptr;
  if (reset & resetAllContexts)
  {
    resetContexts();
    if (!len)
      return;
    reset &= ~(resetContext | resetAllContexts);
  } else {
    ctx = findContext(r);
  }

  if (ctx && (reset & resetContext)) {
    contexts.remove(ctx);
    delete ctx;
    ctx = nullptr;
  }

  if (!ctx)
  {
    if (contexts.size() >= MAX_H265_INSTANCES)
    {
      H265DecoderContext* excess_ctx = contexts.front();
      delete excess_ctx;
      contexts.pop_front();
    }
    ctx = H265DecoderContext::createContext(r);
    if (!ctx)
      throw std::runtime_error("H265Decoder: Context not be created");
    contexts.push_back(ctx);
  }

  if (!len)
    return;

  ctx->decode(is.getptr(len), len, pb);
}
