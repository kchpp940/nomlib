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
#ifndef NOMLIB_GUI_PUBLIC_HEADERS_HPP
#define NOMLIB_GUI_PUBLIC_HEADERS_HPP

#include "nomlib/config.hpp"

#include "nomlib/gui/Drawables.hpp"
#include "nomlib/gui/IDecorator.hpp"
#include "nomlib/gui/Decorator.hpp"
// #include "nomlib/gui/MinimalDecorator.hpp"
#include "nomlib/gui/FinalFantasyFrame.hpp"
#include "nomlib/gui/FinalFantasyDecorator.hpp"

#include "nomlib/gui/RocketFileInterface.hpp"
#include "nomlib/gui/RocketSDL2SystemInterface.hpp"
#include "nomlib/gui/RocketSDL2RenderInterface.hpp"

#include "nomlib/gui/DecoratorInstancerFinalFantasyFrame.hpp"
#include "nomlib/gui/DecoratorInstancerSprite.hpp"

#include "nomlib/gui/UIWidget.hpp"
#include "nomlib/gui/UIMessageBox.hpp"
#include "nomlib/gui/UIQuestionDialogBox.hpp"
#include "nomlib/gui/UIDataViewList.hpp"
#include "nomlib/gui/UIContext.hpp"
#include "nomlib/gui/UIEventListener.hpp"

#include "nomlib/gui/init_librocket.hpp"

#include "nomlib/gui/ResourceLoader.hpp"

#endif // include guard defined
