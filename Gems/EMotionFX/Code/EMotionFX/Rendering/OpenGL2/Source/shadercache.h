/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef __RENDERGL_SHADERCACHE__H
#define __RENDERGL_SHADERCACHE__H

#include "Shader.h"
#include <AzCore/std/string/string.h>
#include <MCore/Source/Array.h>


namespace RenderGL
{
    /**
     * The shader cache.
     * This is some storage container for shaders, which prevents them from being loaded multiple times.
     */
    class RENDERGL_API ShaderCache
    {
        MCORE_MEMORYOBJECTCATEGORY(ShaderCache, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        ShaderCache();
        ~ShaderCache();     // automatically calls Release

        void Release();
        void AddShader(const char* filename, Shader* shader);
        Shader* FindShader(const char* filename) const;
        bool CheckIfHasShader(Shader* shader) const;

    private:
        // a cache entry
        struct Entry
        {
            AZStd::string   mName;      // the search key (unique for each shader)
            Shader*         mShader;
        };

        //
        MCore::Array<Entry> mEntries;   // the shader cache entries
    };
}   // namespace RenderGL

#endif
