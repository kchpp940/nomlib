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

#include "nomlib/system/File.hpp"
#include "nomlib/system/Path.hpp"
#include "nomlib/ptree/Value.hpp"
#include "nomlib/serializers/JsonCppDeserializer.hpp"
#include "nomlib/serializers/IValueDeserializer.hpp"

namespace nom {

SearchPath::SearchPath() :
  fp_( nullptr )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, nom::NOM_LOG_PRIORITY_VERBOSE );
}

SearchPath::~SearchPath()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, nom::NOM_LOG_PRIORITY_VERBOSE );
}

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

void SearchPath::set_deserializer( std::unique_ptr<IValueDeserializer> fp )
{
  this->fp_ = std::move( fp );
}

bool SearchPath::resolve_from( const std::vector<std::string>& search_prefixes,
                               const std::string& base_path )
{
  File dir;
  Path p;

  this->search_prefix_ = search_prefixes;
  this->path_.clear();

  std::vector<std::string> candidates;
  for( auto itr = search_prefixes.begin();
       itr != search_prefixes.end();
       ++itr )
  {
    candidates.push_back( p.join( *itr, base_path ) );
  }

  for( auto itr = candidates.begin(); itr != candidates.end(); ++itr )
  {
    if( dir.exists( *itr ) )
    {
      this->path_ = *itr;
      break;
    }
    else
    {
      NOM_LOG_DEBUG( NOM_LOG_CATEGORY_SYSTEM,
                     "Not using non-existent search path:", *itr );
    }
  }

  if( ! dir.exists( this->path_ ) )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_SYSTEM,
                  "Could not find resources at any of the defined search paths:" );

    for( auto itr = candidates.begin(); itr != candidates.end(); ++itr )
    {
      NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM, *itr );
    }

    return false;
  }
  else
  {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                  "Using resources from:", this->path_ );
  }

  return true;
}

bool SearchPath::load_file( const std::string& filename,
                           const std::string& node )
{
  Value obj;
  Value prefixes;
  std::string base_path;

  if( this->fp_ == nullptr )
  {
    this->fp_.reset( new JsonCppDeserializer() );
  }

  if( this->fp_->load( filename, obj ) == false )
  {
    return false;
  }

  if( obj[node].null_type() )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_SYSTEM,
                  "Top-level object '", node,
                  "' is not defined in the search path config." );
    return false;
  }

  if( ! obj[node].object_type() )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_SYSTEM,
                  "Top-level node '", node, "' must be an object type." );
    return false;
  }

  if( obj[node]["path"].null_type() || ! obj[node]["path"].string_type() )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_SYSTEM,
                  "Path is not defined for node:", node );
    return false;
  }

  if( obj[node]["search_prefix"].null_type() ||
      ! obj[node]["search_prefix"].array_type() )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_SYSTEM,
                  "Search prefix is not defined for node:", node );
    return false;
  }

  base_path = obj[node]["path"].get_string();
  prefixes = obj[node]["search_prefix"];

  std::vector<std::string> prefix_list;
  for( auto itr = prefixes.begin(); itr != prefixes.end(); ++itr )
  {
    prefix_list.push_back( (*itr).get_string() );
  }

  return this->resolve_from( prefix_list, base_path );
}

} // namespace nom
