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
        mEntries.reserve(128);
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
        for (Entry& entry : mEntries)
        {
            entry.mName.clear();
            delete entry.mShader;
        }

        // clear all entries
        mEntries.clear();
    }


    // add the shader to the cache (assume there are no duplicate names)
    void ShaderCache::AddShader(AZStd::string_view filename, Shader* shader)
    {
        mEntries.emplace_back(Entry{filename, shader});
    }


    // try to locate a shader based on its name
    Shader* ShaderCache::FindShader(AZStd::string_view filename) const
    {
        const auto foundShader = AZStd::find_if(begin(mEntries), end(mEntries), [filename](const Entry& entry)
        {
            return AzFramework::StringFunc::Equal(entry.mName, filename, false /* no case */);
        });
        return foundShader != end(mEntries) ? foundShader->mShader : nullptr;
    }


    // check if we have a given shader in the cache
    bool ShaderCache::CheckIfHasShader(Shader* shader) const
    {
        return AZStd::any_of(begin(mEntries), end(mEntries), [shader](const Entry& entry)
        {
            return entry.mShader == shader;
        });
    }
}   // namespace RenderGL
