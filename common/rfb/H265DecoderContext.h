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

#ifndef __RFB_H265DECODERCONTEXT_H__
#define __RFB_H265DECODERCONTEXT_H__

#include <stdint.h>

#include <core/Rect.h>

namespace rfb {

  class ModifiablePixelBuffer;

  class H265DecoderContext {
    public:
      static H265DecoderContext* createContext(const core::Rect& r);

      virtual ~H265DecoderContext() = 0;

      virtual void decode(const uint8_t* /*h265_buffer*/,
                          uint32_t /*len*/,
                          ModifiablePixelBuffer* /*pb*/) {}

      inline bool isEqualRect(const core::Rect &r) const { return r == rect; }

    protected:
      core::Rect rect;

      H265DecoderContext(const core::Rect &r) : rect(r) {}
  };

}

#endif
