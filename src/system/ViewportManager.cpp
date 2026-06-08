/******************************************************************************

  nomlib - C++11 cross-platform game engine

Copyright (c) 2013, 2014 Jeffrey Carpenter <i8degrees@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
#include "nomlib/system/ViewportManager.hpp"

namespace nom {

Point2i ViewportManager::to_logical(const Point2i& window_pos) const
{
  if( !this->bound ) {
    return window_pos;
  }
  const float sx = (this->scale.x > 0.0f) ? this->scale.x : 1.0f;
  const float sy = (this->scale.y > 0.0f) ? this->scale.y : 1.0f;
  return Point2i( static_cast<int>((window_pos.x - this->viewport.x) / sx),
                  static_cast<int>((window_pos.y - this->viewport.y) / sy) );
}

Point2i ViewportManager::to_logical_delta(const Point2i& window_delta) const
{
  if( !this->bound ) {
    return window_delta;
  }
  const float sx = (this->scale.x > 0.0f) ? this->scale.x : 1.0f;
  const float sy = (this->scale.y > 0.0f) ? this->scale.y : 1.0f;
  return Point2i( static_cast<int>(window_delta.x / sx),
                  static_cast<int>(window_delta.y / sy) );
}

} // namespace nom
