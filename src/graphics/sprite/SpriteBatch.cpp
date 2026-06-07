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
#include "nomlib/graphics/sprite/SpriteBatch.hpp"

// Private headers
#include "nomlib/core/unique_ptr.hpp"

namespace nom {

SpriteBatch::SpriteBatch() :
  Sprite()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_RENDER, NOM_LOG_PRIORITY_VERBOSE);
}

SpriteBatch::SpriteBatch( const SpriteBatch& other ) :
  Sprite( other )
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_RENDER, NOM_LOG_PRIORITY_VERBOSE);
}

SpriteBatch& SpriteBatch::operator=( const SpriteBatch& other )
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_RENDER, NOM_LOG_PRIORITY_VERBOSE);

  if( this != &other ) {
    Sprite::operator=( other );
  }
  return *this;
}

SpriteBatch::~SpriteBatch()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_RENDER, NOM_LOG_PRIORITY_VERBOSE);
}

ObjectTypeInfo SpriteBatch::type() const
{
  return NOM_OBJECT_TYPE_INFO(self_type);
}

SpriteBatch* SpriteBatch::clone() const
{
  return( new SpriteBatch(*this) );
}

} // namespace nom
