/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TextureCache.h"
#include "GraphicsManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>


namespace RenderGL
{
    // constructor
    Texture::Texture()
    {
        initializeOpenGLFunctions();
        m_texture    = 0;
        m_width      = 0;
        m_height     = 0;
    }


    // constructor
    Texture::Texture(GLuint texID, uint32 width, uint32 height)
    {
        initializeOpenGLFunctions();
        m_texture    = texID;
        m_width      = width;
        m_height     = height;
    }


    // destructor
    Texture::~Texture()
    {
        glDeleteTextures(1, &m_texture);
    }


    // constructor
    TextureCache::TextureCache()
    {
        m_whiteTexture           = nullptr;
        m_defaultNormalTexture   = nullptr;

        m_entries.reserve(128);
    }


    // destructor
    TextureCache::~TextureCache()
    {
        Release();
    }


    // initialize
    bool TextureCache::Init()
    {
        // create the white dummy texture
        initializeOpenGLFunctions();
        const bool whiteTexture  = CreateWhiteTexture();
        const bool normalTexture = CreateDefaultNormalTexture();
        return (whiteTexture && normalTexture);
    }


    // release all textures
    void TextureCache::Release()
    {
        // delete all textures
        for (Entry& entry : m_entries)
        {
            delete entry.m_texture;
        }

        // clear all entries
        m_entries.clear();

        // delete the white texture
        delete m_whiteTexture;
        m_whiteTexture = nullptr;

        delete m_defaultNormalTexture;
        m_defaultNormalTexture = nullptr;
    }


    // add the texture to the cache (assume there are no duplicate names)
    void TextureCache::AddTexture(const char* filename, Texture* texture)
    {
        m_entries.emplace_back(Entry{filename, texture});
    }


    // try to locate a texture based on its name
    Texture* TextureCache::FindTexture(const char* filename) const
    {
        // get the number of entries and iterate through them
        const auto foundEntry = AZStd::find_if(begin(m_entries), end(m_entries), [filename](const Entry& entry)
        {
            return AzFramework::StringFunc::Equal(entry.m_name.c_str(), filename, false /* no case */);
        });
        return foundEntry != end(m_entries) ? foundEntry->m_texture : nullptr;
    }


    // check if we have a given texture in the cache
    bool TextureCache::CheckIfHasTexture(Texture* texture) const
    {
        // get the number of entries and iterate through them
        return AZStd::any_of(begin(m_entries), end(m_entries), [texture](const Entry& entry)
        {
            return entry.m_texture == texture;
        });
    }


    // remove an item from the cache
    void TextureCache::RemoveTexture(Texture* texture)
    {
        const auto foundEntry = AZStd::find_if(begin(m_entries), end(m_entries), [texture](const Entry& entry)
        {
            return entry.m_texture == texture;
        });

        if (foundEntry != end(m_entries))
        {
            delete foundEntry->m_texture;
            m_entries.erase(foundEntry);
        }
    }

    // create the white texture
    bool TextureCache::CreateWhiteTexture()
    {
        GLuint textureID;
        glGenTextures(1, &textureID);

        constexpr GLsizei width = 2;
        constexpr GLsizei height = 2;
        uint32 imageBuffer[4];
        {
            using AZStd::begin, AZStd::end;
            AZStd::fill(begin(imageBuffer), end(imageBuffer), MCore::RGBA(255, 255, 255, 255)); // actually abgr
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_whiteTexture = new Texture(textureID, width, height);
        return true;
    }


    // create the white texture
    bool TextureCache::CreateDefaultNormalTexture()
    {
        GLuint textureID;
        glGenTextures(1, &textureID);

        constexpr GLsizei width = 2;
        constexpr GLsizei height = 2;
        uint32 imageBuffer[4];
        {
            using AZStd::begin, AZStd::end;
            AZStd::fill(begin(imageBuffer), end(imageBuffer), MCore::RGBA(255, 128, 128, 255)); // opengl wants abgr
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_defaultNormalTexture = new Texture(textureID, width, height);
        return true;
    }
} // namespace RenderGL
