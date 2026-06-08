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
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/serializers/ResourceManifest.hpp"
#include "nomlib/gui/UIContext.hpp"
#include "nomlib/gui/UIWidget.hpp"
#include "nomlib/gui/RocketUtilities.hpp"
#include "nomlib/gui/ResourceLoader.hpp"

namespace nom {
namespace gui {

/// \brief Custom deleter that calls Rocket::Core::ElementDocument::Close().
///
/// \remarks The document pointer is allowed to be null.
inline void close_document_deleter( Rocket::Core::ElementDocument* doc )
{
  if( doc != nullptr )
  {
    doc->Close();
  }
}

/// \brief Unique-ownership handle for an independent UI document.
///
/// The document is automatically Close()d when the handle goes out of scope,
/// is assigned to, or is reset().
///
/// \see CachedResourceLoader::acquire_document
typedef std::unique_ptr<Rocket::Core::ElementDocument,
                        void(*)(Rocket::Core::ElementDocument*)> UIDocumentPtr;

/// \brief Manifest-aware UI resource loader with per-id caching, eager
///        preload, lazy load-on-first-access, and tag-based bulk load/release.
///
/// UI documents come in two ownership models with distinct APIs — pick the
/// one that matches your use case; **do not mix** them for the same document:
///
/// **Shared cached documents — `get_document()`**
/// - A single instance per manifest id is kept in the loader's cache.
/// - The loader owns the document and will Close() it on
///   release() / release_by_tag() / clear() / destruction.
/// - The returned pointer is **non-owning**: you must NOT call Close() on it
///   and you must NOT pass it to a UIWidget that may call close().
/// - Useful for singleton-style overlays, HUD root documents, etc. that
///   live for the whole application lifetime.
/// - Participates in `preload_eager`, `preload_by_tag`, `is_loaded`,
///   `loaded_count`, `release`, `release_by_tag`.
///
/// **Independent documents — `acquire_document()` / `load_into_widget()`**
/// - Every call creates a brand-new Rocket document instance.
/// - The loader never caches or tracks these instances.  They do not appear
///   in `is_loaded` / `loaded_count`, and `release*` / `clear` do not touch
///   them.
/// - Ownership is transferred to the caller via one of two explicit paths:
///   1. `acquire_document(id)` — returns a `UIDocumentPtr` (unique_ptr with
///      a Close() deleter).  The smart pointer owns the lifetime.
///   2. `load_into_widget(id, widget)` — loads the document directly into a
///      UIWidget.  The widget's user is then responsible for calling
///      `UIWidget::close()` or relying on `UIContext::shutdown()`.
///
/// UI fonts are always loader-owned and internally deduplicated by Rocket;
/// `release()` only drops the bookkeeping record (Rocket provides no per-font
/// unload API until `UIContext::shutdown()`).
///
/// This class is a non-owning view of a UIContext; the context must outlive
/// the loader.
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
    // Lazy cached access (fonts)
    // -----------------------------------------------------------------------

    /// \brief Lazily load (or return the cached result for) a UI font by
    ///        manifest id.
    ///
    /// \returns true if the font is available in the UIContext (either already
    ///          present or just successfully loaded).
    ///
    /// \remarks Fonts are deduplicated internally by libRocket.  The loader
    ///          only caches the resolved manifest state to avoid redundant
    ///          lookups and type checks.
    bool get_font( const std::string& id );

    // -----------------------------------------------------------------------
    // Shared cached documents (loader-owned)
    // -----------------------------------------------------------------------

    /// \brief Get the shared cached instance of a UI document, loading it on
    ///        first access.
    ///
    /// \returns **Non-owning** pointer to the cached document, valid until
    ///          release(), clear(), UIContext::shutdown(), or loader
    ///          destruction.  Returns nullptr on load failure.
    ///
    /// \warning Do **not** call Close() on the returned pointer and do **not**
    ///          hand it to a UIWidget that may invoke close() — the loader
    ///          owns the lifetime.
    ///
    /// \see acquire_document for an independent instance owned by the caller.
    void* get_document( const std::string& id );

    // -----------------------------------------------------------------------
    // Independent documents (caller-owned — two explicit transfer paths)
    // -----------------------------------------------------------------------

    /// \brief Create a fresh, independent document instance and transfer
    ///        ownership via a smart pointer.
    ///
    /// \returns A `UIDocumentPtr` (unique_ptr with Close() deleter) owning
    ///          the newly loaded document, or a null pointer on failure.
    ///          The document is automatically Close()d when the pointer is
    ///          destroyed, reassigned, or reset().
    ///
    /// \remarks The loader never caches or tracks independent instances.
    ///          Each call returns a distinct document.
    ///
    /// \see get_document for a shared, loader-owned instance.
    /// \see load_into_widget to bind the document directly to a UIWidget.
    UIDocumentPtr acquire_document( const std::string& id );

    /// \brief Create a fresh, independent document instance and bind it
    ///        directly to a UIWidget.
    ///
    /// \returns true on success, false if the manifest lookup or document
    ///          load failed.
    ///
    /// \remarks The loader never caches or tracks independent instances.
    ///          Each call loads a distinct document into the widget.
    ///          Ownership responsibility passes to the widget's caller:
    ///          either invoke `widget.close()` when done or rely on
    ///          `UIContext::shutdown()` for final cleanup.
    ///
    ///          This is the recommended way to feed a RML document from the
    ///          manifest into a UIMessageBox, UIQuestionDialogBox, or any
    ///          other UIWidget subclass.
    bool load_into_widget( const std::string& id, UIWidget& widget );

    // -----------------------------------------------------------------------
    // Preload
    // -----------------------------------------------------------------------

    /// \brief Preload all Font/UI entries marked preload=Eager.
    ///
    /// Only shared cached documents (those reachable via get_document) are
    /// eligible for eager preloading; independent documents are always
    /// created on demand.
    ///
    /// \returns true if all eager resources loaded successfully.
    bool preload_eager( void );

    /// \brief Preload all Font/UI entries carrying the given tag.
    ///
    /// Only shared cached documents (those reachable via get_document) are
    /// eligible for tag preloading.
    ///
    /// \returns Count of resources successfully (pre)loaded.
    nom::size_type preload_by_tag( const std::string& tag );

    // -----------------------------------------------------------------------
    // Release
    // -----------------------------------------------------------------------

    /// \brief Release a single cached resource by id.
    ///
    /// For a shared cached document the loader calls Close() and erases the
    /// cache entry.  For a font only the bookkeeping record is dropped
    /// (libRocket provides no per-font unload).
    ///
    /// Independent documents created via acquire_document() or
    /// load_into_widget() are **never** affected — they are owned by the
    /// caller.
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

    /// \brief Whether a font id or shared document id is currently cached.
    ///
    /// \note Independent documents returned by acquire_document() /
    ///       load_into_widget() are never tracked and always report false.
    bool is_loaded( const std::string& id ) const;

    /// \brief Total number of cached fonts + shared cached documents.
    ///
    /// Independent documents are not counted.
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

inline UIDocumentPtr
CachedResourceLoader::acquire_document( const std::string& id )
{
  if( this->context_ == nullptr )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "gui::CachedResourceLoader::acquire_document - no context attached." );
    return UIDocumentPtr( nullptr, &close_document_deleter );
  }

  auto doc = static_cast<Rocket::Core::ElementDocument*>(
    nom::load_ui_document( this->manifest_, id, *this->context_ ) );

  return UIDocumentPtr( doc, &close_document_deleter );
}

inline bool
CachedResourceLoader::load_into_widget( const std::string& id, UIWidget& widget )
{
  const ResourceManifestEntry& entry = this->manifest_.find( id );
  if( ! entry.valid() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "gui::CachedResourceLoader::load_into_widget - unknown manifest id:", id );
    return false;
  }
  if( entry.type() != ResourceType::UI )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "gui::CachedResourceLoader::load_into_widget - id '", id,
                 "' has type '", ResourceManifest::type_to_string( entry.type() ),
                 "', expected 'ui'." );
    return false;
  }

  return widget.load_document_file( entry.path() );
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
