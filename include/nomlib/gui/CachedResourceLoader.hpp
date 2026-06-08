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
#ifndef NOMLIB_GUI_CACHED_RESOURCE_LOADER_HPP
#define NOMLIB_GUI_CACHED_RESOURCE_LOADER_HPP

#include <string>
#include <map>
#include <vector>

#include "nomlib/config.hpp"
#include "nomlib/serializers/ResourceManifest.hpp"
#include "nomlib/gui/UIContext.hpp"
#include "nomlib/gui/RocketUtilities.hpp"
#include "nomlib/gui/ResourceLoader.hpp"

namespace nom {
namespace gui {

/// \brief Manifest-aware UI resource loader with per-id caching, eager
///        preload, lazy load-on-first-access, and tag-based bulk load/release.
///
/// \remarks Fonts are registered into the attached UIContext (Rocket handles
///          internal deduplication; the loader only tracks which manifest ids
///          have been resolved and loaded to avoid redundant manifest lookups
///          and path resolution).  Fonts cannot be individually unloaded from
///          Rocket; release() merely drops the loader's bookkeeping record.
///
///          Documents come in two flavours:
///          - Shared cached instances returned by get_document(): cached by id,
///            automatically Close()d on release()/clear()/destruction.  Useful
///            for one-off windows that live for the whole app lifecycle.
///          - Independent instances returned by create_document(): a fresh
///            Rocket document is created on every call and is **never** cached;
///            the caller (typically a UIWidget) owns the lifetime and should
///            call Close() when done, or rely on UIContext::shutdown().
///
///          This class is a non-owning view of a UIContext; the context must
///          outlive the loader.
class CachedResourceLoader
{
  public:
    typedef CachedResourceLoader self_type;

    CachedResourceLoader( void );
    CachedResourceLoader( const ResourceManifest& manifest,
                          UIContext* context );

    ~CachedResourceLoader( void );

    CachedResourceLoader( const self_type& ) = delete;
    self_type& operator =( const self_type& ) = delete;

    CachedResourceLoader( self_type&& other ) noexcept;
    self_type& operator =( self_type&& other ) noexcept;

    void set_manifest( const ResourceManifest& manifest );
    void set_context( UIContext* context );

    const ResourceManifest& manifest( void ) const;
    UIContext* context( void ) const;

    // -----------------------------------------------------------------------
    // Lazy cached access
    // -----------------------------------------------------------------------

    /// \brief Lazily load (or return the cached result for) a UI font by
    ///        manifest id.
    ///
    /// \returns true if the font is available in the UIContext (either already
    ///          present or just successfully loaded).
    bool get_font( const std::string& id );

    /// \brief Get the shared cached instance of a UI document, loading it on
    ///        first access.
    ///
    /// \returns Non-owning pointer to the cached document, valid until
    ///          release(), clear(), UIContext::shutdown(), or loader
    ///          destruction.  Returns nullptr on load failure.
    ///
    /// \remarks Multiple calls with the same id return the same document
    ///          pointer.  If you need an independent document instance (for
    ///          example two message boxes with different titles), use
    ///          create_document().
    void* get_document( const std::string& id );

    /// \brief Always create a new, independent UI document instance.
    ///
    /// \returns Non-owning pointer to the newly created document, or nullptr
    ///          on failure.  The pointer is never stored in the cache; the
    ///          caller is responsible for the document lifetime
    ///          (e.g. UIWidget::close() or UIContext::shutdown()).
    void* create_document( const std::string& id );

    // -----------------------------------------------------------------------
    // Preload
    // -----------------------------------------------------------------------

    /// \brief Preload all Font/UI entries marked preload=Eager.
    ///
    /// \returns true if all eager resources loaded successfully.
    bool preload_eager( void );

    /// \brief Preload all Font/UI entries carrying the given tag.
    ///
    /// \returns Count of resources successfully (pre)loaded.
    nom::size_type preload_by_tag( const std::string& tag );

    // -----------------------------------------------------------------------
    // Release
    // -----------------------------------------------------------------------

    /// \brief Release a single cached resource by id.
    ///
    /// \remarks For cached documents, Rocket::Core::ElementDocument::Close()
    ///          is called and the cache entry is erased.  For fonts, only the
    ///          bookkeeping record is dropped (Rocket provides no per-font
    ///          unload API; the font remains registered until shutdown).
    ///          Independent documents created via create_document() are never
    ///          affected.
    ///
    /// \returns true if the resource was found and released.
    bool release( const std::string& id );

    /// \brief Release all cached Font/UI resources whose manifest entry
    ///        carries the given tag.
    nom::size_type release_by_tag( const std::string& tag );

    /// \brief Release ALL cached UI resources (manifests and context remain
    ///        attached).
    void clear( void );

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    bool is_loaded( const std::string& id ) const;
    nom::size_type loaded_count( void ) const;
    void dump( void ) const;

  private:
    ResourceManifest manifest_;
    UIContext* context_;

    std::map<std::string, bool>                        cache_ui_fonts_;
    std::map<std::string, Rocket::Core::ElementDocument*> cache_ui_docs_;
};

} // namespace gui
} // namespace nom

#endif // include guard defined

// Inline implementation ------------------------------------------------------

namespace nom {
namespace gui {

inline CachedResourceLoader::CachedResourceLoader( void ) :
  context_( nullptr )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

inline CachedResourceLoader::CachedResourceLoader( const ResourceManifest& manifest,
                                                   UIContext* context ) :
  manifest_( manifest ),
  context_( context )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
}

inline CachedResourceLoader::~CachedResourceLoader( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, NOM_LOG_PRIORITY_VERBOSE );
  this->clear();
}

inline CachedResourceLoader::CachedResourceLoader( self_type&& other ) noexcept :
  manifest_( std::move( other.manifest_ ) ),
  context_( other.context_ ),
  cache_ui_fonts_( std::move( other.cache_ui_fonts_ ) ),
  cache_ui_docs_( std::move( other.cache_ui_docs_ ) )
{
  other.context_ = nullptr;
  other.cache_ui_fonts_.clear();
  other.cache_ui_docs_.clear();
}

inline CachedResourceLoader::self_type&
CachedResourceLoader::operator =( self_type&& other ) noexcept
{
  if( this != &other )
  {
    this->clear();
    this->manifest_ = std::move( other.manifest_ );
    this->context_ = other.context_;
    this->cache_ui_fonts_ = std::move( other.cache_ui_fonts_ );
    this->cache_ui_docs_ = std::move( other.cache_ui_docs_ );
    other.context_ = nullptr;
    other.cache_ui_fonts_.clear();
    other.cache_ui_docs_.clear();
  }
  return *this;
}

inline void
CachedResourceLoader::set_manifest( const ResourceManifest& manifest )
{
  this->clear();
  this->manifest_ = manifest;
}

inline void
CachedResourceLoader::set_context( UIContext* context )
{
  this->clear();
  this->context_ = context;
}

inline const ResourceManifest&
CachedResourceLoader::manifest( void ) const
{
  return this->manifest_;
}

inline UIContext*
CachedResourceLoader::context( void ) const
{
  return this->context_;
}

inline bool
CachedResourceLoader::get_font( const std::string& id )
{
  auto itr = this->cache_ui_fonts_.find( id );
  if( itr != this->cache_ui_fonts_.end() )
  {
    return itr->second;
  }

  if( this->context_ == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "gui::CachedResourceLoader::get_font - no context attached." );
    this->cache_ui_fonts_[id] = false;
    return false;
  }

  bool ok = nom::load_ui_font( this->manifest_, id, *this->context_ );
  this->cache_ui_fonts_[id] = ok;
  return ok;
}

inline void*
CachedResourceLoader::get_document( const std::string& id )
{
  auto itr = this->cache_ui_docs_.find( id );
  if( itr != this->cache_ui_docs_.end() )
  {
    return itr->second;
  }

  if( this->context_ == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "gui::CachedResourceLoader::get_document - no context attached." );
    return nullptr;
  }

  auto doc = nom::load_ui_document( this->manifest_, id, *this->context_ );
  if( doc == nullptr )
  {
    return nullptr;
  }
  this->cache_ui_docs_[id] = static_cast<Rocket::Core::ElementDocument*>( doc );
  return doc;
}

inline void*
CachedResourceLoader::create_document( const std::string& id )
{
  if( this->context_ == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "gui::CachedResourceLoader::create_document - no context attached." );
    return nullptr;
  }

  return nom::load_ui_document( this->manifest_, id, *this->context_ );
}

inline bool
CachedResourceLoader::preload_eager( void )
{
  if( this->context_ == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "gui::CachedResourceLoader::preload_eager - no context attached." );
    return false;
  }

  bool all_ok = true;
  auto fonts = this->manifest_.find_by_type( ResourceType::Font );
  auto docs  = this->manifest_.find_by_type( ResourceType::UI );

  for( const auto& e : fonts )
  {
    if( e.preload() != PreloadStrategy::Eager ) continue;
    if( this->is_loaded( e.id() ) ) continue;
    if( this->get_font( e.id() ) == false ) all_ok = false;
  }

  for( const auto& e : docs )
  {
    if( e.preload() != PreloadStrategy::Eager ) continue;
    if( this->is_loaded( e.id() ) ) continue;
    if( this->get_document( e.id() ) == nullptr ) all_ok = false;
  }

  return all_ok;
}

inline nom::size_type
CachedResourceLoader::preload_by_tag( const std::string& tag )
{
  if( this->context_ == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "gui::CachedResourceLoader::preload_by_tag - no context attached." );
    return 0;
  }

  nom::size_type count = 0;
  auto entries = this->manifest_.find_by_tag( tag );

  for( const auto& e : entries )
  {
    if( e.type() != ResourceType::Font && e.type() != ResourceType::UI ) continue;
    if( this->is_loaded( e.id() ) ) { ++count; continue; }

    bool loaded = false;
    if( e.type() == ResourceType::Font )
    {
      loaded = this->get_font( e.id() );
    }
    else if( e.type() == ResourceType::UI )
    {
      loaded = ( this->get_document( e.id() ) != nullptr );
    }
    if( loaded ) ++count;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "gui::CachedResourceLoader::preload_by_tag('", tag,
                "') loaded ", count, " resources." );
  return count;
}

inline bool
CachedResourceLoader::release( const std::string& id )
{
  auto fitr = this->cache_ui_fonts_.find( id );
  if( fitr != this->cache_ui_fonts_.end() )
  {
    this->cache_ui_fonts_.erase( fitr );
    return true;
  }

  auto ditr = this->cache_ui_docs_.find( id );
  if( ditr != this->cache_ui_docs_.end() )
  {
    if( ditr->second != nullptr )
    {
      ditr->second->Close();
    }
    this->cache_ui_docs_.erase( ditr );
    return true;
  }

  return false;
}

inline nom::size_type
CachedResourceLoader::release_by_tag( const std::string& tag )
{
  nom::size_type count = 0;
  auto entries = this->manifest_.find_by_tag( tag );
  for( const auto& e : entries )
  {
    if( e.type() != ResourceType::Font && e.type() != ResourceType::UI ) continue;
    if( this->release( e.id() ) ) ++count;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "gui::CachedResourceLoader::release_by_tag('", tag,
                "') released ", count, " resources." );
  return count;
}

inline void
CachedResourceLoader::clear( void )
{
  for( const auto& kv : this->cache_ui_docs_ )
  {
    if( kv.second != nullptr )
    {
      kv.second->Close();
    }
  }
  this->cache_ui_fonts_.clear();
  this->cache_ui_docs_.clear();
}

inline bool
CachedResourceLoader::is_loaded( const std::string& id ) const
{
  if( this->cache_ui_fonts_.count( id ) > 0 ) return true;
  if( this->cache_ui_docs_.count( id ) > 0 ) return true;
  return false;
}

inline nom::size_type
CachedResourceLoader::loaded_count( void ) const
{
  return this->cache_ui_fonts_.size() + this->cache_ui_docs_.size();
}

inline void
CachedResourceLoader::dump( void ) const
{
  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "gui::CachedResourceLoader (", this->loaded_count(),
                " cached):" );
  for( const auto& kv : this->cache_ui_fonts_ )
  {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM, "  [ui_font] ", kv.first );
  }
  for( const auto& kv : this->cache_ui_docs_ )
  {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM, "  [ui_doc]  ", kv.first );
  }
}

} // namespace gui
} // namespace nom
