/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        void AddShader(AZStd::string_view filename, Shader* shader);
        Shader* FindShader(AZStd::string_view filename) const;
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
