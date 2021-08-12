/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_TEXTURECACHE_H
#define __RENDERGL_TEXTURECACHE_H

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include "RenderGLConfig.h"

#include <AzCore/PlatformIncl.h>
#include <QOpenGLExtraFunctions>

namespace RenderGL
{
    class RENDERGL_API Texture
        : protected QOpenGLExtraFunctions
    {
        MCORE_MEMORYOBJECTCATEGORY(Texture, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        Texture();
        Texture(AZ::u32 texID, uint32 width, uint32 height);
        ~Texture();

        MCORE_INLINE uint32 GetHeight() const       { return m_height; }
        MCORE_INLINE AZ::u32 GetID() const          { return m_texture; }
        MCORE_INLINE uint32 GetWidth() const        { return m_width; }

    protected:
        AZ::u32 m_texture;
        uint32 m_width;
        uint32 m_height;
    };


    /**
     * The texture cache.
     * This is some storage container for texture, which prevents them from being loaded multiple times.
     */
    class RENDERGL_API TextureCache
        : protected QOpenGLExtraFunctions
    {
        MCORE_MEMORYOBJECTCATEGORY(TextureCache, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        TextureCache();
        ~TextureCache();

        void Release();

        void AddTexture(const char* filename, Texture* texture);
        Texture* FindTexture(const char* filename) const;
        MCORE_INLINE Texture* GetWhiteTexture()             { return m_whiteTexture; }
        MCORE_INLINE Texture* GetDefaultNormalTexture()     { return m_defaultNormalTexture; }
        bool CheckIfHasTexture(Texture* texture) const;
        bool Init();
        void RemoveTexture(Texture* texture);

    private:
        bool CreateWhiteTexture();
        bool CreateDefaultNormalTexture();

        struct Entry
        {
            AZStd::string   m_name;      // the search key (unique for each texture)
            Texture*        m_texture;
        };

        AZStd::vector<Entry>     m_entries;
        Texture*                m_whiteTexture;
        Texture*                m_defaultNormalTexture;
    };
}


#endif
