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

#ifndef __RENDERGL_TEXTURECACHE_H
#define __RENDERGL_TEXTURECACHE_H

#include <AzCore/std/string/string.h>
#include <MCore/Source/Array.h>
#include "RenderGLConfig.h"


namespace RenderGL
{
    class RENDERGL_API Texture
    {
        MCORE_MEMORYOBJECTCATEGORY(Texture, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        Texture();
        Texture(AZ::u32 texID, uint32 width, uint32 height);
        ~Texture();

        MCORE_INLINE uint32 GetHeight() const       { return mHeight; }
        MCORE_INLINE AZ::u32 GetID() const          { return mTexture; }
        MCORE_INLINE uint32 GetWidth() const        { return mWidth; }

    protected:
        AZ::u32 mTexture;
        uint32 mWidth;
        uint32 mHeight;
    };


    /**
     * The texture cache.
     * This is some storage container for texture, which prevents them from being loaded multiple times.
     */
    class RENDERGL_API TextureCache
    {
        MCORE_MEMORYOBJECTCATEGORY(TextureCache, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        TextureCache();
        ~TextureCache();

        void Release();

        void AddTexture(const char* filename, Texture* texture);
        Texture* FindTexture(const char* filename) const;
        MCORE_INLINE Texture* GetWhiteTexture()             { return mWhiteTexture; }
        MCORE_INLINE Texture* GetDefaultNormalTexture()     { return mDefaultNormalTexture; }
        bool CheckIfHasTexture(Texture* texture) const;
        bool Init();
        void RemoveTexture(Texture* texture);

    private:
        bool CreateWhiteTexture();
        bool CreateDefaultNormalTexture();

        struct Entry
        {
            AZStd::string   mName;      // the search key (unique for each texture)
            Texture*        mTexture;
        };

        MCore::Array<Entry>     mEntries;
        Texture*                mWhiteTexture;
        Texture*                mDefaultNormalTexture;
    };
}


#endif
