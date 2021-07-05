/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mEntries.SetMemoryCategory(MEMCATEGORY_RENDERING);
        mEntries.Reserve(128);
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
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            mEntries[i].mName.clear();
            delete mEntries[i].mShader;
        }

        // clear all entries
        mEntries.Clear();
    }


    // add the shader to the cache (assume there are no duplicate names)
    void ShaderCache::AddShader(AZStd::string_view filename, Shader* shader)
    {
        mEntries.AddEmpty();
        mEntries.GetLast().mName    = filename;
        mEntries.GetLast().mShader  = shader;
    }


    // try to locate a shader based on its name
    Shader* ShaderCache::FindShader(AZStd::string_view filename) const
    {
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            if (AzFramework::StringFunc::Equal(mEntries[i].mName, filename, false /* no case */)) // non-case-sensitive name compare
            {
                return mEntries[i].mShader;
            }
        }

        // not found
        return nullptr;
    }


    // check if we have a given shader in the cache
    bool ShaderCache::CheckIfHasShader(Shader* shader) const
    {
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            if (mEntries[i].mShader == shader)
            {
                return true;
            }
        }

        return false;
    }
}   // namespace RenderGL
