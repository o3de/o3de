/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "shadercache.h"
#include <MCore/Source/StringConversions.h>


namespace RenderGL
{
    // constructor
    ShaderCache::ShaderCache()
    {
        m_entries.reserve(128);
    }


    // destructor
    ShaderCache::~ShaderCache()
    {
        Release();
    }


    // release all shaders
    void ShaderCache::Release()
    {
        // delete all shaders
        for (Entry& entry : m_entries)
        {
            entry.m_name.clear();
            delete entry.m_shader;
        }

        // clear all entries
        m_entries.clear();
    }


    // add the shader to the cache (assume there are no duplicate names)
    void ShaderCache::AddShader(AZStd::string_view filename, Shader* shader)
    {
        m_entries.emplace_back(Entry{filename, shader});
    }


    // try to locate a shader based on its name
    Shader* ShaderCache::FindShader(AZStd::string_view filename) const
    {
        const auto foundShader = AZStd::find_if(begin(m_entries), end(m_entries), [filename](const Entry& entry)
        {
            return AzFramework::StringFunc::Equal(entry.m_name, filename, false /* no case */);
        });
        return foundShader != end(m_entries) ? foundShader->m_shader : nullptr;
    }


    // check if we have a given shader in the cache
    bool ShaderCache::CheckIfHasShader(Shader* shader) const
    {
        return AZStd::any_of(begin(m_entries), end(m_entries), [shader](const Entry& entry)
        {
            return entry.m_shader == shader;
        });
    }
}   // namespace RenderGL
