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
#include "nomlib/system/ResourceManifest.hpp"

#include "nomlib/ptree/Value.hpp"
#include "nomlib/serializers/JsonCppDeserializer.hpp"
#include "nomlib/serializers/IValueDeserializer.hpp"

namespace nom {

ResourceManifest::ResourceManifest( void ) :
  fp_( nullptr )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, nom::NOM_LOG_PRIORITY_VERBOSE );
}

ResourceManifest::~ResourceManifest( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_SYSTEM, nom::NOM_LOG_PRIORITY_VERBOSE );
}

ResourceFile::Type ResourceManifest::type_from_string( const std::string& type_str )
{
  if( type_str == "Graphic" )        return ResourceFile::Type::Graphic;
  if( type_str == "Image" )          return ResourceFile::Type::Graphic;
  if( type_str == "Texture" )        return ResourceFile::Type::Graphic;
  if( type_str == "Audio" )          return ResourceFile::Type::Audio;
  if( type_str == "Sound" )          return ResourceFile::Type::Audio;
  if( type_str == "Movie" )          return ResourceFile::Type::Movie;
  if( type_str == "TrueTypeFont" )   return ResourceFile::Type::TrueTypeFont;
  if( type_str == "TTF" )            return ResourceFile::Type::TrueTypeFont;
  if( type_str == "BitmapFont" )     return ResourceFile::Type::BitmapFont;
  if( type_str == "BMFont" )         return ResourceFile::Type::BitmapFont;

  NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                "Unknown resource type string:", type_str,
                "-- falling back to Invalid" );
  return ResourceFile::Type::Invalid;
}

void ResourceManifest::set_deserializer( std::unique_ptr<IValueDeserializer> fp )
{
  this->fp_ = std::move( fp );
}

bool ResourceManifest::load_file( const std::string& manifest_file,
                                  const std::string& manifest_node )
{
  Value obj;

  if( this->fp_ == nullptr )
  {
    this->fp_.reset( new JsonCppDeserializer() );
  }

  if( this->fp_->load( manifest_file, obj ) == false )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
                 "Could not load manifest file:", manifest_file );
    return false;
  }

  if( obj[manifest_node].null_type() )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_SYSTEM,
                  "Manifest node '", manifest_node,
                  "' is not defined in file:", manifest_file );
    return false;
  }

  if( ! obj[manifest_node].object_type() )
  {
    NOM_LOG_CRIT( NOM_LOG_CATEGORY_SYSTEM,
                  "Manifest node '", manifest_node,
                  "' must be an object type in file:", manifest_file );
    return false;
  }

  for( auto itr = obj[manifest_node].begin();
       itr != obj[manifest_node].end();
       ++itr )
  {
    const std::string& res_name = itr.key();
    const Value& entry = *itr;

    if( ! entry.object_type() )
    {
      NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                    "Skipping non-object manifest entry:", res_name );
      continue;
    }

    if( entry["path"].null_type() || ! entry["path"].string_type() )
    {
      NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                    "Skipping manifest entry '", res_name,
                    "': missing or invalid 'path' field" );
      continue;
    }

    ResourceDescriptor desc;
    desc.name = res_name;
    desc.path = entry["path"].get_string();

    if( entry["type"].string_type() )
    {
      desc.type = type_from_string( entry["type"].get_string() );
    }
    else
    {
      desc.type = ResourceFile::Type::Invalid;
    }

    if( entry["metadata"].object_type() )
    {
      const Value& meta = entry["metadata"];
      for( auto meta_itr = meta.begin(); meta_itr != meta.end(); ++meta_itr )
      {
        if( (*meta_itr).string_type() )
        {
          desc.metadata[meta_itr.key()] = (*meta_itr).get_string();
        }
      }
    }

    this->entries_.insert( { res_name, desc } );
  }

  if( this->entries_.empty() )
  {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_SYSTEM,
                  "No valid resource entries found in manifest file:",
                  manifest_file );
    return false;
  }

  NOM_LOG_INFO( NOM_LOG_CATEGORY_SYSTEM,
                "Loaded", this->entries_.size(),
                "resource descriptor(s) from manifest:", manifest_file );
  return true;
}

const ResourceDescriptor* ResourceManifest::find( const std::string& name ) const
{
  auto itr = this->entries_.find( name );
  if( itr != this->entries_.end() )
  {
    return &itr->second;
  }
  return nullptr;
}

bool ResourceManifest::exists( const std::string& name ) const
{
  return ( this->entries_.find( name ) != this->entries_.end() );
}

bool ResourceManifest::insert( const ResourceDescriptor& descriptor )
{
  if( descriptor.name.empty() )
  {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_SYSTEM,
                 "Cannot insert resource descriptor with empty name" );
    return false;
  }

  auto result = this->entries_.insert( { descriptor.name, descriptor } );
  return result.second;
}

nom::size_type ResourceManifest::size( void ) const
{
  return this->entries_.size();
}

void ResourceManifest::clear( void )
{
  this->entries_.clear();
}

ResourceManifest::const_iterator ResourceManifest::begin( void ) const
{
  return this->entries_.begin();
}

ResourceManifest::const_iterator ResourceManifest::end( void ) const
{
  return this->entries_.end();
}

} // namespace nom
