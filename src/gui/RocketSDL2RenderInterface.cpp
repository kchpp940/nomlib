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
#include "nomlib/gui/RocketSDL2RenderInterface.hpp"

// Private headers (third-party)
#include <SDL.h>

#include <SDL_opengl.h>

#if !(SDL_VIDEO_RENDER_OGL)
  #error "Only the opengl sdl backend is supported. To add support for others, see http://mdqinc.com/blog/2013/01/integrating-librocket-with-sdl-2/"
#endif

// Forward declarations
#include "nomlib/graphics/RenderWindow.hpp"

// Private headers
#include "nomlib/graphics/Image.hpp"
#include "nomlib/graphics/Texture.hpp"
#include "nomlib/graphics/RenderStateGuard.hpp"

namespace nom {

priv::glUseProgramObjectARB_func RocketSDL2RenderInterface::ctx_ = nullptr;

// static
bool RocketSDL2RenderInterface::gl_init( int width, int height )
{
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  glMatrixMode( GL_PROJECTION | GL_MODELVIEW );
  glLoadIdentity();
  glOrtho( 0, width, height, 0, 0, 1 );

  if( SDL_GL_ExtensionSupported("GL_ARB_shader_objects") == false ) {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_APPLICATION,
                  "OpenGL extension 'GL_ARB_shader_objects' is unsupported." );
    return false;
  }
  if( SDL_GL_ExtensionSupported("GL_ARB_shading_language_100") == false ) {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_APPLICATION,
                  "OpenGL extension 'GL_ARB_shading_language_100' is unsupported." );
    return false;
  }
  if( SDL_GL_ExtensionSupported("GL_ARB_vertex_shader") == false ) {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_APPLICATION,
                  "OpenGL extension 'GL_ARB_vertex_shader' is unsupported." );
    return false;
  }
  if( SDL_GL_ExtensionSupported("GL_ARB_fragment_shader") == false ) {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_APPLICATION,
                  "OpenGL extension 'GL_ARB_fragment_shader' is unsupported." );
    return false;
  }

  RocketSDL2RenderInterface::ctx_ =
    (priv::glUseProgramObjectARB_func) SDL_GL_GetProcAddress("glUseProgramObjectARB");

  NOM_ASSERT(RocketSDL2RenderInterface::ctx_ != nullptr);
  if( RocketSDL2RenderInterface::ctx_ == nullptr ) {
    return false;
  }

  return true;
}

RocketSDL2RenderInterface::RocketSDL2RenderInterface( RenderWindow* window )
{
  this->window_ = window;
}

RocketSDL2RenderInterface::~RocketSDL2RenderInterface()
{
  RocketSDL2RenderInterface::ctx_ = nullptr;
}

void RocketSDL2RenderInterface::Release()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE, nom::NOM_LOG_PRIORITY_VERBOSE );

  delete this;
}

Point2f RocketSDL2RenderInterface::logical_scale() const
{
  NOM_ASSERT( this->window_ != nullptr );
  Point2f scale( 1.0f, 1.0f );

  if( this->window_ != nullptr ) {
    SDL_RenderGetScale( this->window_->renderer(), &scale.x, &scale.y );
  }

  return scale;
}

void RocketSDL2RenderInterface::
RenderGeometry( Rocket::Core::Vertex* vertices, int num_vertices, int* indices,
                int num_indices, Rocket::Core::TextureHandle texture_handle,
                const Rocket::Core::Vector2f& translation )
{
  NOM_ASSERT( this->window_ != nullptr );
  if( this->window_ == nullptr ) {
    return;
  }

  // --- Render State Isolation ----------------------------------------------
  // Save the full SDL + OpenGL state BEFORE we touch anything. This guard
  // ensures that when libRocket's GL bypass code exits, all rendering state
  // (color, blend, viewport, scissor, texture binding, vertex arrays, ...)
  // is restored to exactly what it was before we entered. This prevents
  // GUI rendering from polluting subsequent Sprite / Texture / Shape draws.
  RenderStateGuard state_guard( this->window_->renderer(),
                                RenderStateGuard::Scope::All );

  SDL_Texture* sdl_texture = nullptr;
  Point2f scale = this->logical_scale();

  if( RocketSDL2RenderInterface::ctx_ ) {
    RocketSDL2RenderInterface::ctx_(0);
  }

  glPushMatrix();
  glTranslatef( translation.x * scale.x, translation.y * scale.y, 0.0f );

  std::vector<Rocket::Core::Vector2f> Positions(num_vertices);
  std::vector<Rocket::Core::Colourb> Colors(num_vertices);
  std::vector<Rocket::Core::Vector2f> TexCoords(num_vertices);
  float texw = 0.0f, texh = 0.0f;

  auto nom_texture = (nom::Texture*)texture_handle;
  if( nom_texture != nullptr ) {
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    sdl_texture = (SDL_Texture*) nom_texture->texture();
    SDL_GL_BindTexture( sdl_texture, &texw, &texh );
  }

  for( int i = 0; i < num_vertices; ++i )
  {
    Positions[i].x = vertices[i].position.x * scale.x;
    Positions[i].y = vertices[i].position.y * scale.y;
    Colors[i] = vertices[i].colour;

    if( sdl_texture != nullptr ) {
      TexCoords[i].x = vertices[i].tex_coord.x * texw;
      TexCoords[i].y = vertices[i].tex_coord.y * texh;
    } else {
      TexCoords[i] = vertices[i].tex_coord;
    }
  }

  glEnableClientState( GL_VERTEX_ARRAY );
  glEnableClientState( GL_COLOR_ARRAY );
  glVertexPointer( 2, GL_FLOAT, 0, &Positions[0] );
  glColorPointer( 4, GL_UNSIGNED_BYTE, 0, &Colors[0] );
  glTexCoordPointer( 2, GL_FLOAT, 0, &TexCoords[0] );

  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glDrawElements( GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, indices );

  glDisableClientState( GL_VERTEX_ARRAY );
  glDisableClientState( GL_COLOR_ARRAY );

  if( sdl_texture != nullptr ) {
    SDL_GL_UnbindTexture( sdl_texture );
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
  }

  glPopMatrix();

  // NOTE: state_guard destructor fires here and restores EVERYTHING
  // (draw color, blend mode, viewport, clip, GL matrices, scissor, ...).
  //
  // This cleanly replaces the previous hacks:
  //   - glColor4f(1,1,1,1)  +  hardcoded set_color(Color4i::Blue)
  //   - glDisable(GL_BLEND) + SDL_SetRenderDrawBlendMode(NONE)
  //   - SDL_RenderDrawPoint(-1, -1) trick to flush SDL internal state
}

void RocketSDL2RenderInterface::EnableScissorRegion( bool enable )
{
  NOM_ASSERT( this->window_ != nullptr );

  if( enable ) {
    glEnable( GL_SCISSOR_TEST );
  } else {
    glDisable( GL_SCISSOR_TEST );
  }
}

void RocketSDL2RenderInterface::SetScissorRegion( int x, int y, int width, int height )
{
  NOM_ASSERT( this->window_ != nullptr );
  if( this->window_ == nullptr ) {
    return;
  }

  Point2f scale    = this->logical_scale();
  IntRect viewport = this->window_->viewport();
  Size2i output    = this->window_->output_size();

  viewport.x = viewport.x * scale.x;
  viewport.y = viewport.y * scale.y;
  viewport.w = viewport.w * scale.x;
  viewport.h = viewport.h * scale.y;
  y = y * scale.y;

  // Calculations taken from the source at SDL_render_gl.c:GL_UpdateViewport
  // with modification.
  //
  // Incorrect calculations done here can result in bugs with libRocket's
  // scrollbar functionality.
  glScissor(  viewport.x,
              ( output.h - viewport.y - y - viewport.h ),
              viewport.w,
              viewport.h );
}

bool RocketSDL2RenderInterface::LoadTexture( Rocket::Core::TextureHandle& texture_handle,
                                             Rocket::Core::Vector2i& texture_dimensions,
                                             const Rocket::Core::String& source )
{
  Rocket::Core::FileInterface* file_interface = Rocket::Core::GetFileInterface();
  Rocket::Core::FileHandle file_handle = file_interface->Open( source );

  if( !file_handle ) {
    NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION,
                  "Could not obtain file handle for source:",
                  source.CString() );
    return false;
  }

  file_interface->Seek( file_handle, 0, SEEK_END );
  nom::size_type buffer_size = file_interface->Tell( file_handle );
  file_interface->Seek( file_handle, 0, SEEK_SET );

  char* buffer = new char[buffer_size];
  file_interface->Read( buffer, buffer_size, file_handle );
  file_interface->Close( file_handle );

  nom::size_type i;
  for( i = source.Length() - 1; i > 0; --i ) {
    if( source[i] == '.' ) {
      break;
    }
  }

  Rocket::Core::String extension = source.Substring( i + 1, source.Length() - i );

  Image surface;
  Texture* texture = new Texture();
  NOM_ASSERT( texture != nullptr );

  if( surface.load_memory( buffer, buffer_size, extension.CString() ) == true ) {
    if( texture->create( surface ) == true ) {
      texture_handle = (Rocket::Core::TextureHandle) texture;
      texture_dimensions =
        Rocket::Core::Vector2i( surface.width(), surface.height() );
    } else {
      NOM_DELETE_PTR( texture );
      NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION,
                    "Could not create texture handle from image source." );
      delete[] buffer;
      return false;
    }
    delete[] buffer;
    return true;
  }

  delete[] buffer;
  NOM_DELETE_PTR( texture );
  NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION, "Could not create texture handle." );
  return false;
}

bool RocketSDL2RenderInterface::GenerateTexture( Rocket::Core::TextureHandle& texture_handle,
                                                 const Rocket::Core::byte* source,
                                                 const Rocket::Core::Vector2i& source_dimensions )
{
  #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    Uint32 rmask = 0xff000000;
    Uint32 gmask = 0x00ff0000;
    Uint32 bmask = 0x0000ff00;
    Uint32 amask = 0x000000ff;
  #else
    Uint32 rmask = 0x000000ff;
    Uint32 gmask = 0x0000ff00;
    Uint32 bmask = 0x00ff0000;
    Uint32 amask = 0xff000000;
  #endif

  Image surface;
  bool ret;

  ret = surface.initialize(
                            (void*) source,
                            source_dimensions.x,
                            source_dimensions.y,
                            32,
                            source_dimensions.x * 4,
                            rmask, gmask, bmask, amask );

  Texture* texture = new Texture();
  NOM_ASSERT( texture != nullptr );

  if( ret ) {
    if( texture->create( surface ) == false ) {
      NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION,
                    "Could not generate texture from pixel data." );
      NOM_DELETE_PTR( texture );
      return false;
    }

    SDL_SetTextureBlendMode( texture->texture(), SDL_BLENDMODE_BLEND );
    texture_handle = (Rocket::Core::TextureHandle) texture;

    return true;
  }

  NOM_DELETE_PTR( texture );

  NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION, "Could not generate texture." );
  return false;
}

void RocketSDL2RenderInterface::
ReleaseTexture( Rocket::Core::TextureHandle texture_handle )
{
  auto texture = (nom::Texture*) texture_handle;
  if( texture != nullptr ) {
    NOM_DELETE_PTR( texture );
  }
}

} // namespace nom
