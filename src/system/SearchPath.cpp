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
#include "nomlib/system/SearchPath.hpp"

#include "nomlib/ptree/Value.hpp"
#include "nomlib/system/File.hpp"

namespace nom {

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

SearchPath::SearchPath()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, nom::NOM_LOG_PRIORITY_VERBOSE );
}

SearchPath::~SearchPath()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, nom::NOM_LOG_PRIORITY_VERBOSE );
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

bool SearchPath::resolve_internal()
{
  File fp;

  for( auto itr = this->search_prefix_.begin();
       itr != this->search_prefix_.end();
       ++itr )
  {
    const std::string candidate = (*itr) + this->path_;
    if( fp.is_directory( candidate ) )
    {
      this->path_ = candidate;
      NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                    "SearchPath: resolved to:", this->path_ );
      return true;
    }
  }

  NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
               "SearchPath: could not resolve a valid directory from",
               this->search_prefix_.size(), "prefix(es)" );
  return false;
}

// ---------------------------------------------------------------------------
// Value parsing (no serializers dependency — only ptree)
// ---------------------------------------------------------------------------

bool SearchPath::parse_from_value( const Value& resources_node )
{
  if( resources_node.null_type() )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_SYSTEM,
                  "SearchPath: resources Value node is null" );
    return false;
  }

  if( ! resources_node.object_type() )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_SYSTEM,
                  "SearchPath: resources Value node must be an object" );
    return false;
  }

  // Parse "search_prefix" array
  if( resources_node["search_prefix"].null_type() == false &&
      resources_node["search_prefix"].size() > 0 )
  {
    this->search_prefix_.clear();

    const Value& prefixes = resources_node["search_prefix"];
    for( auto itr = prefixes.begin(); itr != prefixes.end(); ++itr )
    {
      if( (*itr).string_type() )
      {
        this->search_prefix_.push_back( (*itr).get_string() );
      }
    }
  }

  // Parse "path" string
  if( resources_node["path"].string_type() )
  {
    this->path_ = resources_node["path"].get_string();
  }
  else
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
                 "SearchPath: 'path' field missing or not a string" );
    return false;
  }

  // Now scan prefixes
  return this->resolve_internal();
}

// ---------------------------------------------------------------------------
// Programmatic configuration
// ---------------------------------------------------------------------------

bool SearchPath::resolve_from(
  const std::vector<std::string>& search_prefixes,
  const std::string& base_path )
{
  this->search_prefix_ = search_prefixes;
  this->path_ = base_path;
  return this->resolve_internal();
}

// ---------------------------------------------------------------------------
// Query API
// ---------------------------------------------------------------------------

const std::string& SearchPath::path() const
{
  return this->path_;
}

std::string SearchPath::resolve( const std::string& relative_path ) const
{
  if( this->path_.empty() )
  {
    return relative_path;
  }

  // Match the original SearchPath behavior: simple concatenation of base
  // path with relative path. The config is expected to provide a trailing
  // path separator in the base "path" field.
  return this->path_ + relative_path;
}

const std::vector<std::string>& SearchPath::search_prefixes() const
{
  return this->search_prefix_;
}

bool SearchPath::is_resolved() const
{
  return ( this->path_.empty() == false );
}

} // namespace nom
